/**
 * tam.c (Revised for PostgreSQL 14 - Final Fixes)
 *
 * This file implements a Table Access Method (TAM) for PostgreSQL that
 * interfaces with a custom NVRAM-backed, lock-based storage engine.
 */

// --- PostgreSQL Headers ---
#include "postgres.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "access/htup_details.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "nodes/pathnodes.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "executor/tuptable.h"
#include "storage/bufmgr.h" // For BufferAccessStrategy

// --- Your Custom Database Engine Headers ---
#include "ram_bptree.h"
#include "free_space.h"
#include "lock_manager.h"
#include "wal.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mytam_create_table);
Datum mytam_create_table(PG_FUNCTION_ARGS)
{
    // Get the table name argument from the SQL call
    text *table_name_text = PG_GETARG_TEXT_PP(0);
    char *table_name = text_to_cstring(table_name_text);

    // Call your backend's table creation function
    int table_id = db_create_table(table_name);

    if (table_id < 0)
    {
        ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("mytam: failed to create table backend for '%s'", table_name)));
    }

    pfree(table_name);

    PG_RETURN_INT32(table_id);
}

void _PG_init(void)
{
    db_init();
}

void _PG_fini(void)
{
    db_shutdown();
}

Datum mytam_handler(PG_FUNCTION_ARGS);

// --- Custom Structs for Scan Management ---
typedef struct MyTamScanDesc
{
    TableScanDescData rs_base;
    Table *table;
    int current_key;
    bool scan_started;
} MyTamScanDesc;

// --- TAM Implementation: Scan (Read) Functions ---

static TableScanDesc
mytam_scan_begin(Relation relation, Snapshot snapshot, int nkeys, ScanKey key, ParallelTableScanDesc pscan, uint32 flags)
{
    ereport(LOG, (errmsg("mytam: scan_begin called for table '%s'", RelationGetRelationName(relation))));
    MyTamScanDesc *scan;
    const char *table_name = RelationGetRelationName(relation);

    scan = (MyTamScanDesc *)palloc0(sizeof(MyTamScanDesc));
    scan->rs_base.rs_rd = relation;
    scan->rs_base.rs_snapshot = snapshot;
    scan->rs_base.rs_nkeys = nkeys;
    scan->rs_base.rs_flags = flags;

    scan->table = db_open_table(table_name);
    if (!scan->table)
    {
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_TABLE), errmsg("mytam: could not open table \"%s\"", table_name)));
    }

    scan->current_key = -1;
    scan->scan_started = false;

    return (TableScanDesc)scan;
}

static bool
mytam_scan_getnextslot(TableScanDesc sscan, ScanDirection direction, TupleTableSlot *slot)
{
    MyTamScanDesc *scan = (MyTamScanDesc *)sscan;
    int next_key;
    size_t data_size;
    void *data_ptr;
    int txn_id = 0;
    TupleDesc tupDesc;
    Datum values[2];
    bool nulls[2] = {false, false};
    HeapTuple tuple;

    if (direction != ForwardScanDirection)
    {
        elog(ERROR, "mytam: only forward scans are supported");
    }

    if (!scan->scan_started)
    {
        next_key = db_get_first_key(scan->table);
        scan->scan_started = true;
    }
    else
    {
        next_key = db_get_next_row(scan->table, scan->current_key);
    }

    if (next_key == -1)
    {
        return false;
    }
    scan->current_key = next_key;

    data_ptr = db_get_row(scan->table, txn_id, next_key, &data_size);
    if (!data_ptr)
    {
        return mytam_scan_getnextslot(sscan, direction, slot);
    }

    ExecClearTuple(slot);
    tupDesc = slot->tts_tupleDescriptor;

    values[0] = Int32GetDatum(next_key);
    values[1] = CStringGetTextDatum((const char *)data_ptr);

    tuple = heap_form_tuple(tupDesc, values, nulls);

    HeapTupleHeaderSetXmin(tuple->t_data, FrozenTransactionId);
    HeapTupleHeaderSetXmax(tuple->t_data, InvalidTransactionId);
    ItemPointerSet(&tuple->t_self, 0, (OffsetNumber)next_key);

    // FIX: Use ExecStoreHeapTuple instead of ExecStoreTuple
    ExecStoreHeapTuple(tuple, slot, false);

    pfree(DatumGetPointer(values[1]));

    return true;
}

static void
mytam_scan_end(TableScanDesc sscan)
{
    ereport(LOG, (errmsg("mytam: scan_end called")));
    MyTamScanDesc *scan = (MyTamScanDesc *)sscan;
    if (scan->table)
    {
        db_close_table(scan->table);
    }
    pfree(scan);
}

// --- TAM Implementation: DML (Write) Functions ---

static void
mytam_tuple_insert(Relation relation, TupleTableSlot *slot, CommandId cid, int options, BulkInsertState bistate)
{
    ereport(LOG, (errmsg("mytam: tuple_insert called for table '%s'", RelationGetRelationName(relation))));
    Datum *values;
    bool *nulls;
    int key;
    char *data;
    int txn_id = 0;
    bool success;

    slot_getallattrs(slot);
    values = slot->tts_values;
    nulls = slot->tts_isnull;

    if (nulls[0])
    {
        ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), errmsg("mytam: primary key cannot be null")));
    }
    key = DatumGetInt32(values[0]);

    if (!nulls[1])
    {
        data = text_to_cstring(DatumGetTextPP(values[1]));
    }
    else
    {
        data = "";
    }

    success = db_put_row(db_open_table(RelationGetRelationName(relation)), txn_id, key, data, strlen(data) + 1);

    if (!success)
    {
        ereport(ERROR, (errcode(ERRCODE_UNIQUE_VIOLATION), errmsg("mytam: row with key %d already exists", key)));
    }

    if (strcmp(data, "") != 0)
    {
        pfree(data);
    }
}

static TM_Result
mytam_tuple_delete(Relation relation, ItemPointer tid, CommandId cid, Snapshot snapshot, Snapshot crosscheck, bool wait, TM_FailureData *tmfd, bool all_dead)
{
    ereport(LOG, (errmsg("mytam: tuple_delete called for table '%s'", RelationGetRelationName(relation))));
    int key = ItemPointerGetOffsetNumber(tid);
    int txn_id = 0; // Placeholder
    bool success;

    success = db_delete_row(db_open_table(RelationGetRelationName(relation)), txn_id, key);

    if (success)
    {
        return TM_Ok;
    }
    else
    {
        return TM_SelfModified;
    }
}

static TM_Result
mytam_tuple_update(Relation relation, ItemPointer otid, TupleTableSlot *slot, CommandId cid, Snapshot snapshot, Snapshot crosscheck, bool wait, TM_FailureData *tmfd, LockTupleMode *lockmode, bool *all_dead)
{
    ereport(LOG, (errmsg("mytam: tuple_update called for table '%s'", RelationGetRelationName(relation))));
    elog(ERROR, "mytam: tuple updates not supported directly");
    return TM_SelfModified;
}

// --- Other Required TAM Functions (Stubs/Simple Implementations for PG14) ---

static void mytam_finish_bulk_insert(Relation relation, int options)
{
    ereport(LOG, (errmsg("mytam: finish_bulk_insert called for table '%s'", RelationGetRelationName(relation))));
}

static void mytam_relation_set_new_filenode(Relation relation, const RelFileNode *newrnode, char persistence, TransactionId *freezeXid, MultiXactId *minmulti) {}

static void mytam_relation_nontransactional_truncate(Relation relation)
{
    ereport(LOG, (errmsg("mytam: relation_nontransactional_truncate called for table '%s'", RelationGetRelationName(relation))));
}

static void mytam_relation_copy_data(Relation relation, const RelFileNode *newrnode)
{
    ereport(LOG, (errmsg("mytam: relation_copy_data called for table '%s'", RelationGetRelationName(relation))));
}

// FIX: Signature updated to match PG14's expectation
static void mytam_relation_copy_for_cluster(Relation new_relation, Relation old_relation, Relation old_index, bool use_sort, TransactionId OldestXmin, TransactionId *xid_map, MultiXactId *multi_map, double *num_tuples, double *tups_vacuumed, double *tups_recently_dead)
{
    ereport(LOG, (errmsg("mytam: relation_copy_for_cluster called for table '%s'", RelationGetRelationName(new_relation))));
}

// FIX: Signature updated to match PG14's expectation
static void mytam_vacuum_relation(Relation relation, VacuumParams *params, BufferAccessStrategy baccess)
{
    ereport(LOG, (errmsg("mytam: vacuum_relation called for table '%s'", RelationGetRelationName(relation))));
    if (relation)
    {
        long row_count = db_get_table_row_count(db_open_table(RelationGetRelationName(relation)));
        ereport(LOG, (errmsg("mytam: vacuum found %ld live rows in \"%s\"", row_count, RelationGetRelationName(relation))));
    }
}

static bool mytam_scan_analyze_next_block(TableScanDesc scan, BlockNumber blockno, BufferAccessStrategy bstrategy) { return false; }
static bool mytam_scan_analyze_next_tuple(TableScanDesc scan, TransactionId OldestXmin, double *liverows, double *deadrows, TupleTableSlot *slot) { return false; }
static IndexFetchTableData *mytam_index_fetch_begin(Relation relation)
{
    ereport(LOG, (errmsg("mytam: index_fetch_begin called for table '%s'", RelationGetRelationName(relation))));
    return NULL;
}
static void mytam_index_fetch_reset(IndexFetchTableData *data) {}
static void mytam_index_fetch_end(IndexFetchTableData *data) {}
static bool mytam_tuple_tid_valid(TableScanDesc sscan, ItemPointer tid) { return true; }
static bool mytam_tuple_fetch_row_version(Relation relation, ItemPointer tid, Snapshot snapshot, TupleTableSlot *slot) { return false; }
static void mytam_tuple_get_latest_tid(TableScanDesc sscan, ItemPointer tid) {}
static bool mytam_tuple_satisfies_snapshot(Relation relation, TupleTableSlot *slot, Snapshot snapshot) { return true; }

// --- Handler Registration ---

static const TableAmRoutine mytam_routine = {
    .type = T_TableAmRoutine,
    .scan_begin = mytam_scan_begin,
    .scan_getnextslot = mytam_scan_getnextslot,
    .scan_end = mytam_scan_end,
    .tuple_insert = mytam_tuple_insert,
    .tuple_delete = mytam_tuple_delete,
    .tuple_update = mytam_tuple_update,
    .finish_bulk_insert = mytam_finish_bulk_insert,
    .relation_set_new_filenode = mytam_relation_set_new_filenode,
    .relation_nontransactional_truncate = mytam_relation_nontransactional_truncate,
    .relation_copy_data = mytam_relation_copy_data,
    .relation_copy_for_cluster = mytam_relation_copy_for_cluster,
    .relation_vacuum = mytam_vacuum_relation,
    .scan_analyze_next_block = mytam_scan_analyze_next_block,
    .scan_analyze_next_tuple = mytam_scan_analyze_next_tuple,
    .index_fetch_begin = mytam_index_fetch_begin,
    .index_fetch_reset = mytam_index_fetch_reset,
    .index_fetch_end = mytam_index_fetch_end,
    .tuple_tid_valid = mytam_tuple_tid_valid,
    .tuple_fetch_row_version = mytam_tuple_fetch_row_version,
    .tuple_get_latest_tid = mytam_tuple_get_latest_tid,
    .tuple_satisfies_snapshot = mytam_tuple_satisfies_snapshot,
};

PG_FUNCTION_INFO_V1(mytam_handler);

Datum mytam_handler(PG_FUNCTION_ARGS)
{
    PG_RETURN_POINTER(&mytam_routine);
}
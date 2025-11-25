#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <immintrin.h> // For Intel intrinsics
#include "../include/wal.h"

// NVRAM persistence functions
void flush_range(void *start, size_t size)
{
    // Flush cache lines (64 bytes each)
    for (size_t i = 0; i < size; i += 64)
    {
        // THIS IS THE FIX:
        // We are using _mm_clflush, which is a standard instruction
        // and does NOT require any special compiler flags.
        _mm_clflush((char *)start + i);
    }
    _mm_sfence(); // Ensure all previous stores are visible
}

// Atomic 64-bit write using non-temporal store (movnti)
void atomic_write_64(void *dest, uint64_t val)
{
    _mm_stream_si64((long long *)dest, (long long)val);
    _mm_sfence(); // Ensure the write is committed
}

WALTable *wal_tables[MAX_TABLES] = {NULL};

int wal_create_table(int table_id, void *memory_ptr)
{
    WALTable *new_table;
    if (table_id < 0 || table_id >= MAX_TABLES)
    {
        printf("Error: Invalid table ID %d.\n", table_id);
        return 0;
    }

    if (wal_tables[table_id] != NULL)
    {
        printf("Error: WAL Table ID %d already exists.\n", table_id);
        return 0;
    }

    // Initialize the WAL table in allocated NVRAM space
    new_table = (WALTable *)memory_ptr;
    new_table->table_id = table_id;
    new_table->entry_head = NULL;
    new_table->entry_tail = NULL;
    new_table->commit_ptr = NULL;

    // Initialize mutex
    pthread_mutex_init(&new_table->mutex, NULL);

    // Ensure WAL table data is persisted to NVRAM
    flush_range(new_table, sizeof(WALTable));

    wal_tables[table_id] = new_table;
    return 1;
}

int wal_add_entry(int table_id, int key, void *data_ptr, int op, void *entry_ptr, size_t data_size)
{
    WALTable *table;
    WALEntry *entry;
    WALEntry *old_tail;

    if (table_id < 0 || table_id >= MAX_TABLES || wal_tables[table_id] == NULL)
    {
        printf("Error: WAL Table %d not found.\n", table_id);
        return 0;
    }

    table = wal_tables[table_id];

    // Lock the WAL table mutex
    pthread_mutex_lock(&table->mutex);

    // Create WAL entry in allocated NVRAM space
    entry = (WALEntry *)entry_ptr;
    entry->key = key;
    entry->data_ptr = data_ptr;
    entry->op_flag = op;
    entry->data_size = data_size;
    entry->next = NULL;

    // First, persist the entry content
    flush_range(entry, sizeof(WALEntry));

    // Add to the end of the linked list
    if (table->entry_tail == NULL)
    {
        // First entry in the list
        table->entry_head = entry;
        table->entry_tail = entry;

        // Persist head and tail pointers
        flush_range(&table->entry_head, sizeof(void *));
        flush_range(&table->entry_tail, sizeof(void *));
    }
    else
    {
        // Append to existing list
        old_tail = table->entry_tail;
        old_tail->next = entry;

        // First persist the next pointer of the old tail
        flush_range(&old_tail->next, sizeof(void *));

        // Then update the tail pointer
        table->entry_tail = entry;
        flush_range(&table->entry_tail, sizeof(void *));
    }

    // Unlock the WAL table mutex
    pthread_mutex_unlock(&table->mutex);
    return 1;
}

void wal_advance_commit_ptr(int table_id, int txn_id)
{
    WALTable *table;
    if (table_id < 0 || table_id >= MAX_TABLES || wal_tables[table_id] == NULL)
    {
        printf("Error: WAL Table %d not found.\n", table_id);
        return;
    }

    table = wal_tables[table_id];

    // Lock the WAL table mutex
    pthread_mutex_lock(&table->mutex);

    // Update commit pointer to current tail (last entry)
    table->commit_ptr = table->entry_tail;

    // Use atomic write for the commit pointer update
    atomic_write_64(&table->commit_ptr, (uint64_t)table->entry_tail);

    // Unlock the WAL table mutex
    pthread_mutex_unlock(&table->mutex);
}

void wal_show_data(void)
{
    int i;
    for (i = 0; i < MAX_TABLES; i++)
    {
        WALTable *table;
        WALEntry *current;
        int entry_count = 0;

        if (wal_tables[i] == NULL)
            continue;

        table = wal_tables[i];

        // Lock the WAL table mutex before reading
        pthread_mutex_lock(&table->mutex);

        printf("\nTable ID: %d\n", table->table_id);
        printf("Commit Pointer: %p\n", table->commit_ptr);

        // Traverse the linked list of entries
        current = table->entry_head;

        while (current != NULL)
        {
            printf("Entry %d: Key: %d | Operation: %s | Data: %s | Size: %zu | %s\n",
                   entry_count++,
                   current->key,
                   current->op_flag ? "Add" : "Delete",
                   (char *)current->data_ptr,
                   current->data_size,
                   (current == table->commit_ptr) ? "COMMITTED" : "");

            current = current->next;
        }

        // Unlock the WAL table mutex after reading
        pthread_mutex_unlock(&table->mutex);
    }
}
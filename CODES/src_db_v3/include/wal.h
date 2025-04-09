#ifndef WAL_H
#define WAL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // Added for mutex support

#define MAX_TABLES 10   // Maximum number of WAL tables
#define MAX_ENTRIES 100 // Maximum number of entries per WAL table

// WAL Entry Structure
typedef struct WALEntry
{
    int row_id;       // Unique row identifier
    void *data_ptr;   // Pointer to actual data in NVRAM
    int op_flag;      // 1 = Add, 0 = Delete
    size_t data_size; // Size of the data (added to support delete operations properly)
} WALEntry;

// WAL Table Structure
typedef struct WALTable
{
    int table_id;                   // Unique Table ID
    int entry_count;                // Number of WAL entries
    int commit_ptr;                 // Commit pointer (tracks committed entries)
    WALEntry *entries[MAX_ENTRIES]; // Array of WAL entries
    pthread_mutex_t mutex;          // Mutex for thread-safe WAL operations
} WALTable;

extern WALTable *wal_tables[MAX_TABLES];

// Function Declarations
void flush_range(void *start, size_t size);
int wal_create_table(int table_id, void *memory_ptr);                                                   // Create WAL table
int wal_add_entry(int table_id, int row_id, void *data_ptr, int op, void *entry_ptr, size_t data_size); // Add entry (modified signature)
void wal_show_data();                                                                                   // Display stored data
void wal_advance_commit_ptr(int table_id, int txn_id);                                                  // Advance commit pointer during transaction commit

#endif // WAL_H
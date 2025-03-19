#ifndef WAL_H
#define WAL_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_TABLES 10   // Maximum number of WAL tables
#define MAX_ENTRIES 100 // Maximum number of entries per WAL table

// WAL Entry Structure
typedef struct WALEntry
{
    int row_id;     // Unique row identifier
    void *data_ptr; // Pointer to actual data in NVRAM
    int op_flag;    // 1 = Add, 0 = Delete
} WALEntry;

// WAL Table Structure
typedef struct WALTable
{
    int table_id;                   // Unique Table ID
    int entry_count;                // Number of WAL entries
    int commit_ptr;                 // Commit pointer (tracks committed entries)
    WALEntry *entries[MAX_ENTRIES]; // Array of WAL entries
} WALTable;

// Function Declarations
int wal_create_table(int table_id, void *memory_ptr);                                 // Create WAL table
int wal_add_entry(int table_id, int row_id, void *data_ptr, int op, void *entry_ptr); // Add entry
void wal_show_data();                                                                 // Display stored data

#endif // WAL_H

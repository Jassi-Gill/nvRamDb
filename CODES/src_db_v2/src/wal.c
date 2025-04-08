// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "../include/wal.h"

// WALTable *wal_tables[MAX_TABLES] = {NULL};

// int wal_create_table(int table_id, void *memory_ptr)
// {
//     if (table_id < 0 || table_id >= MAX_TABLES)
//         return 0;

//     if (wal_tables[table_id] != NULL)
//     {
//         printf("Error: Table ID %d already exists.\n", table_id);
//         return 0;
//     }

//     // Initialize the WAL table in allocated space
//     WALTable *new_table = (WALTable *)memory_ptr;
//     new_table->table_id = table_id;
//     new_table->entry_count = 0;
//     new_table->commit_ptr = 0;

//     wal_tables[table_id] = new_table;
//     return 1;
// }

// int wal_add_entry(int table_id, int row_id, void *data_ptr, int op, void *entry_ptr)
// {
//     if (table_id < 0 || table_id >= MAX_TABLES || wal_tables[table_id] == NULL)
//     {
//         printf("Error: Table %d not found.\n", table_id);
//         return 0;
//     }

//     WALTable *table = wal_tables[table_id];
//     if (table->entry_count >= MAX_ENTRIES)
//     {
//         printf("Error: WAL table full for Table %d.\n", table_id);
//         return 0;
//     }

//     // Create WAL entry in allocated space
//     WALEntry *entry = (WALEntry *)entry_ptr;
//     entry->row_id = row_id;
//     entry->data_ptr = data_ptr;
//     entry->op_flag = op;

//     // Insert entry into WAL table
//     table->entries[table->entry_count++] = entry;
//     return 1;
// }

// void wal_show_data()
// {
//     for (int i = 0; i < MAX_TABLES; i++)
//     {
//         if (wal_tables[i] == NULL)
//             continue;

//         WALTable *table = wal_tables[i];
//         printf("\nTable ID: %d\n", table->table_id);
//         for (int j = 0; j < table->entry_count; j++)
//         {
//             WALEntry *entry = table->entries[j];
//             if (entry != NULL)
//             {
//                 printf("Row ID: %d | Data: %s | Operation: %s\n",
//                        entry->row_id, (char *)entry->data_ptr, entry->op_flag ? "Add" : "Delete");
//             }
//         }
//     }
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/wal.h"

WALTable *wal_tables[MAX_TABLES] = {NULL};

int wal_create_table(int table_id, void *memory_ptr)
{
    if (table_id < 0 || table_id >= MAX_TABLES)
        return 0;

    if (wal_tables[table_id] != NULL)
    {
        printf("Error: Table ID %d already exists.\n", table_id);
        return 0;
    }

    // Initialize the WAL table in allocated space
    WALTable *new_table = (WALTable *)memory_ptr;
    new_table->table_id = table_id;
    new_table->entry_count = 0;
    new_table->commit_ptr = 0;
    
    // Initialize mutex
    pthread_mutex_init(&new_table->mutex, NULL);

    wal_tables[table_id] = new_table;
    return 1;
}

int wal_add_entry(int table_id, int row_id, void *data_ptr, int op, void *entry_ptr, size_t data_size)
{
    if (table_id < 0 || table_id >= MAX_TABLES || wal_tables[table_id] == NULL)
    {
        printf("Error: Table %d not found.\n", table_id);
        return 0;
    }

    WALTable *table = wal_tables[table_id];
    
    // Lock the WAL table mutex
    pthread_mutex_lock(&table->mutex);
    
    if (table->entry_count >= MAX_ENTRIES)
    {
        printf("Error: WAL table full for Table %d.\n", table_id);
        pthread_mutex_unlock(&table->mutex);
        return 0;
    }

    // Create WAL entry in allocated space
    WALEntry *entry = (WALEntry *)entry_ptr;
    entry->row_id = row_id;
    entry->data_ptr = data_ptr;
    entry->op_flag = op;
    entry->data_size = data_size;

    // Insert entry into WAL table
    table->entries[table->entry_count++] = entry;
    
    // Unlock the WAL table mutex
    pthread_mutex_unlock(&table->mutex);
    
    return 1;
}

void wal_advance_commit_ptr(int table_id, int txn_id)
{
    if (table_id < 0 || table_id >= MAX_TABLES || wal_tables[table_id] == NULL)
    {
        printf("Error: Table %d not found.\n", table_id);
        return;
    }

    WALTable *table = wal_tables[table_id];
    
    // Lock the WAL table mutex
    pthread_mutex_lock(&table->mutex);
    
    // In a real implementation, this would advance the commit pointer
    // based on transaction ID and handle proper checkpointing
    table->commit_ptr = table->entry_count;
    
    // Unlock the WAL table mutex
    pthread_mutex_unlock(&table->mutex);
}

void wal_show_data()
{
    for (int i = 0; i < MAX_TABLES; i++)
    {
        if (wal_tables[i] == NULL)
            continue;

        WALTable *table = wal_tables[i];
        
        // Lock the WAL table mutex before reading
        pthread_mutex_lock(&table->mutex);
        
        printf("\nTable ID: %d (Commit Ptr: %d)\n", table->table_id, table->commit_ptr);
        for (int j = 0; j < table->entry_count; j++)
        {
            WALEntry *entry = table->entries[j];
            if (entry != NULL)
            {
                printf("Row ID: %d | Data: %s | Operation: %s | Size: %zu\n",
                       entry->row_id, 
                       (char *)entry->data_ptr, 
                       entry->op_flag ? "Add" : "Delete", 
                       entry->data_size);
            }
        }
        
        // Unlock the WAL table mutex after reading
        pthread_mutex_unlock(&table->mutex);
    }
}
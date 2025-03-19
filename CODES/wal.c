#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_TABLES 1024  // Maximum number of tables
#define WAL_ENTRIES 1000 // Max WAL entries per table

typedef struct
{
    int add_delete_flag; // 1 for add, 0 for delete
    int table_key;       // Unique table identifier
    int row_key;         // Unique row identifier
    void *data_ptr;      // Pointer to actual data in memory
} WAL_Entry;

typedef struct
{
    WAL_Entry entries[WAL_ENTRIES];
    int commit_ptr; // Points to the last committed entry
    int next_entry; // Points to the next available slot
    sem_t lock;     // Semaphore for concurrency control
} WAL_Table;

WAL_Table wal_tables[MAX_TABLES];

void init_wal()
{
    for (int i = 0; i < MAX_TABLES; i++)
    {
        wal_tables[i].commit_ptr = 0;
        wal_tables[i].next_entry = 0;
        sem_init(&wal_tables[i].lock, 0, 1);
    }
}

void insert_wal(int table_key, int row_key, void *data, int is_add)
{
    int index = table_key % MAX_TABLES;
    sem_wait(&wal_tables[index].lock);

    WAL_Table *wal = &wal_tables[index];
    int pos = wal->next_entry;

    wal->entries[pos].add_delete_flag = is_add;
    wal->entries[pos].table_key = table_key;
    wal->entries[pos].row_key = row_key;
    wal->entries[pos].data_ptr = data;

    wal->next_entry = (pos + 1) % WAL_ENTRIES;

    sem_post(&wal->lock);
}

void commit_wal(int table_key)
{
    int index = table_key % MAX_TABLES;
    sem_wait(&wal_tables[index].lock);

    WAL_Table *wal = &wal_tables[index];
    wal->commit_ptr = wal->next_entry;

    sem_post(&wal->lock);
}

void print_wal(int table_key)
{
    int index = table_key % MAX_TABLES;
    sem_wait(&wal_tables[index].lock);

    WAL_Table *wal = &wal_tables[index];
    printf("WAL for Table %d:\n", table_key);
    for (int i = 0; i < wal->next_entry; i++)
    {
        printf("Entry %d: %s, Row %d, Data Ptr: %p\n", i, wal->entries[i].add_delete_flag ? "ADD" : "DELETE", wal->entries[i].row_key, wal->entries[i].data_ptr);
    }

    sem_post(&wal->lock);
}

int main()
{
    init_wal();
    int data1 = 42, data2 = 84;
    insert_wal(1, 100, &data1, 1);
    insert_wal(1, 101, &data2, 1);
    commit_wal(1);
    print_wal(1);
    return 0;
}
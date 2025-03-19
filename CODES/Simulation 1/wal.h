#ifndef WAL_H
#define WAL_H

#include <stddef.h>

typedef struct WalEntry
{
    int table_id;
    int row_id;
    char operation; // 'A' for Add, 'D' for Delete
    size_t data_offset;
    struct WalEntry *next;
} WalEntry;

typedef struct WalTable
{
    int table_id;
    WalEntry *head;
    size_t commit_pointer;
    struct WalTable *next;
} WalTable;

void init_wal();
void create_table(int table_id);
void add_data(int table_id, int row_id, char operation, const char *data);
void show_data();
void save_wal_metadata();
void load_wal_metadata();

#endif

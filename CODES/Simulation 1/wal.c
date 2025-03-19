#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "wal.h"
#include "free_space.h"

#define FILEPATH "/home/master/Desktop/My Works/PROJECT/CODES/NVRAM/nvram.img"

WalTable *wal_tables = NULL;

// Initialize WAL, load from NVRAM
void init_wal()
{
    load_wal_metadata();
}

// Create WAL table in NVRAM
void create_table(int table_id)
{
    WalTable *new_table = (WalTable *)malloc(sizeof(WalTable));
    new_table->table_id = table_id;
    new_table->head = NULL;
    new_table->commit_pointer = allocate_space(sizeof(WalTable));
    new_table->next = wal_tables;
    wal_tables = new_table;
    save_wal_metadata();
}

// Add entry to WAL
void add_data(int table_id, int row_id, char operation, const char *data)
{
    WalTable *table = wal_tables;
    while (table && table->table_id != table_id)
        table = table->next;

    if (!table)
    {
        printf("Table not found!\n");
        return;
    }

    size_t data_offset = allocate_space(strlen(data) + 1);
    int fd = open(FILEPATH, O_RDWR);
    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    strcpy((char *)map + data_offset, data);
    munmap(map, FILESIZE);
    close(fd);

    WalEntry *new_entry = (WalEntry *)malloc(sizeof(WalEntry));
    new_entry->table_id = table_id;
    new_entry->row_id = row_id;
    new_entry->operation = operation;
    new_entry->data_offset = data_offset;
    new_entry->next = table->head;
    table->head = new_entry;

    save_wal_metadata();
}

// Show stored data
void show_data()
{
    WalTable *table = wal_tables;
    while (table)
    {
        printf("Table ID: %d\n", table->table_id);
        WalEntry *entry = table->head;
        while (entry)
        {
            int fd = open(FILEPATH, O_RDWR);
            void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            printf("Row ID: %d, Operation: %c, Data: %s\n", entry->row_id, entry->operation, (char *)map + entry->data_offset);
            munmap(map, FILESIZE);
            close(fd);
            entry = entry->next;
        }
        table = table->next;
    }
}

// Save WAL metadata in NVRAM
void save_wal_metadata()
{
    int fd = open(FILEPATH, O_RDWR);
    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(map + sizeof(FreeSpaceManager), wal_tables, sizeof(WalTable));
    munmap(map, FILESIZE);
    close(fd);
}

// Load WAL metadata from NVRAM
void load_wal_metadata()
{
    int fd = open(FILEPATH, O_RDWR);
    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(wal_tables, map + sizeof(FreeSpaceManager), sizeof(WalTable));
    munmap(map, FILESIZE);
    close(fd);
}

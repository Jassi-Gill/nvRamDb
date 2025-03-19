#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "free_space.h"
#include "wal.h"

void create_table()
{
    int table_id;
    printf("Enter Table ID: ");
    scanf("%d", &table_id);

    // Request free space for WAL table creation
    void *table_ptr = allocate_memory(sizeof(WALTable));
    if (!table_ptr)
    {
        printf("Error: Not enough space to create WAL table.\n");
        return;
    }

    // Create WAL table in allocated space
    if (!wal_create_table(table_id, table_ptr))
    {
        printf("Error: WAL table creation failed.\n");
        free_memory(table_ptr, sizeof(WALTable));
        return;
    }

    printf("Table %d created successfully.\n", table_id);
}

void add_data()
{
    int table_id, row_id, op;
    char data[256];

    printf("Enter Table ID: ");
    scanf("%d", &table_id);
    printf("Enter Row ID: ");
    scanf("%d", &row_id);
    printf("Enter Data: ");
    scanf(" %[^\n]", data);
    printf("Enter Operation (1 = Add, 0 = Delete): ");
    scanf("%d", &op);

    // Allocate space for WAL entry
    void *wal_entry_ptr = allocate_memory(sizeof(WALEntry));
    if (!wal_entry_ptr)
    {
        printf("Error: Not enough space to store WAL entry.\n");
        return;
    }

    // Allocate space for actual data
    void *data_ptr = allocate_memory(strlen(data) + 1);
    if (!data_ptr)
    {
        printf("Error: Not enough space to store actual data.\n");
        free_memory(wal_entry_ptr, sizeof(WALEntry));
        return;
    }

    // Store data in NVRAM
    strcpy((char *)data_ptr, data);

    // Insert WAL entry
    if (!wal_add_entry(table_id, row_id, data_ptr, op, wal_entry_ptr))
    {
        printf("Error: Failed to add WAL entry.\n");
        free_memory(wal_entry_ptr, sizeof(WALEntry));
        free_memory(data_ptr, strlen(data) + 1);
        return;
    }

    printf("Data added successfully to Table %d, Row %d.\n", table_id, row_id);
}

void show_data()
{
    printf("\n=== Stored Data ===\n");
    wal_show_data();
    printf("===================\n");
}

int main()
{
    init_free_space(); // Initialize free space management

    while (1)
    {
        printf("\nMenu:\n");
        printf("1. Create Table\n");
        printf("2. Add Data\n");
        printf("3. Show Data\n");
        printf("4. Exit\n");
        printf("Enter choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            create_table();
            break;
        case 2:
            add_data();
            break;
        case 3:
            show_data();
            break;
        case 4:
            cleanup_free_space(); // Cleanup before exit
            exit(0);
        default:
            printf("Invalid choice, try again.\n");
        }
    }
}

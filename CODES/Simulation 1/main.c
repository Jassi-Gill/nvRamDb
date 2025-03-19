#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "free_space.h"
#include "wal.h"

void display_menu()
{
    printf("\n========================\n");
    printf("Write-Ahead Log (WAL) System\n");
    printf("========================\n");
    printf("1. Create Table\n");
    printf("2. Add Data\n");
    printf("3. Show Data\n");
    printf("4. Exit\n");
    printf("Choose an option: ");
}

int main()
{
    int choice, table_id, row_id;
    char operation;
    char data[256];

    // Initialize Free Space Manager & WAL System
    init_free_space_manager();
    init_wal();

    while (1)
    {
        display_menu();
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            // Create a table
            printf("Enter Table ID: ");
            scanf("%d", &table_id);
            create_table(table_id);
            printf("Table %d created successfully!\n", table_id);
            break;

        case 2:
            // Add data (Insert/Delete)
            printf("Enter Table ID: ");
            scanf("%d", &table_id);
            printf("Enter Row ID: ");
            scanf("%d", &row_id);
            printf("Enter Data: ");
            scanf(" %[^\n]", data);
            printf("Enter Operation (A for Add, D for Delete): ");
            scanf(" %c", &operation);

            add_data(table_id, row_id, operation, data);
            printf("Data processed successfully!\n");
            break;

        case 3:
            // Show stored data
            printf("\nStored Data:\n");
            show_data();
            break;

        case 4:
            // Exit
            printf("Exiting program...\n");
            return 0;

        default:
            printf("Invalid choice! Please try again.\n");
        }
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../include/ram_bptree.h"
#include "../include/free_space.h"
#include "../include/wal.h"
#include "../include/lock_manager.h"

#define NUM_THREADS 4
#define NUM_OPERATIONS 5
#define CONTENTION_KEYS 3  // Shared keys all threads will try to access

// Shared table for all threads
Table *shared_table = NULL;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to pass data to thread
typedef struct {
    int thread_id;
    int start_key;
} ThreadData;

// Thread function for concurrent database operations
void* thread_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int thread_id = data->thread_id;
    int start_key = data->start_key;
    
    // Begin a transaction
    int txn_id = db_begin_transaction();
    
    pthread_mutex_lock(&print_mutex);
    printf("Thread %d: Started transaction %d\n", thread_id, txn_id);
    pthread_mutex_unlock(&print_mutex);
    
    if (txn_id < 0) {
        pthread_mutex_lock(&print_mutex);
        printf("Thread %d: Failed to start transaction\n", thread_id);
        pthread_mutex_unlock(&print_mutex);
        return NULL;
    }
    
    // Perform multiple operations in the transaction
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // Sometimes operate on a thread-specific key, sometimes on a shared key to create contention
        bool use_shared_key = (rand() % 3 == 0);  // 1/3 chance of using a shared key
        int key;
        
        if (use_shared_key) {
            // Use a shared key that will cause contention
            key = 9000 + (rand() % CONTENTION_KEYS);
        } else {
            // Use a thread-specific key
            key = start_key + i;
        }
        
        char data[64];
        snprintf(data, sizeof(data), "Data from thread %d, operation %d", thread_id, i);
        
        // Random delay with wider range (0-500ms) to increase chance of lock conflicts
        usleep(rand() % 500000);
        
        // Insert row
        pthread_mutex_lock(&print_mutex);
        printf("Thread %d: Attempting to insert key %d\n", thread_id, key);
        pthread_mutex_unlock(&print_mutex);
        
        // Loop a few times if we can't get the lock, to better demonstrate waiting behavior
        bool success = false;
        int retry_count = 0;
        while (!success && retry_count < 3) {
            success = db_put_row(shared_table, txn_id, key, data, strlen(data) + 1);
            if (!success) {
                pthread_mutex_lock(&print_mutex);
                printf("Thread %d: Lock contention on key %d, retrying...\n", thread_id, key);
                pthread_mutex_unlock(&print_mutex);
                
                // Wait a bit before retrying
                usleep(100000 + (rand() % 200000));  // 100-300ms
                retry_count++;
            }
        }
        
        pthread_mutex_lock(&print_mutex);
        if (success) {
            printf("Thread %d: Successfully inserted key %d\n", thread_id, key);
        } else {
            printf("Thread %d: Failed to insert key %d after retries (locked by another thread)\n", thread_id, key);
        }
        pthread_mutex_unlock(&print_mutex);
        
        // Try to read some data
        // Randomly choose between reading a thread-specific key or a shared contention key
        int read_key;
        if (rand() % 2 == 0) {
            // Try to read a contention key
            read_key = 9000 + (rand() % CONTENTION_KEYS);
        } else {
            // Try to read another thread's data
            read_key = (rand() % NUM_THREADS) * 100 + (rand() % NUM_OPERATIONS);
        }
        
        pthread_mutex_lock(&print_mutex);
        printf("Thread %d: Attempting to read key %d\n", thread_id, read_key);
        pthread_mutex_unlock(&print_mutex);
        
        size_t size;
        void* read_data = db_get_row(shared_table, txn_id, read_key, &size);
        
        pthread_mutex_lock(&print_mutex);
        if (read_data) {
            printf("Thread %d: Read key %d: %s\n", thread_id, read_key, (char*)read_data);
        } else {
            printf("Thread %d: Key %d not found or locked\n", thread_id, read_key);
        }
        pthread_mutex_unlock(&print_mutex);
    }
    
    // Random chance of committing or aborting (80% commit, 20% abort)
    bool should_commit = (rand() % 100) < 80;
    
    if (should_commit) {
        pthread_mutex_lock(&print_mutex);
        printf("Thread %d: Committing transaction %d\n", thread_id, txn_id);
        pthread_mutex_unlock(&print_mutex);
        
        if (db_commit_transaction(txn_id)) {
            pthread_mutex_lock(&print_mutex);
            printf("Thread %d: Transaction %d committed successfully\n", thread_id, txn_id);
            pthread_mutex_unlock(&print_mutex);
        } else {
            pthread_mutex_lock(&print_mutex);
            printf("Thread %d: Failed to commit transaction %d\n", thread_id, txn_id);
            pthread_mutex_unlock(&print_mutex);
        }
    } else {
        pthread_mutex_lock(&print_mutex);
        printf("Thread %d: Aborting transaction %d\n", thread_id, txn_id);
        pthread_mutex_unlock(&print_mutex);
        
        if (db_abort_transaction(txn_id)) {
            pthread_mutex_lock(&print_mutex);
            printf("Thread %d: Transaction %d aborted successfully\n", thread_id, txn_id);
            pthread_mutex_unlock(&print_mutex);
        } else {
            pthread_mutex_lock(&print_mutex);
            printf("Thread %d: Failed to abort transaction %d\n", thread_id, txn_id);
            pthread_mutex_unlock(&print_mutex);
        }
    }
    
    free(data);
    return NULL;
}

// Function to demonstrate deadlock scenario
void demonstrate_deadlock() {
    printf("\n--- Demonstrating Deadlock Detection/Prevention ---\n");
    
    // Create two threads that will attempt to acquire locks in opposite order
    pthread_t thread1, thread2;
    ThreadData *data1 = malloc(sizeof(ThreadData));
    ThreadData *data2 = malloc(sizeof(ThreadData));
    
    data1->thread_id = 100;
    data1->start_key = 1000;
    
    data2->thread_id = 200;
    data2->start_key = 2000;
    
    // Create threads
    pthread_create(&thread1, NULL, thread_worker, data1);
    pthread_create(&thread2, NULL, thread_worker, data2);
    
    // Wait for threads to complete
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("--- Deadlock Demonstration Complete ---\n\n");
}

// Main function
int main() {
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize the database
    db_init();
    
    // Create a test table
    int table_id = db_create_table("concurrent_test");
    if (table_id < 0) {
        printf("Failed to create table\n");
        return 1;
    }
    
    // Open the table
    shared_table = db_open_table("concurrent_test");
    if (!shared_table) {
        printf("Failed to open table\n");
        return 1;
    }
    
    printf("=== Starting Multithreaded Database Test ===\n");
    
    // Create thread data
    ThreadData thread_data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start_key = i * 100;  // Different key ranges for each thread
        
        if (pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Show the WAL data after concurrent operations
    printf("\n=== Write-Ahead Log After Concurrent Operations ===\n");
    wal_show_data();
    
    // Demonstrate table contents
    printf("\n=== Table Contents After Concurrent Operations ===\n");
    int txn_id = db_begin_transaction();
    int key = -1;
    while ((key = db_get_next_row(shared_table, key)) != -1) {
        size_t size;
        void *data = db_get_row(shared_table, txn_id, key, &size);
        if (data) {
            printf("Row %d: %s\n", key, (char*)data);
        }
    }
    db_commit_transaction(txn_id);
    
    // Optionally demonstrate deadlock scenario
    demonstrate_deadlock();
    
    // Close the table and shutdown
    db_close_table(shared_table);
    db_shutdown();
    
    printf("\n=== Multithreaded Database Test Complete ===\n");
    return 0;
}
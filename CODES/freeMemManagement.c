#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_HEAP_SIZE 100 // Maximum number of free blocks

typedef struct FreeBlock
{
    void *address;
    size_t size;
} FreeBlock;

typedef struct HeapManager
{
    size_t total_size;
    FreeBlock heap[MAX_HEAP_SIZE]; // Binary heap (max heap)
    int heap_size;
} HeapManager;

// Metadata structure (stored at start of each memory block)
typedef struct Metadata
{
    uint8_t allocated;   // 1 bit for allocation status (0 = free, 1 = allocated)
    size_t size;         // 4 bytes for size
    FreeBlock *heapNode; // 4-byte pointer to heap node
} Metadata;

HeapManager manager;

// Heap helper functions
void swap(FreeBlock *a, FreeBlock *b)
{
    FreeBlock temp = *a;
    *a = *b;
    *b = temp;
}

// Maintain max heap property
void heapifyDown(int index)
{
    int largest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < manager.heap_size && manager.heap[left].size > manager.heap[largest].size)
        largest = left;
    if (right < manager.heap_size && manager.heap[right].size > manager.heap[largest].size)
        largest = right;
    if (largest != index)
    {
        swap(&manager.heap[index], &manager.heap[largest]);
        heapifyDown(largest);
    }
}

void heapifyUp(int index)
{
    int parent = (index - 1) / 2;
    while (index > 0 && manager.heap[index].size > manager.heap[parent].size)
    {
        swap(&manager.heap[index], &manager.heap[parent]);
        index = parent;
        parent = (index - 1) / 2;
    }
}

// Insert a free block into heap
void insertHeap(void *address, size_t size)
{
    if (manager.heap_size >= MAX_HEAP_SIZE)
    {
        printf("Heap is full, cannot insert more blocks.\n");
        return;
    }
    manager.heap[manager.heap_size].address = address;
    manager.heap[manager.heap_size].size = size;
    heapifyUp(manager.heap_size);
    manager.heap_size++;
}

// Extract the largest free block from heap
FreeBlock extractMax()
{
    FreeBlock maxBlock = manager.heap[0];
    manager.heap[0] = manager.heap[--manager.heap_size];
    heapifyDown(0);
    return maxBlock;
}

// Initialize heap manager
void initHeap(size_t total_size)
{
    manager.total_size = total_size;
    manager.heap_size = 0;

    void *startAddress = malloc(total_size);
    if (!startAddress)
    {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    // Store initial free block in heap
    insertHeap(startAddress, total_size);
}

// Allocate memory
void *allocateMemory(size_t size)
{
    if (manager.heap_size == 0)
    {
        printf("No available memory.\n");
        return NULL;
    }

    FreeBlock block = extractMax();
    if (block.size < size)
    {
        printf("Not enough space available.\n");
        return NULL;
    }

    Metadata *meta = (Metadata *)block.address;
    meta->allocated = 1;
    meta->size = size;

    void *allocatedMemory = (void *)(meta + 1); // Return memory after metadata

    // If leftover space exists, insert it back into heap
    size_t remainingSize = block.size - size - sizeof(Metadata);
    if (remainingSize > sizeof(Metadata))
    {
        void *remainingAddress = (void *)((char *)block.address + size + sizeof(Metadata));
        insertHeap(remainingAddress, remainingSize);
    }

    return allocatedMemory;
}

// Free memory
void freeMemory(void *ptr)
{
    if (!ptr)
        return;

    Metadata *meta = (Metadata *)ptr - 1;
    if (!meta->allocated)
    {
        printf("Memory block already free.\n");
        return;
    }

    meta->allocated = 0;
    insertHeap((void *)meta, meta->size + sizeof(Metadata));
}

// Display heap state
void displayHeap()
{
    printf("Free memory blocks (max heap order):\n");
    for (int i = 0; i < manager.heap_size; i++)
    {
        printf("Block at %p | Size: %zu\n", manager.heap[i].address, manager.heap[i].size);
    }
}

int main()
{
    // Initialize 1MB heap
    initHeap(1024 * 1024);

    printf("\nAllocating 256 bytes...\n");
    void *ptr1 = allocateMemory(256);
    displayHeap();

    printf("\nAllocating 128 bytes...\n");
    void *ptr2 = allocateMemory(128);
    displayHeap();

    printf("\nFreeing first block...\n");
    freeMemory(ptr1);
    displayHeap();

    return 0;
}

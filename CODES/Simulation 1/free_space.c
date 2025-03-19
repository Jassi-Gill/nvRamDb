#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "free_space.h"



FreeSpaceManager manager = {0, NULL};

// Initialize free space manager, loading metadata if available
void init_free_space_manager()
{
    manager.total_size = FILESIZE;
    manager.free_list = NULL;
    load_free_space_metadata();

    if (!manager.free_list)
    {
        // If no metadata was loaded, initialize full free block
        manager.free_list = (FreeBlock *)malloc(sizeof(FreeBlock));
        manager.free_list->size = FILESIZE;
        manager.free_list->offset = 0;
        manager.free_list->next = NULL;
        save_free_space_metadata();
    }
}

// Allocate memory using first-fit
size_t allocate_space(size_t size)
{
    FreeBlock *prev = NULL, *curr = manager.free_list;

    while (curr)
    {
        if (curr->size >= size)
        {
            size_t allocated_offset = curr->offset;
            curr->offset += size;
            curr->size -= size;

            if (curr->size == 0)
            { // Remove empty block
                if (prev)
                    prev->next = curr->next;
                else
                    manager.free_list = curr->next;
                free(curr);
            }
            save_free_space_metadata();
            return allocated_offset;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1; // No space available
}

// Free memory and merge adjacent blocks
void free_space(size_t offset, size_t size)
{
    FreeBlock *new_block = (FreeBlock *)malloc(sizeof(FreeBlock));
    new_block->size = size;
    new_block->offset = offset;
    new_block->next = manager.free_list;
    manager.free_list = new_block;
    save_free_space_metadata();
}

// Save free space metadata in NVRAM
void save_free_space_metadata()
{
    int fd = open(FILEPATH, O_RDWR);
    if (fd == -1)
        return;

    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        return;
    }

    memcpy(map, &manager, sizeof(FreeSpaceManager));
    munmap(map, FILESIZE);
    close(fd);
}

// Load free space metadata from NVRAM
void load_free_space_metadata()
{
    int fd = open(FILEPATH, O_RDWR);
    if (fd == -1)
        return;

    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        return;
    }

    memcpy(&manager, map, sizeof(FreeSpaceManager));
    munmap(map, FILESIZE);
    close(fd);
}

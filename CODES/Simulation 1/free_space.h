#ifndef FREE_SPACE_H
#define FREE_SPACE_H

#include <stddef.h>

#define FILEPATH "/home/master/Desktop/My Works/PROJECT/CODES/NVRAM/nvram.img"
#define FILESIZE (2L * 1024 * 1024 * 1024) // 2GB
// Structure for a free memory block
typedef struct FreeBlock
{
    size_t size;
    size_t offset; // Offset in NVRAM file
    struct FreeBlock *next;
} FreeBlock;

// Structure for managing free space
typedef struct FreeSpaceManager
{
    size_t total_size;
    FreeBlock *free_list;
} FreeSpaceManager;

void init_free_space_manager();
size_t allocate_space(size_t size);
void free_space(size_t offset, size_t size);
void save_free_space_metadata();
void load_free_space_metadata();

#endif

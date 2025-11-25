#ifndef FREE_SPACE_H
#define FREE_SPACE_H

#include <stddef.h>
#include <pthread.h> // Include this for pthread_mutex_t

#define FILEPATH "/dev/dax0.0"
#define FILESIZE (2L * 1024 * 1024 * 1024) // 2GB

extern pthread_mutex_t free_space_mutex;

// --- ADD PROTOTYPES for all public functions ---
void init_free_space(void);
void *allocate_memory(size_t size);
void free_memory(void *ptr, size_t size);
void cleanup_free_space(void);

#endif // FREE_SPACE_H
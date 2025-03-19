#include "free_space.c"

int main()
{
    init_free_space();

    // Allocate memory
    void *mem1 = allocate_memory(1024);
    printf("Allocated 1024 bytes at: %p\n", mem1);

    void *mem2 = allocate_memory(2048);
    printf("Allocated 2048 bytes at: %p\n", mem2);

    // Free memory
    free_memory(mem1, 1024);
    printf("Freed 1024 bytes\n");

    free_memory(mem2, 2048);
    printf("Freed 2048 bytes\n");

    cleanup_free_space();
    return 0;
}

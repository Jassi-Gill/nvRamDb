#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// #define FILEPATH "/home/master/Desktop/My Works/PROJECT/CODES/NVRAM/nvram.img"
#define FILEPATH "/dev/dax0.0"
#define FILESIZE (2L * 1024 * 1024 * 1024) // 2GB

int main()
{
    int fd = open(FILEPATH, O_RDWR);
    if (fd == -1)
    {
        perror("Error opening file");
        return 1;
    }

    void *map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

    sprintf((char *)map, "Hello, NVRAM!");

    printf("Data written: %s\n", (char *)map);

    printf("NVRAM Data: %.*s\n", 13, map);

    // if (msync(map, FILESIZE, MS_SYNC) == -1)
    // {
    //     perror("msync failed");
    //     munmap(map, FILESIZE);
    //     close(fd);
    //     return 1;
    // }
    // printf("Changes synced to disk\n");

    // Clean up
    if (munmap(map, FILESIZE) == -1)
    {
        perror("munmap failed");
    }
    close(fd);
    return 0;
}

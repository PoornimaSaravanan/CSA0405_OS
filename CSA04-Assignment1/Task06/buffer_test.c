
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <buffer_size>\n", argv[0]);
        return 1;
    }

    int buffer_size = atoi(argv[1]);

    if (buffer_size <= 0) {
        printf("Invalid buffer size\n");
        return 1;
    }

    int fd = open("testfile.txt", O_RDONLY);

    if (fd == -1) {
        perror("File open failed");
        return 1;
    }

    char *buffer = malloc(buffer_size);

    if (buffer == NULL) {
        perror("Memory allocation failed");
        close(fd);
        return 1;
    }

    ssize_t bytes;
    int read_count = 0;

    while ((bytes = read(fd, buffer, buffer_size)) > 0) {
        read_count++;
    }

    if (bytes == -1) {
        perror("Read failed");
        free(buffer);
        close(fd);
        return 1;
    }

    printf("Buffer Size: %d bytes\n", buffer_size);
    printf("Read System Calls: %d\n", read_count + 1);

    free(buffer);
    close(fd);

    return 0;
}

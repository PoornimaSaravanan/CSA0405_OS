#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    int fd;
    char buffer[256];
    ssize_t n;

    fd = open("patient_record.txt", O_RDONLY);

    if (fd == -1) {
        perror("File opening failed");
        return 1;
    }

    printf("Patient Record:\n");

    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, n) == -1) {
            perror("Write failed");
            close(fd);
            return 1;
        }
    }

    if (n == -1) {
        perror("Read failed");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
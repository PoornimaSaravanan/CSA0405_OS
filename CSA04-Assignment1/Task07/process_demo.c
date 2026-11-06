
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int status;

    printf("Parent Process Started\n");

    // First child process
    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Child Process 1: Executing command\n");

        execlp("expr", "expr", "6", "*", "7", (char *)NULL);

        perror("Exec failed");
        _exit(127);
    }

    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child Process 1 Exit Status: %d\n",
               WEXITSTATUS(status));
    }

    // Second child process
    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Child Process 2: Running\n");
        _exit(3);
    }

    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child Process 2 Exit Status: %d\n",
               WEXITSTATUS(status));
    }

    printf("Parent Process Completed\n");

    return 0;
}

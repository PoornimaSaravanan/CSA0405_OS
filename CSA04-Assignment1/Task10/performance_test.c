
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>

#define COUNT 1000

// Thread function
void *thread_function(void *arg)
{
    (void)arg;
    return NULL;
}

// Function to calculate execution time
double get_time(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main(void)
{
    double start, end;
    double process_time, thread_time;

    printf("SMART HOSPITAL - PERFORMANCE MEASUREMENT\n");
    printf("=========================================\n\n");

    // PROCESS PERFORMANCE TEST
    start = get_time();

    for (int i = 0; i < COUNT; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0)
        {
            _exit(0);
        }

        if (waitpid(pid, NULL, 0) < 0)
        {
            perror("Wait failed");
            return 1;
        }
    }

    end = get_time();
    process_time = end - start;

    printf("PROCESS PERFORMANCE:\n");
    printf("Processes Created: %d\n", COUNT);
    printf("Total Time: %.6f seconds\n\n", process_time);

    // THREAD PERFORMANCE TEST
    pthread_t thread;

    start = get_time();

    for (int i = 0; i < COUNT; i++)
    {
        int result = pthread_create(
            &thread,
            NULL,
            thread_function,
            NULL
        );

        if (result != 0)
        {
            fprintf(stderr, "Thread creation failed: %d\n", result);
            return 1;
        }

        result = pthread_join(thread, NULL);

        if (result != 0)
        {
            fprintf(stderr, "Thread join failed: %d\n", result);
            return 1;
        }
    }

    end = get_time();
    thread_time = end - start;

    printf("THREAD PERFORMANCE:\n");
    printf("Threads Created: %d\n", COUNT);
    printf("Total Time: %.6f seconds\n\n", thread_time);

    // FINAL COMPARISON
    printf("PERFORMANCE COMPARISON:\n");
    printf("Process Time: %.6f seconds\n", process_time);
    printf("Thread Time : %.6f seconds\n", thread_time);

    if (process_time > thread_time)
    {
        printf("Result: Threads were faster in this experiment.\n");
    }
    else if (thread_time > process_time)
    {
        printf("Result: Processes were faster in this experiment.\n");
    }
    else
    {
        printf("Result: Both took approximately equal time.\n");
    }

    return 0;
}

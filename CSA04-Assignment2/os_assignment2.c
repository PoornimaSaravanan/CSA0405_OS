
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define MAX 10
#define SIZE 5

double timer_now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1000000000.0;
}

int getint(const char *msg, int min, int max)
{
    int n;
    printf("%s", msg);

    while (scanf("%d", &n) != 1 || n < min || n > max) {
        printf("Enter a number between %d and %d: ", min, max);
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
    return n;
}

/* Q1 - PROCESS MANAGEMENT */

void q1(void)
{
    printf("\nQ1 - PROCESS MANAGEMENT\n");
    printf("Parent PID: %ld\n", (long)getpid());

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        printf("Child PID: %ld\n", (long)getpid());
        printf("Child parent PID: %ld\n", (long)getppid());
        fflush(stdout);
        _exit(0);
    }

    waitpid(pid, NULL, 0);

    printf("Parent process completed waiting.\n");
    printf("Process states: NEW -> READY -> RUNNING -> WAITING -> TERMINATED\n");
}

/* Q2 - CPU SCHEDULING */

typedef struct {
    int id, arrival, burst, priority, remaining, completion;
} Process;

void q2(void)
{
    int n = getint("\nNumber of processes (1-10): ", 1, MAX);
    Process original[MAX];

    for (int i = 0; i < n; i++) {
        original[i].id = i + 1;

        printf("\nProcess P%d\n", i + 1);
        original[i].arrival = getint("Arrival time: ", 0, 1000);
        original[i].burst = getint("Burst time: ", 1, 1000);
        original[i].priority = getint("Priority (smaller = higher): ", 0, 1000);
        original[i].remaining = original[i].burst;
    }

    for (int alg = 0; alg < 5; alg++) {
        Process p[MAX];
        memcpy(p, original, sizeof(Process) * n);

        int time = 0, completed = 0;
        int gantt[100000], count = 0;
        int quantum = 2;

        const char *names[] = {
            "FCFS", "SJF", "SRTF", "Priority", "Round Robin"
        };

        printf("\n========== %s ==========\n", names[alg]);

        if (alg == 4)
            quantum = getint("Round Robin quantum: ", 1, 100);

        if (alg == 0 || alg == 1 || alg == 3) {
            int done[MAX] = {0};

            while (completed < n) {
                int best = -1;

                for (int i = 0; i < n; i++) {
                    if (!done[i] && p[i].arrival <= time) {
                        if (best == -1 ||
                            (alg == 0 && p[i].arrival < p[best].arrival) ||
                            (alg == 1 && p[i].burst < p[best].burst) ||
                            (alg == 3 && p[i].priority < p[best].priority))
                            best = i;
                    }
                }

                if (best == -1) {
                    time++;
                    continue;
                }

                done[best] = 1;

                for (int j = 0; j < p[best].burst; j++) {
                    gantt[count++] = p[best].id;
                    time++;
                }

                p[best].completion = time;
                completed++;
            }
        }
        else if (alg == 2) {
            while (completed < n) {
                int best = -1;

                for (int i = 0; i < n; i++) {
                    if (p[i].remaining > 0 && p[i].arrival <= time &&
                        (best == -1 ||
                         p[i].remaining < p[best].remaining))
                        best = i;
                }

                if (best == -1) {
                    time++;
                    continue;
                }

                gantt[count++] = p[best].id;
                p[best].remaining--;
                time++;

                if (p[best].remaining == 0) {
                    p[best].completion = time;
                    completed++;
                }
            }
        }
        else {
            int queue[100000], front = 0, rear = 0;
            int added[MAX] = {0};

            while (completed < n) {
                for (int i = 0; i < n; i++) {
                    if (!added[i] && p[i].arrival <= time) {
                        queue[rear++] = i;
                        added[i] = 1;
                    }
                }

                if (front == rear) {
                    time++;
                    continue;
                }

                int k = queue[front++];
                int run = p[k].remaining < quantum ?
                          p[k].remaining : quantum;

                for (int j = 0; j < run; j++) {
                    gantt[count++] = p[k].id;
                    p[k].remaining--;
                    time++;

                    for (int i = 0; i < n; i++) {
                        if (!added[i] && p[i].arrival <= time) {
                            queue[rear++] = i;
                            added[i] = 1;
                        }
                    }
                }

                if (p[k].remaining == 0) {
                    p[k].completion = time;
                    completed++;
                } else {
                    queue[rear++] = k;
                }
            }
        }

        double totalWT = 0, totalTAT = 0;

        printf("\nPID\tAT\tBT\tCT\tTAT\tWT\n");

        for (int i = 0; i < n; i++) {
            int tat = p[i].completion - p[i].arrival;
            int wt = tat - p[i].burst;

            totalWT += wt;
            totalTAT += tat;

            printf("P%d\t%d\t%d\t%d\t%d\t%d\n",
                   p[i].id, p[i].arrival, p[i].burst,
                   p[i].completion, tat, wt);
        }

        printf("Average Waiting Time: %.2f\n", totalWT / n);
        printf("Average Turnaround Time: %.2f\n", totalTAT / n);

        printf("Gantt Chart: ");
        for (int i = 0; i < count; i++)
            printf("| P%d ", gantt[i]);
        printf("|\n");
    }
}

/* Q3 - MULTITHREADING */

void *thread_work(void *arg)
{
    long loops = *(long *)arg;
    volatile unsigned long value = 0;

    for (long i = 0; i < loops; i++)
        value = value * 1664525UL + 1013904223UL;

    return NULL;
}

void q3(void)
{
    int pools[] = {1, 2, 4, 8};
    long total = 20000000;
    double baseline = 0;

    printf("\nQ3 - THREAD POOL PERFORMANCE\n");

    for (int s = 0; s < 4; s++) {
        int n = pools[s];
        pthread_t threads[8];
        long work[8];

        double start = timer_now();

        for (int i = 0; i < n; i++) {
            work[i] = total / n;

            if (pthread_create(&threads[i], NULL,
                               thread_work, &work[i]) != 0) {
                perror("pthread_create");
                return;
            }
        }

        for (int i = 0; i < n; i++)
            pthread_join(threads[i], NULL);

        double elapsed = timer_now() - start;

        if (n == 1)
            baseline = elapsed;

        printf("%d threads: %.6f seconds | Speed-up: %.2fx\n",
               n, elapsed, baseline / elapsed);
    }
}

/* Q4 - PIPE COMMUNICATION */

void pipe_demo(void)
{
    int fd[2];
    char message[] = "Smart Hospital Patient Information";
    char received[100] = {0};

    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return;
    }

    if (pid == 0) {
        close(fd[1]);

        ssize_t r = read(fd[0], received, sizeof(received) - 1);

        if (r > 0) {
            received[r] = '\0';
            printf("Child received: %s\n", received);
            fflush(stdout);
        }

        close(fd[0]);
        _exit(0);
    }

    close(fd[0]);
    write(fd[1], message, sizeof(message));
    close(fd[1]);

    waitpid(pid, NULL, 0);
}

/* Q4 - SHARED MEMORY */

void shared_memory_demo(void)
{
    const char *name = "/csa04_os_shared";
    const size_t size = 4096;

    int fd = shm_open(name, O_CREAT | O_RDWR, 0600);

    if (fd == -1) {
        perror("shm_open");
        return;
    }

    if (ftruncate(fd, (off_t)size) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(name);
        return;
    }

    char *memory = mmap(NULL, size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);

    if (memory == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(name);
        return;
    }

    strcpy(memory, "Patient P1001 - Stable Condition");

    printf("Shared Memory Data: %s\n", memory);

    munmap(memory, size);
    close(fd);
    shm_unlink(name);
}

void q4(void)
{
    printf("\nQ4 - INTER-PROCESS COMMUNICATION\n");

    printf("\nPIPE:\n");
    pipe_demo();

    printf("\nSHARED MEMORY:\n");
    shared_memory_demo();
}

/* Q5 - SEMAPHORE SYNCHRONISATION */

int buffer[SIZE];
int in = 0, out = 0;

sem_t empty_slots, full_slots, mutex_lock;

void *producer(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 10; i++) {
        sem_wait(&empty_slots);
        sem_wait(&mutex_lock);

        buffer[in] = i;
        in = (in + 1) % SIZE;

        printf("Producer produced: %d\n", i);

        sem_post(&mutex_lock);
        sem_post(&full_slots);
    }

    return NULL;
}

void *consumer(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 10; i++) {
        sem_wait(&full_slots);
        sem_wait(&mutex_lock);

        int item = buffer[out];
        out = (out + 1) % SIZE;

        printf("Consumer consumed: %d\n", item);

        sem_post(&mutex_lock);
        sem_post(&empty_slots);
    }

    return NULL;
}

void q5(void)
{
    printf("\nQ5 - PRODUCER CONSUMER PROBLEM\n");

    if (sem_init(&empty_slots, 0, SIZE) != 0) {
        perror("sem_init");
        return;
    }

    if (sem_init(&full_slots, 0, 0) != 0) {
        perror("sem_init");
        sem_destroy(&empty_slots);
        return;
    }

    if (sem_init(&mutex_lock, 0, 1) != 0) {
        perror("sem_init");
        sem_destroy(&empty_slots);
        sem_destroy(&full_slots);
        return;
    }

    pthread_t p, c;

    int rp = pthread_create(&p, NULL, producer, NULL);
    int rc = pthread_create(&c, NULL, consumer, NULL);

    if (rp != 0 || rc != 0) {
        printf("Thread creation failed.\n");
        if (rp == 0) pthread_join(p, NULL);
        if (rc == 0) pthread_join(c, NULL);
    } else {
        pthread_join(p, NULL);
        pthread_join(c, NULL);
        printf("Producer and Consumer completed successfully.\n");
    }

    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    sem_destroy(&mutex_lock);
}

/* Q6 - BANKER'S ALGORITHM */

void q6(void)
{
    int n = getint("\nNumber of processes (1-10): ", 1, MAX);
    int m = getint("Number of resource types (1-5): ", 1, 5);

    int allocation[MAX][MAX] = {0};
    int maximum[MAX][MAX] = {0};
    int need[MAX][MAX] = {0};
    int available[MAX] = {0};
    int work[MAX] = {0};
    int finish[MAX] = {0};
    int sequence[MAX];
    int count = 0;

    printf("\nEnter Allocation Matrix:\n");

    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            char msg[80];
            snprintf(msg, sizeof(msg), "P%d R%d: ", i, j);
            allocation[i][j] = getint(msg, 0, 10000);
        }

    printf("\nEnter Maximum Matrix:\n");

    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            char msg[80];
            snprintf(msg, sizeof(msg), "P%d R%d: ", i, j);
            maximum[i][j] = getint(msg, 0, 10000);

            if (maximum[i][j] < allocation[i][j]) {
                printf("Invalid input: Max is below Allocation.\n");
                return;
            }

            need[i][j] = maximum[i][j] - allocation[i][j];
        }

    printf("\nEnter Available Resources:\n");

    for (int j = 0; j < m; j++) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Resource %d: ", j);
        available[j] = getint(msg, 0, 10000);
    }

    memcpy(work, available, sizeof(int) * m);

    printf("\nNeed Matrix:\n");

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++)
            printf("%d ", need[i][j]);
        printf("\n");
    }

    for (int pass = 0; pass < n; pass++) {
        for (int i = 0; i < n; i++) {
            if (finish[i])
                continue;

            int possible = 1;

            for (int j = 0; j < m; j++) {
                if (need[i][j] > work[j])
                    possible = 0;
            }

            if (possible) {
                for (int j = 0; j < m; j++)
                    work[j] += allocation[i][j];

                finish[i] = 1;
                sequence[count++] = i;
            }
        }
    }

    if (count == n) {
        printf("\nSAFE STATE\nSafe Sequence: ");

        for (int i = 0; i < n; i++)
            printf("P%d%s", sequence[i],
                   i == n - 1 ? "\n" : " -> ");
    } else {
        printf("\nUNSAFE STATE - No safe sequence.\n");
    }

    double start = timer_now();

    for (int i = 0; i < 1000; i++) {
        volatile int check = 0;
        for (int j = 0; j < n; j++)
            check += finish[j];
        (void)check;
    }

    printf("1000 checks completed in %.6f seconds.\n",
           timer_now() - start);
}

/* MAIN MENU */

int main(void)
{
    int choice;

    while (1) {
        printf("\n========================================\n");
        printf(" CSA04 - OPERATING SYSTEMS ASSIGNMENT 2\n");
        printf("========================================\n");

        printf("1. Q1 - Process Management\n");
        printf("2. Q2 - CPU Scheduling Algorithms\n");
        printf("3. Q3 - Multithreading\n");
        printf("4. Q4 - IPC (Pipe and Shared Memory)\n");
        printf("5. Q5 - Semaphore Synchronisation\n");
        printf("6. Q6 - Banker's Algorithm\n");
        printf("0. Exit\n");

        choice = getint("Enter your choice: ", 0, 6);

        switch (choice) {
            case 1: q1(); break;
            case 2: q2(); break;
            case 3: q3(); break;
            case 4: q4(); break;
            case 5: q5(); break;
            case 6: q6(); break;
            case 0:
                printf("Program completed.\n");
                return 0;
        }
    }
}

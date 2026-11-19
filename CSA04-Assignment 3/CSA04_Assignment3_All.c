
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>

/* =====================================================
   Q1 - LINKING, LOADING AND ADDRESS BINDING
   ===================================================== */

void q1_linking(void) {
    FILE *fp;

    printf("\n========== Q1: LINKING AND LOADING ==========\n");

    fp = fopen("crypto.c", "w");
    if (fp == NULL) {
        perror("crypto.c");
        return;
    }

    fprintf(fp,
        "#include <stdio.h>\n"
        "void encrypt_message(void) {\n"
        "    puts(\"Encryption library version 1\");\n"
        "}\n");
    fclose(fp);

    fp = fopen("bank.c", "w");
    if (fp == NULL) {
        perror("bank.c");
        return;
    }

    fprintf(fp,
        "#include <stdio.h>\n"
        "void encrypt_message(void);\n"
        "int main(void) {\n"
        "    puts(\"Banking application started\");\n"
        "    encrypt_message();\n"
        "    return 0;\n"
        "}\n");
    fclose(fp);

    printf("\nBuilding static executable...\n");
    (void)system("gcc -c crypto.c -o crypto.o");
    (void)system("gcc bank.c crypto.o -o bank_static");

    printf("\nBuilding dynamic shared library...\n");
    (void)system("gcc -fPIC -c crypto.c -o crypto_pic.o");
    (void)system("gcc -shared -o libcrypto_demo.so crypto_pic.o");
    (void)system("gcc bank.c -L. -lcrypto_demo -Wl,-rpath,'$ORIGIN' -o bank_dynamic");

    printf("\nStatic executable output:\n");
    (void)system("./bank_static");

    printf("\nDynamic executable output:\n");
    (void)system("./bank_dynamic");

    printf("\nExecutable sizes:\n");
    (void)system("ls -lh bank_static bank_dynamic libcrypto_demo.so");

    printf("\nDynamic library dependencies:\n");
    (void)system("ldd ./bank_dynamic");

    printf("\nRun-time linking using dlopen() and dlsym():\n");

    fp = fopen("runtime_load.c", "w");

    if (fp != NULL) {
        fprintf(fp,
            "#include <stdio.h>\n"
            "#include <dlfcn.h>\n"
            "typedef void (*encrypt_fn)(void);\n"
            "int main(void) {\n"
            " void *h = dlopen(\"./libcrypto_demo.so\", RTLD_NOW);\n"
            " if (!h) { puts(dlerror()); return 1; }\n"
            " dlerror();\n"
            " encrypt_fn f = (encrypt_fn)dlsym(h, \"encrypt_message\");\n"
            " const char *e = dlerror();\n"
            " if (e) { puts(e); dlclose(h); return 1; }\n"
            " f();\n"
            " dlclose(h);\n"
            " return 0;\n"
            "}\n");

        fclose(fp);
        (void)system("gcc runtime_load.c -o runtime_load -ldl");
        (void)system("./runtime_load");
    }

    printf("\nAddress binding stages:\n");
    printf("Compile-time: Address fixed during compilation.\n");
    printf("Load-time: Relocation performed by the loader.\n");
    printf("Execution-time: Address translation during execution.\n");
    printf("PIC: Position-independent code supports shared libraries.\n");

    printf("\nQ1 completed.\n");
}

/* =====================================================
   Q2 - PAGING
   ===================================================== */

void q2_paging(void) {
    const int PAGE_SIZE = 1024;
    const int PAGES = 8;

    int page_table[8] = {5, 2, -1, 7, 0, -1, 3, -1};

    unsigned logical[] = {
        0, 1500, 3100, 6200, 9000, 2049
    };

    printf("\n========== Q2: PAGING ==========\n");
    printf("Page size = %d bytes\n", PAGE_SIZE);

    for (int i = 0; i < 6; i++) {
        unsigned address = logical[i];
        unsigned page = address / PAGE_SIZE;
        unsigned offset = address % PAGE_SIZE;

        printf("\nLogical address: %u\n", address);
        printf("Page number: %u\n", page);
        printf("Offset: %u\n", offset);

        if (page >= PAGES) {
            printf("Result: Invalid address\n");
        } else if (page_table[page] == -1) {
            printf("Result: Page fault (page not present)\n");
        } else {
            unsigned physical =
                (unsigned)page_table[page] * PAGE_SIZE + offset;

            printf("Frame number: %d\n", page_table[page]);
            printf("Physical address: %u\n", physical);
        }
    }

    printf("\nPage-table size calculations:\n");
    printf("32-bit address, 4 KB pages, 4-byte PTE = 4 MiB\n");
    printf("48-bit address, 4 KB pages, 4-byte PTE = 256 GiB\n");

    printf("\nTLB Effective Access Time (memory = 100 ns, TLB = 10 ns):\n");

    double hit[] = {0.80, 0.95, 0.99};

    for (int i = 0; i < 3; i++) {
        double eat =
            hit[i] * 110.0 + (1.0 - hit[i]) * 210.0;

        printf("Hit ratio %.0f%%: EAT = %.2f ns\n",
               hit[i] * 100.0, eat);
    }

    printf("\nActual Linux page size:\n");
    (void)system("getconf PAGESIZE");

    printf("\nQ2 completed.\n");
}

/* =====================================================
   Q3 - SEGMENTATION
   ===================================================== */

typedef struct {
    int base;
    int limit;
} Segment;

void q3_segmentation(void) {
    Segment segments[] = {
        {10000, 4000},
        {20000, 2000},
        {30000, 1000}
    };

    int tests[8][2] = {
        {0, 120}, {1, 1500}, {2, 999}, {0, 4000},
        {1, 2001}, {2, 10}, {0, 3999}, {1, 0}
    };

    printf("\n========== Q3: SEGMENTATION ==========\n");

    printf("Segment table:\n");

    for (int i = 0; i < 3; i++) {
        printf("Segment %d: Base = %d, Limit = %d\n",
               i, segments[i].base, segments[i].limit);
    }

    printf("\nLogical address translations:\n");

    for (int i = 0; i < 8; i++) {
        int segment = tests[i][0];
        int offset = tests[i][1];

        printf("(%d, %d): ", segment, offset);

        if (segment < 0 || segment >= 3) {
            printf("Invalid segment\n");
        } else if (offset < 0 ||
                   offset >= segments[segment].limit) {
            printf("Protection fault - offset exceeds limit\n");
        } else {
            printf("Physical address = %d\n",
                   segments[segment].base + offset);
        }
    }

    printf("\nSegmentation with paging:\n");
    printf("Logical address -> Segment table -> Segment page table\n");
    printf("-> Frame number + page offset -> Physical address\n");

    printf("\nLinux process memory map information:\n");
    printf("Process memory maps can be inspected using /proc/PID/maps.\n");
    printf("Invalid memory access can cause SIGSEGV.\n");

    printf("\nFragmentation example:\n");
    printf("Total free memory = 75 MB\n");
    printf("Largest free hole = 30 MB\n");
    printf("External fragmentation = 75 - 30 = 45 MB\n");

    printf("\nQ3 completed.\n");
}

/* =====================================================
   Q4 - VIRTUAL MEMORY AND DEMAND PAGING
   ===================================================== */

void q4_virtual_memory(void) {
    double rates[] = {0.001, 0.00001, 0.000001};

    double memory_time = 100.0;
    double fault_time = 8000000.0;

    printf("\n========== Q4: VIRTUAL MEMORY ==========\n");

    printf("\nPage-fault handling steps:\n");
    printf("1. CPU references a page not present in memory.\n");
    printf("2. Hardware generates a page-fault trap.\n");
    printf("3. OS checks address validity and permissions.\n");
    printf("4. OS locates the page or creates a zero-filled page.\n");
    printf("5. OS selects a free frame or replacement victim.\n");
    printf("6. Page table and TLB are updated.\n");
    printf("7. Faulting instruction is restarted.\n");

    printf("\nEffective Access Time:\n");

    for (int i = 0; i < 3; i++) {
        double eat =
            (1.0 - rates[i]) * memory_time +
            rates[i] * (fault_time + memory_time);

        printf("Fault rate = %.8f, EAT = %.2f ns\n",
               rates[i], eat);
    }

    printf("\nMaximum fault rate for 10%% slowdown:\n");
    printf("p <= 10 / 8000000 = 0.00000125\n");
    printf("Approximately one fault per 800000 references.\n");

    printf("\nCopy-on-Write demonstration using fork():\n");

    size_t bytes = 4 * 1024 * 1024;
    char *memory = malloc(bytes);

    if (memory == NULL) {
        perror("malloc");
        return;
    }

    for (size_t i = 0; i < bytes; i += 4096)
        memory[i] = 1;

    pid_t pid = fork();

    if (pid == 0) {
        for (size_t i = 0; i < bytes; i += 4096)
            memory[i] = 2;

        printf("Child process modified its memory pages.\n");
        fflush(stdout);
        _exit(0);

    } else if (pid > 0) {
        wait(NULL);
        printf("Parent process completed. Copy-on-Write was demonstrated.\n");

    } else {
        perror("fork");
    }

    free(memory);

    printf("\nDemand paging experiment:\n");

    long page_size = sysconf(_SC_PAGESIZE);
    size_t total = 16 * 1024 * 1024;
    size_t pages = total / (size_t)page_size;

    volatile unsigned char *area = calloc(1, total);

    if (area != NULL) {
        struct rusage before, after;

        getrusage(RUSAGE_SELF, &before);

        for (size_t i = 0; i < pages; i++)
            area[i * (size_t)page_size] = 1;

        getrusage(RUSAGE_SELF, &after);

        printf("Touched 16 MB sequentially.\n");
        printf("Minor page faults: %ld\n",
               after.ru_minflt - before.ru_minflt);
        printf("Major page faults: %ld\n",
               after.ru_majflt - before.ru_majflt);

        free((void *)area);
    }

    printf("\nThrashing occurs when active working sets exceed available frames.\n");
    printf("Working-set management can reduce excessive page faults.\n");

    printf("\nQ4 completed.\n");
}

/* =====================================================
   Q5 - PAGE REPLACEMENT ALGORITHMS
   ===================================================== */

#define MAXF 10

int find_page(const int frames[], int count, int page) {
    for (int i = 0; i < count; i++) {
        if (frames[i] == page)
            return i;
    }

    return -1;
}

int fifo(const int ref[], int length, int capacity) {
    int frames[MAXF];
    int count = 0, pointer = 0, faults = 0;

    for (int i = 0; i < MAXF; i++)
        frames[i] = -1;

    for (int i = 0; i < length; i++) {
        if (find_page(frames, count, ref[i]) >= 0)
            continue;

        faults++;

        if (count < capacity) {
            frames[count++] = ref[i];
        } else {
            frames[pointer] = ref[i];
            pointer = (pointer + 1) % capacity;
        }
    }

    return faults;
}

int lru(const int ref[], int length, int capacity) {
    int frames[MAXF], last[MAXF];
    int count = 0, faults = 0;

    for (int i = 0; i < MAXF; i++) {
        frames[i] = -1;
        last[i] = -1;
    }

    for (int i = 0; i < length; i++) {
        int pos = find_page(frames, count, ref[i]);

        if (pos >= 0) {
            last[pos] = i;
            continue;
        }

        faults++;

        if (count < capacity) {
            frames[count] = ref[i];
            last[count] = i;
            count++;
        } else {
            int victim = 0;

            for (int j = 1; j < capacity; j++) {
                if (last[j] < last[victim])
                    victim = j;
            }

            frames[victim] = ref[i];
            last[victim] = i;
        }
    }

    return faults;
}

int optimal(const int ref[], int length, int capacity) {
    int frames[MAXF], count = 0, faults = 0;

    for (int i = 0; i < MAXF; i++)
        frames[i] = -1;

    for (int i = 0; i < length; i++) {
        if (find_page(frames, count, ref[i]) >= 0)
            continue;

        faults++;

        if (count < capacity) {
            frames[count++] = ref[i];
            continue;
        }

        int victim = 0;
        int farthest = -1;

        for (int j = 0; j < capacity; j++) {
            int next = length;

            for (int k = i + 1; k < length; k++) {
                if (ref[k] == frames[j]) {
                    next = k;
                    break;
                }
            }

            if (next > farthest) {
                farthest = next;
                victim = j;
            }
        }

        frames[victim] = ref[i];
    }

    return faults;
}

int clock_replace(const int ref[], int length, int capacity) {
    int frames[MAXF], bits[MAXF];
    int hand = 0, count = 0, faults = 0;

    for (int i = 0; i < MAXF; i++) {
        frames[i] = -1;
        bits[i] = 0;
    }

    for (int i = 0; i < length; i++) {
        int pos = find_page(frames, count, ref[i]);

        if (pos >= 0) {
            bits[pos] = 1;
            continue;
        }

        faults++;

        if (count < capacity) {
            frames[count] = ref[i];
            bits[count] = 1;
            count++;
            hand = count % capacity;

        } else {
            while (bits[hand] == 1) {
                bits[hand] = 0;
                hand = (hand + 1) % capacity;
            }

            frames[hand] = ref[i];
            bits[hand] = 1;
            hand = (hand + 1) % capacity;
        }
    }

    return faults;
}

void q5_replacement(void) {
    int ref[] = {
        7,0,1,2,0,3,0,4,2,3,
        0,3,2,1,2,0,1,7,0,1
    };

    int belady[] = {
        1,2,3,4,1,2,5,1,2,3,4,5
    };

    int length = (int)(sizeof(ref) / sizeof(ref[0]));
    int b_length = (int)(sizeof(belady) / sizeof(belady[0]));

    printf("\n========== Q5: PAGE REPLACEMENT ==========\n");

    printf("\nMain reference string:\n");

    for (int i = 0; i < length; i++)
        printf("%d ", ref[i]);

    printf("\n\nPage-fault comparison:\n");

    for (int f = 3; f <= 4; f++) {
        printf("%d frames: FIFO = %d, LRU = %d, Optimal = %d, Clock = %d\n",
               f,
               fifo(ref, length, f),
               lru(ref, length, f),
               optimal(ref, length, f),
               clock_replace(ref, length, f));
    }

    printf("\nBelady's anomaly demonstration:\n");

    printf("FIFO with 3 frames = %d faults\n",
           fifo(belady, b_length, 3));

    printf("FIFO with 4 frames = %d faults\n",
           fifo(belady, b_length, 4));

    printf("\nClock uses reference bits and a circular pointer.\n");
    printf("Optimal is a theoretical benchmark because future references are unknown.\n");

    printf("\nQ5 completed.\n");
}

/* =====================================================
   Q6 - DISK MANAGEMENT
   ===================================================== */

void q6_disk(void) {
    double disk_size = 4.0;
    int disks = 6;
    double q = 0.02;

    double raid0 = disks * disk_size;
    double raid1_sixway = disk_size;
    double raid5 = (disks - 1) * disk_size;
    double raid6 = (disks - 2) * disk_size;
    double raid10 = (disks / 2) * disk_size;

    double raid1_loss = q * q;
    double raid5_loss = 0.0;

    for (int k = 2; k <= disks; k++) {
        double combination = 1.0;

        for (int j = 1; j <= k; j++)
            combination *= (double)(disks - j + 1) / j;

        double probability = combination;

        for (int j = 0; j < k; j++)
            probability *= q;

        for (int j = 0; j < disks - k; j++)
            probability *= (1.0 - q);

        raid5_loss += probability;
    }

    printf("\n========== Q6: DISK MANAGEMENT ==========\n");

    printf("\nLinux block-device information:\n");
    (void)system("lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS");

    printf("\nSwap information:\n");
    (void)system("swapon --show");

    printf("\nFile-system usage:\n");
    (void)system("df -h");

    printf("\nRAID capacity calculation (6 disks x 4 TB):\n");
    printf("RAID 0 = %.0f TB\n", raid0);
    printf("RAID 1 six-way mirror = %.0f TB\n", raid1_sixway);
    printf("RAID 5 = %.0f TB\n", raid5);
    printf("RAID 6 = %.0f TB\n", raid6);
    printf("RAID 10 = %.0f TB\n", raid10);

    printf("\nIllustrative independent failure probability (q = %.2f):\n", q);

    printf("Two-disk mirror data-loss probability = %.4f%%\n",
           raid1_loss * 100.0);

    printf("Six-disk RAID 5 data-loss probability = %.4f%%\n",
           raid5_loss * 100.0);

    printf("\nDisk management concepts:\n");
    printf("Partitioning divides a disk into logical regions.\n");
    printf("Formatting creates a file system such as ext4 or XFS.\n");
    printf("Bad blocks are remapped or marked unusable.\n");
    printf("Swap provides backing storage for less-active memory pages.\n");
    printf("RAID improves availability but does not replace backups.\n");

    printf("\nQ6 completed.\n");
}

/* =====================================================
   Q7 - MEMORY MANAGEMENT DESIGN
   ===================================================== */

long elapsed_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L
         + (end.tv_nsec - start.tv_nsec);
}

void q7_memory_plan(void) {
    const size_t bytes = 16 * 1024 * 1024;

    volatile unsigned char *buffer = malloc(bytes);

    printf("\n========== Q7: MEMORY MANAGEMENT PLAN ==========\n");

    printf("\nMemory budget:\n");
    printf("Physical RAM = 2 GB\n");
    printf("Machine-learning task = 1.5 GB\n");
    printf("Remaining before OS/control/logging reservations = 0.5 GB\n");
    printf("A fourth 500 MB task cannot safely be added without redesign.\n");

    if (buffer == NULL) {
        perror("malloc");
        return;
    }

    printf("\nMemory locking experiment:\n");

    int locked = 0;

    if (mlock((const void *)buffer, bytes) == 0) {
        locked = 1;
        printf("mlock succeeded: 16 MB memory region locked.\n");
    } else {
        perror("mlock");
        printf("Locking may require suitable privileges or memory limits.\n");
    }

    long page_size = sysconf(_SC_PAGESIZE);

    for (size_t i = 0; i < bytes; i += (size_t)page_size)
        buffer[i] = 1;

    printf("Memory pages were touched before measurement.\n");

    struct timespec start, end;
    long worst = 0;

    volatile uint64_t checksum = 0;

    for (int i = 0; i < 10000; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int j = 0; j < 100; j++) {
            size_t index =
                ((size_t)i * 4096 + (size_t)j * 64) % bytes;

            checksum += buffer[index];
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        long duration = elapsed_ns(start, end);

        if (duration > worst)
            worst = duration;
    }

    printf("Mode: %s\n", locked ? "Locked" : "Unlocked");
    printf("Worst observed loop duration = %ld ns\n", worst);
    printf("Checksum = %llu\n", (unsigned long long)checksum);

    if (locked)
        munlock((const void *)buffer, bytes);

    free((void *)buffer);

    printf("\nPinned-page replacement simulation:\n");

    int frames[6] = {-1, -1, -1, -1, -1, -1};
    int pinned[6] = {1, 1, 0, 0, 0, 0};

    int reference[] = {0,1,2,3,2,4,5,2,3,1,4};

    int hand = 2;
    int faults = 0;

    int n = (int)(sizeof(reference) / sizeof(reference[0]));

    for (int i = 0; i < n; i++) {
        int hit = 0;

        for (int j = 0; j < 6; j++) {
            if (frames[j] == reference[i])
                hit = 1;
        }

        if (hit)
            continue;

        faults++;

        int victim = -1;

        for (int k = 0; k < 6; k++) {
            int j = (hand + k) % 6;

            if (!pinned[j]) {
                victim = j;
                break;
            }
        }

        if (victim == -1) {
            printf("No evictable frame available.\n");
            break;
        }

        frames[victim] = reference[i];
        hand = (victim + 1) % 6;

        printf("Page %d loaded into frame %d\n",
               reference[i], victim);
    }

    printf("Page faults in simulation = %d\n", faults);
    printf("Pinned frames were never selected for replacement.\n");

    printf("\nRecommended design:\n");
    printf("1. Lock and pre-fault real-time control memory.\n");
    printf("2. Avoid dynamic allocation in the critical loop.\n");
    printf("3. Use bounded logging buffers.\n");
    printf("4. Stream or reduce the machine-learning workload if needed.\n");
    printf("5. Consider an RTOS for strict hard real-time requirements.\n");

    printf("\nNote: mlock does not by itself guarantee hard real-time deadlines.\n");
    printf("\nQ7 completed.\n");
}

/* =====================================================
   MAIN - RUN ALL QUESTIONS
   ===================================================== */

int main(void) {
    printf("=================================================\n");
    printf(" CSA04 OPERATING SYSTEMS - ASSIGNMENT 3 (CO3)\n");
    printf(" MEMORY MANAGEMENT IMPLEMENTATION\n");
    printf("=================================================\n");

    q1_linking();
    q2_paging();
    q3_segmentation();
    q4_virtual_memory();
    q5_replacement();
    q6_disk();
    q7_memory_plan();

    printf("\n=================================================\n");
    printf(" ALL Q1 TO Q7 IMPLEMENTATION SECTIONS COMPLETED\n");
    printf("=================================================\n");

    return 0;
}

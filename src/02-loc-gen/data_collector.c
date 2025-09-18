#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_collector.h"

int read_cpu_stats() {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            printf("CPU stats: %s", line);
            break;
        }
    }
    fclose(f);
    return 0;
}

int read_memory_stats() {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }
    char line[256];
    printf("Memory stats:\n");
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);
    return 0;
}

int read_io_stats() {
    FILE* f = fopen("/proc/diskstats", "r");
    if (!f) {
        perror("Failed to open /proc/diskstats");
        return -1;
    }
    char line[256];
    printf("IO stats:\n");
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);
    return 0;
}

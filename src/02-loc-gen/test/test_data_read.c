#include <stdio.h>
#include <assert.h>
#include "../data_collector.h"

int main() {
    printf("Testing data reading functions...\n");

    assert(read_cpu_stats() == 0 && "Failed to read CPU stats");
    assert(read_memory_stats() == 0 && "Failed to read Memory stats");
    assert(read_io_stats() == 0 && "Failed to read IO stats");

    printf("All data reading tests passed!\n");
    return 0;
}

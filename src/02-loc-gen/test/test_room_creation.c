#include <stdio.h>
#include <assert.h>
#include "../room_manager.h"

int main() {
    printf("Testing room creation...\n");

    int cpu_room = create_room("cpu-room", 1000);
    int mem_room = create_room("memory-room", 2000);
    int inf_room = create_room("inf-stats-room", 3000);

    assert(cpu_room >= 0 && "Failed to create cpu-room");
    assert(mem_room >= 0 && "Failed to create memory-room");
    assert(inf_room >= 0 && "Failed to create inf-stats-room");

    printf("Testing start/stop monitoring...\n");
    assert(start_monitoring(cpu_room) == 0 && "Start cpu-room fail");
    assert(stop_monitoring(cpu_room) == 0 && "Stop cpu-room fail");

    assert(start_monitoring(mem_room) == 0 && "Start memory-room fail");
    assert(stop_monitoring(mem_room) == 0 && "Stop memory-room fail");

    assert(start_monitoring(inf_room) == 0 && "Start inf-stats-room fail");
    assert(stop_monitoring(inf_room) == 0 && "Stop inf-stats-room fail");

    printf("Testing delete room...\n");
    assert(delete_room(cpu_room) == 0 && "Delete cpu-room fail");

    printf("All room creation tests passed!\n");
    return 0;
}

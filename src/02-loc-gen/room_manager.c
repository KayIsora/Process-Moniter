#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "room_manager.h"
#include "data_collector.h"

#define MAX_ROOMS 10

typedef struct {
    int id;
    room_type_t type;
    int interval_ms;
    int running;
} room_t;

static room_t rooms[MAX_ROOMS];
static int room_count = 0;

// Chuyển từ chuỗi sang enum
static int str_to_room_type(const char* type_str) {
    if (strcmp(type_str, "cpu-room") == 0) return CPU_ROOM;
    if (strcmp(type_str, "memory-room") == 0) return MEMORY_ROOM;
    if (strcmp(type_str, "inf-stats-room") == 0) return INF_STATS_ROOM;
    return -1;
}

int create_room(const char* room_type_str, int interval) {
    if (room_count >= MAX_ROOMS) {
        printf("Max rooms reached\n");
        return -1;
    }
    int type = str_to_room_type(room_type_str);
    if (type == -1) {
        printf("Invalid room type %s\n", room_type_str);
        return -1;
    }

    rooms[room_count].id = room_count;
    rooms[room_count].type = type;
    rooms[room_count].interval_ms = interval;
    rooms[room_count].running = 0;
    printf("Created room id %d type %s interval %dms\n", room_count, room_type_str, interval);
    return room_count++;
}

int delete_room(int room_id) {
    if (room_id < 0 || room_id >= room_count) {
        printf("Invalid room id\n");
        return -1;
    }
    for (int i = room_id; i < room_count - 1; i++) {
        rooms[i] = rooms[i + 1];
        rooms[i].id = i;
    }
    room_count--;
    printf("Deleted room id %d\n", room_id);
    return 0;
}

int start_monitoring(int room_id) {
    if (room_id < 0 || room_id >= room_count) {
        printf("Invalid room id\n");
        return -1;
    }
    if (rooms[room_id].running) {
        printf("Room id %d already running\n", room_id);
        return -1;
    }
    rooms[room_id].running = 1;
    printf("Started monitoring room id %d\n", room_id);
    return 0;
}

int stop_monitoring(int room_id) {
    if (room_id < 0 || room_id >= room_count) {
        printf("Invalid room id\n");
        return -1;
    }
    if (!rooms[room_id].running) {
        printf("Room id %d is not running\n", room_id);
        return -1;
    }
    rooms[room_id].running = 0;
    printf("Stopped monitoring room id %d\n", room_id);
    return 0;
}

void show_room_data(int room_id) {
    if (room_id < 0 || room_id >= room_count) {
        printf("Invalid room id\n");
        return;
    }
    room_t* room = &rooms[room_id];
    printf("Displaying data for room id %d\n", room->id);
    switch(room->type) {
        case CPU_ROOM:
            read_cpu_stats();
            break;
        case MEMORY_ROOM:
            read_memory_stats();
            break;
        case INF_STATS_ROOM:
            read_io_stats();
            break;
        default:
            printf("Unknown room type\n");
    }
}

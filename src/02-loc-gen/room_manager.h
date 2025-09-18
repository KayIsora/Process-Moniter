#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

typedef enum {
    CPU_ROOM,
    MEMORY_ROOM,
    INF_STATS_ROOM
} room_type_t;

int create_room(const char* room_type_str, int interval);
int delete_room(int room_id);
int start_monitoring(int room_id);
int stop_monitoring(int room_id);
void show_room_data(int room_id);

#endif // ROOM_MANAGER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "room_manager.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./loc_gen <command> [args...]\n");
        return 1;
    }
    const char* cmd = argv[1];

    if (strcmp(cmd, "create") == 0) {
        if (argc != 4) {
            printf("Usage: ./loc_gen create <room_type> <interval_ms>\n");
            return 1;
        }
        const char* room_type = argv[2];
        int interval = atoi(argv[3]);
        create_room(room_type, interval);
    } else if (strcmp(cmd, "delete") == 0) {
        if (argc != 3) {
            printf("Usage: ./loc_gen delete <room_id>\n");
            return 1;
        }
        int id = atoi(argv[2]);
        delete_room(id);
    } else if (strcmp(cmd, "start") == 0) {
        if (argc != 3) {
            printf("Usage: ./loc_gen start <room_id>\n");
            return 1;
        }
        int id = atoi(argv[2]);
        start_monitoring(id);
    } else if (strcmp(cmd, "stop") == 0) {
        if (argc != 3) {
            printf("Usage: ./loc_gen stop <room_id>\n");
            return 1;
        }
        int id = atoi(argv[2]);
        stop_monitoring(id);
    } else if (strcmp(cmd, "show") == 0) {
        if (argc != 3) {
            printf("Usage: ./loc_gen show <room_id>\n");
            return 1;
        }
        int id = atoi(argv[2]);
        show_room_data(id);
    } else {
        printf("Unknown command %s\n", cmd);
        return 1;
    }
    return 0;
}

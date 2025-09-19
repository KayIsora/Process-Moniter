#include "daemon.h"

int handle_command(const char *command, char *response, size_t response_size) {
    if (command == NULL || response == NULL || response_size == 0) {
        return -1;
    }
    
    // Parse command
    char cmd[64], room_name[MAX_ROOM_NAME];
    int room_size;
    int parsed = 0;
    
    // Try different command formats
    if (sscanf(command, "%63s %63s %d", cmd, room_name, &room_size) == 3) {
        parsed = 3;
    } else if (sscanf(command, "%63s %63s", cmd, room_name) == 2) {
        parsed = 2;
    } else if (sscanf(command, "%63s", cmd) == 1) {
        parsed = 1;
    }
    
    if (parsed == 0) {
        snprintf(response, response_size, "ERROR Invalid command format");
        return 0;
    }
    
    log_message(LOG_INFO, "Processing command: %s", cmd);
    
    // Handle commands
    if (strcmp(cmd, "CREATE") == 0) {
        if (parsed < 3) {
            snprintf(response, response_size, "ERROR CREATE requires room name and size");
            return 0;
        }
        return cmd_create_room(room_name, room_size, response, response_size);
    } 
    else if (strcmp(cmd, "START") == 0) {
        if (parsed < 2) {
            snprintf(response, response_size, "ERROR START requires room name");
            return 0;
        }
        return cmd_start_room(room_name, response, response_size);
    }
    else if (strcmp(cmd, "STOP") == 0) {
        if (parsed < 2) {
            snprintf(response, response_size, "ERROR STOP requires room name");
            return 0;
        }
        return cmd_stop_room(room_name, response, response_size);
    }
    else if (strcmp(cmd, "SHOW") == 0) {
        if (parsed < 2) {
            list_rooms(response, response_size);
        } else {
            cmd_show_room(room_name, response, response_size);
        }
        return 0;
    }
    else if (strcmp(cmd, "EXIT") == 0) {
        return cmd_exit_daemon(response, response_size);
    }
    else {
        snprintf(response, response_size, "ERROR Unknown command: %s", cmd);
        return 0;
    }
}

int cmd_create_room(const char *room_name, int room_size, char *response, size_t response_size) {
    if (room_name == NULL || response == NULL) {
        return -1;
    }
    
    // Validate room size
    if (room_size <= 0 || room_size > 10000) {
        snprintf(response, response_size, "ERROR Invalid room size (must be 1-10000)");
        return 0;
    }
    
    pthread_mutex_lock(&rooms_mutex);
    
    // Check if room already exists
    room_t *existing_room = find_room(room_name);
    if (existing_room != NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' already exists", room_name);
        return 0;
    }
    
    // Add new room
    if (add_room(room_name, room_size) < 0) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Failed to create room '%s'", room_name);
        return 0;
    }
    
    pthread_mutex_unlock(&rooms_mutex);
    
    log_message(LOG_INFO, "Created room '%s' with size %d", room_name, room_size);
    snprintf(response, response_size, "OK Room '%s' created with size %d", room_name, room_size);
    return 0;
}

int cmd_start_room(const char *room_name, char *response, size_t response_size) {
    if (room_name == NULL || response == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&rooms_mutex);
    
    room_t *room = find_room(room_name);
    if (room == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' not found", room_name);
        return 0;
    }
    
    if (room->status == ROOM_RUNNING) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' is already running", room_name);
        return 0;
    }
    
    // Start monitoring process (simulated)
    room->status = ROOM_RUNNING;
    room->monitor_pid = getpid();  // In real implementation, this would be a forked process
    
    pthread_mutex_unlock(&rooms_mutex);
    
    log_message(LOG_INFO, "Started monitoring room '%s'", room_name);
    snprintf(response, response_size, "OK Room '%s' monitoring started", room_name);
    return 0;
}

int cmd_stop_room(const char *room_name, char *response, size_t response_size) {
    if (room_name == NULL || response == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&rooms_mutex);
    
    room_t *room = find_room(room_name);
    if (room == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' not found", room_name);
        return 0;
    }
    
    if (room->status != ROOM_RUNNING) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' is not running", room_name);
        return 0;
    }
    
    // Stop monitoring process
    room->status = ROOM_STOPPED;
    if (room->monitor_pid > 0) {
        // In real implementation, would kill the monitoring process
        room->monitor_pid = 0;
    }
    
    pthread_mutex_unlock(&rooms_mutex);
    
    log_message(LOG_INFO, "Stopped monitoring room '%s'", room_name);
    snprintf(response, response_size, "OK Room '%s' monitoring stopped", room_name);
    return 0;
}

int cmd_show_room(const char *room_name, char *response, size_t response_size) {
    if (room_name == NULL || response == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&rooms_mutex);
    
    room_t *room = find_room(room_name);
    if (room == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        snprintf(response, response_size, "ERROR Room '%s' not found", room_name);
        return 0;
    }
    
    snprintf(response, response_size, 
             "OK Room: %s, Size: %d, Status: %s, PID: %d", 
             room->name, room->size, room_status_string(room->status), room->monitor_pid);
    
    pthread_mutex_unlock(&rooms_mutex);
    return 0;
}

int cmd_exit_daemon(char *response, size_t response_size) {
    if (response == NULL) {
        return -1;
    }
    
    log_message(LOG_INFO, "Received EXIT command, shutting down daemon");
    snprintf(response, response_size, "OK Daemon shutting down");
    return 1;  // Special return value to indicate daemon should exit
}

// Room management functions
room_t *find_room(const char *room_name) {
    int i;
    for (i = 0; i < room_count; i++) {
        if (rooms[i].active && strcmp(rooms[i].name, room_name) == 0) {
            return &rooms[i];
        }
    }
    return NULL;
}

int add_room(const char *room_name, int room_size) {
    if (room_count >= MAX_ROOMS) {
        return -1;
    }
    
    strncpy(rooms[room_count].name, room_name, MAX_ROOM_NAME - 1);
    rooms[room_count].name[MAX_ROOM_NAME - 1] = '\0';
    rooms[room_count].size = room_size;
    rooms[room_count].status = ROOM_CREATED;
    rooms[room_count].monitor_pid = 0;
    rooms[room_count].active = 1;
    
    room_count++;
    return 0;
}

int remove_room(const char *room_name) {
    room_t *room = find_room(room_name);
    if (room == NULL) {
        return -1;
    }
    
    room->active = 0;
    return 0;
}

void list_rooms(char *response, size_t response_size) {
    int offset = 0;
    int i;
    
    offset += snprintf(response + offset, response_size - offset, "OK Rooms:\n");
    
    pthread_mutex_lock(&rooms_mutex);
    
    if (room_count == 0) {
        snprintf(response + offset, response_size - offset, "No rooms created");
    } else {
        for (i = 0; i < room_count && offset < (int)response_size - 1; i++) {
            if (rooms[i].active) {
                offset += snprintf(response + offset, response_size - offset,
                                 "  %s: size=%d, status=%s\n",
                                 rooms[i].name, rooms[i].size, 
                                 room_status_string(rooms[i].status));
            }
        }
    }
    
    pthread_mutex_unlock(&rooms_mutex);
}

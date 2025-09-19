#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "main_daemon.h"
#include "logger.h"
#include "ipc_handler.h"

volatile daemon_state_t g_daemon_state = {0};
volatile daemon_stats_t g_daemon_stats = {0};

static room_info_t* find_room(const char *room_name);
static void* room_monitor_thread(void *arg);
static int initialize_server_socket(void);
static int create_pid_file(const char *pid_file);
static int remove_pid_file(const char *pid_file);
static void cleanup_on_exit(void);
static void signal_handler(int sig);
static int install_signal_handlers(void);

int load_configuration(const char *config_file) {
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        log_warn("Config file not found, using defaults");
        strncpy(g_daemon_state.config.log_path, DEFAULT_LOG_FILE, sizeof(g_daemon_state.config.log_path));
        g_daemon_state.config.log_level = LOG_INFO;
        g_daemon_state.config.structured_logging = 1;
        g_daemon_state.config.daemon_port = DEFAULT_PORT;
        g_daemon_state.config.max_rooms = DEFAULT_MAX_ROOMS;
        g daemon_state.config.max_clients = DEFAULT_MAX_CLIENTS;
        g_daemon_state.config.collection_interval = DEFAULT_COLLECTION_INTERVAL;
        strncpy(g_daemon_state.config.fifo_path, DEFAULT_FIFO_PATH, sizeof(g_daemon_state.config.fifo_path));
        strncpy(g_daemon_state.config.procfs_path, DEFAULT_PROCFS_PATH, sizeof(g_daemon_state.config.procfs_path));
        strncpy(g_daemon_state.config.pid_file, DEFAULT_PID_FILE, sizeof(g_daemon_state.config.pid_file));
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char key[128], value[256];
        if (sscanf(line, " %127[^=] = %255[^\"]", key, value) == 2) {
            char *k = key; while (*k == ' ' || *k == '\t') k++;
            char *v = value; while (*v == ' ' || *v == '\t') v++;
            if (strcasecmp(k, "log_path") == 0) {
                strncpy(g_daemon_state.config.log_path, v, sizeof(g_daemon_state.config.log_path));
            } else if (strcasecmp(k, "log_level") == 0) {
                g_daemon_state.config.log_level = log_level_from_string(v);
            } else if (strcasecmp(k, "structured_logging") == 0) {
                g_daemon_state.config.structured_logging = (strcasecmp(v, "true") == 0) ? 1 : 0;
            } else if (strcasecmp(k, "daemon_port") == 0) {
                g_daemon_state.config.daemon_port = atoi(v);
            } else if (strcasecmp(k, "max_rooms") == 0) {
                g_daemon_state.config.max_rooms = atoi(v);
            } else if (strcasecmp(k, "max_clients") == 0) {
                g_daemon_state.config.max_clients = atoi(v);
            } else if (strcasecmp(k, "collection_interval") == 0) {
                g_daemon_state.config.collection_interval = atoi(v);
            } else if (strcasecmp(k, "fifo_path") == 0) {
                strncpy(g_daemon_state.config.fifo_path, v, sizeof(g_daemon_state.config.fifo_path));
            } else if (strcasecmp(k, "procfs_path") == 0) {
                strncpy(g_daemon_state.config.procfs_path, v, sizeof(g_daemon_state.config.procfs_path));
            } else if (strcasecmp(k, "pid_file") == 0) {
                strncpy(g_daemon_state.config.pid_file, v, sizeof(g_daemon_state.config.pid_file));
            }
        }
    }

    fclose(fp);
    log_info("Configuration loaded from %s", config_file);
    return 0;
}

static room_info_t* find_room(const char *room_name) {
    if (!room_name) return NULL;
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    for (int i=0; i<g_daemon_state.config.max_rooms; i++) {
        if (g_daemon_state.rooms[i].state != ROOM_STATE_INACTIVE &&
            strncmp(g_daemon_state.rooms[i].name, room_name, MAX_ROOM_NAME) == 0) {
            pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
            return &g_daemon_state.rooms[i];
        }
    }
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
    return NULL;
}

void* room_monitor_thread(void* arg) {
    room_info_t* room = (room_info_t*)arg;
    log_info("Starting monitoring thread for room %s", room->name);
    while (g_daemon_state.running && room->active) {
        monitor_data_t data = {0};
        if (ipc_read_kernel_data(room->name, &data) == 0) {
            pthread_mutex_lock(&g_daemon_state.rooms_mutex);
            room->latest_data = data;
            room->last_update = time(NULL);
            g_daemon_stats.data_points_collected++;
            pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
            log_debug("Collected data for room %s: CPU=%.2f%% MEM=%.2f%% PROC=%d",
            room->name, data.cpu_usage, data.memory_usage, data.process_count);
        } else {
            log_warn("Failed to collect data for room %s", room->name);
            room->error_count++;
            if (room->error_count > 10) {
                log_error("Too many errors for room %s, stopping monitoring", room->name);
                room->state = ROOM_STATE_ERROR;
                break;
            }
        }
        sleep(room->collection_interval);
    }
    log_info("Monitoring thread stopped for room %s", room->name);
    return NULL;
}

int create_room(const char *room_name, int collection_interval) {
    if (!room_name) return -1;
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    for(int i=0;i<g_daemon_state.config.max_rooms;i++) {
        if (g_daemon_state.rooms[i].state != ROOM_STATE_INACTIVE &&
            strncmp(g_daemon_state.rooms[i].name, room_name, MAX_ROOM_NAME) == 0) {
            pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
            return -1;
        }
    }
    int slot = -1;
    for (int i=0; i<g_daemon_state.config.max_rooms; i++) {
        if (g_daemon_state.rooms[i].state == ROOM_STATE_INACTIVE) {
            slot = i;
            break;
        }
    }
    if(slot == -1) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return -1;
    }
    strncpy(g_daemon_state.rooms[slot].name, room_name, MAX_ROOM_NAME);
    g_daemon_state.rooms[slot].state = ROOM_STATE_CREATED;
    g_daemon_state.rooms[slot].created_time = time(NULL);
    g_daemon_state.rooms[slot].collection_interval = collection_interval > 0 ? collection_interval : g_daemon_state.config.collection_interval;
    g_daemon_state.rooms[slot].active = 0;
    g_daemon_state.rooms[slot].error_count = 0;
    g_daemon_state.room_count++;
    g_daemon_stats.rooms_created++;
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
    log_info("Created new room %s", room_name);
    return 0;
}

int start_room(const char *room_name) {
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    room_info_t *room = find_room(room_name);
    if (!room) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return -1;
    }
    if (room->state == ROOM_STATE_RUNNING) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return 0;
    }
    room->active = 1;
    room->state = ROOM_STATE_RUNNING;
    if (pthread_create(&room->monitor_thread, NULL, room_monitor_thread, room) != 0) {
        log_error("Cannot start monitor thread for room %s", room_name);
        room->state = ROOM_STATE_ERROR;
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return -1;
    }
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
    ipc_write_kernel_control(room_name, 1);
    log_info("Started room %s", room_name);
    return 0;
}

int stop_room(const char *room_name) {
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    room_info_t *room = find_room(room_name);
    if (!room) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return -1;
    }
    if(room->state != ROOM_STATE_RUNNING) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return 0;
    }
    room->active = 0;
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
    pthread_join(room->monitor_thread, NULL);
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    room->state = ROOM_STATE_CREATED;
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
    ipc_write_kernel_control(room_name, 0);
    log_info("Stopped room %s", room_name);
    return 0;
}

int delete_room(const char *room_name) {
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    room_info_t *room = find_room(room_name);
    if (!room) {
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        return -1;
    }
    if(room->state == ROOM_STATE_RUNNING) {
        room->active = 0;
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        pthread_join(room->monitor_thread, NULL);
        pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    }
    memset(room, 0, sizeof(room_info_t));
    room->state = ROOM_STATE_INACTIVE;
    g_daemon_state.room_count--;
    g_daemon_stats.rooms_deleted++;
    pthread_mutex_unlock(&g daemon_state.rooms_mutex);
    log_info("Deleted room %s", room_name);
    return 0;
}
int process_command(const command_t *command, response_t *response) {
    if (!command || !response) return -1;

    memset(response, 0, sizeof(response_t));
    response->timestamp = time(NULL);

    switch (command->type) {
    case CMD_CREATE_ROOM:
        if (create_room(command->room_name, command->param1) == 0) {
            response->type = RESP_SUCCESS;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' created", command->room_name);
        } else {
            response->type = RESP_ERROR;
            snprintf(response->message, sizeof(response->message), 
                    "Failed to create room '%s'", command->room_name);
        }
        break;
    case CMD_START_ROOM:
        if (start_room(command->room_name) == 0) {
            response->type = RESP_SUCCESS;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' started", command->room_name);
        } else {
            response->type = RESP_ERROR;
            snprintf(response->message, sizeof(response->message), 
                    "Failed to start room '%s'", command->room_name);
        }
        break;
    case CMD_STOP_ROOM:
        if (stop_room(command->room_name) == 0) {
            response->type = RESP_SUCCESS;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' stopped", command->room_name);
        } else {
            response->type = RESP_ERROR;
            snprintf(response->message, sizeof(response->message), 
                    "Failed to stop room '%s'", command->room_name);
        }
        break;
    case CMD_DELETE_ROOM:
        if (delete_room(command->room_name) == 0) {
            response->type = RESP_SUCCESS;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' deleted", command->room_name);
        } else {
            response->type = RESP_ERROR;
            snprintf(response->message, sizeof(response->message), 
                    "Failed to delete room '%s'", command->room_name);
        }
        break;
    case CMD_SHOW_ROOM: {
        pthread_mutex_lock(&g_daemon_state.rooms_mutex);
        room_info_t *room = find_room(command->room_name);
        if (room && room->state != ROOM_STATE_INACTIVE) {
            response->type = RESP_SUCCESS;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' data", command->room_name);
            snprintf(response->data, sizeof(response->data),
                    "CPU: %.2f%%, Memory: %.2f%%, Processes: %d",
                    room->latest_data.cpu_usage, room->latest_data.memory_usage,
                    room->latest_data.process_count);
        } else {
            response->type = RESP_ROOM_NOT_FOUND;
            snprintf(response->message, sizeof(response->message), 
                    "Room '%s' not found", command->room_name);
        }
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        break;
    }
    case CMD_LIST_ROOMS: {
        pthread_mutex_lock(&g_daemon_state.rooms_mutex);
        char room_list[MAX_MESSAGE_SIZE] = "";
        for (int i = 0; i < g_daemon_state.config.max_rooms; i++) {
            if (g_daemon_state.rooms[i].state != ROOM_STATE_INACTIVE) {
                char tmp[128];
                snprintf(tmp, sizeof(tmp), "%s (%s), ",
                        g_daemon_state.rooms[i].name,
                        g_daemon_state.rooms[i].state == ROOM_STATE_RUNNING ? "running" : "stopped");
                strncat(room_list, tmp, sizeof(room_list) - strlen(room_list) - 1);
            }
        }
        pthread_mutex_unlock(&g_daemon_state.rooms_mutex);
        strncpy(response->data, room_list, sizeof(response->data));
        response->type = RESP_SUCCESS;
        snprintf(response->message, sizeof(response->message), "Active rooms");
        break;
    }
    case CMD_STATUS:
        response->type = RESP_SUCCESS;
        snprintf(response->message, sizeof(response->message), "Daemon status");
        snprintf(response->data, sizeof(response->data),
                "Uptime: %ld seconds, Rooms: %d, Commands processed: %lu",
                time(NULL) - g_daemon_state.start_time,
                g_daemon_state.room_count,
                g_daemon_stats.commands_processed);
        break;
    default:
        response->type = RESP_INVALID_COMMAND;
        snprintf(response->message, sizeof(response->message), "Invalid command");
        break;
    }
    g_daemon_stats.commands_processed++;
    return 0;
}

int receive_command_from_client(int client_socket, command_t *command) {
    char buffer[1024];
    ssize_t received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return -1;
    buffer[received] = '\0';
    char cmd_str[64];
    if (sscanf(buffer, "%63s %63s %d %d", cmd_str, command->room_name, &command->param1, &command->param2) >= 1) {
        if (strcmp(cmd_str, "create") == 0) command->type = CMD_CREATE_ROOM;
        else if (strcmp(cmd_str, "start") == 0) command->type = CMD_START_ROOM;
        else if (strcmp(cmd_str, "stop") == 0) command->type = CMD_STOP_ROOM;
        else if (strcmp(cmd_str, "delete") == 0) command->type = CMD_DELETE_ROOM;
        else if (strcmp(cmd_str, "show") == 0) command->type = CMD_SHOW_ROOM;
        else if (strcmp(cmd_str, "list") == 0) command->type = CMD_LIST_ROOMS;
        else if (strcmp(cmd_str, "status") == 0) command->type = CMD_STATUS;
        else return -1;
        command->timestamp = time(NULL);
        return 0;
    }
    return -1;
}

int send_response_to_client(int client_socket, const response_t *response) {
    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer), "%s: %s",
                      (response->type == RESP_SUCCESS) ? "SUCCESS" : "ERROR",
                      response->message);
    if (response->data[0] != '\0') {
        len += snprintf(buffer + len, sizeof(buffer) - len, "\nData: %s", response->data);
    }
    len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
    return (send(client_socket, buffer, len, MSG_NOSIGNAL) == len) ? 0 : -1;
}

void* client_handler_thread(void *arg) {
    client_connection_t *client = (client_connection_t*)arg;
    log_info("Client connected: %s:%d",
             inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
    while (g_daemon_state.running && client->active) {
        command_t command;
        response_t response;
        if (receive_command_from_client(client->socket_fd, &command) != 0) break;
        client->last_activity = time(NULL);
        process_command(&command, &response);
        send_response_to_client(client->socket_fd, &response);
    }
    log_info("Client disconnected: %s:%d",
             inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
    close(client->socket_fd);
    client->active = 0;
    return NULL;
}

void* network_thread(void *arg) {
    (void)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    log_info("Starting network thread on port %d", g_daemon_state.config.daemon_port);
    while (g_daemon_state.running) {
        int client_socket = accept(g_daemon_state.server_socket,
                                  (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (g_daemon_state.running)
                log_error("Accept failed: %s", strerror(errno));
            continue;
        }
        pthread_mutex_lock(&g_daemon_state.clients_mutex);
        int client_slot = -1;
        for (int i = 0; i < g_daemon_state.config.max_clients; i++) {
            if (!g_daemon_state.clients[i].active) {
                client_slot = i;
                break;
            }
        }
        if (client_slot == -1) {
            pthread_mutex_unlock(&g_daemon_state.clients_mutex);
            log_warn("Max clients reached, rejecting connection");
            close(client_socket);
            continue;
        }
        g_daemon_state.clients[client_slot].socket_fd = client_socket;
        g_daemon_state.clients[client_slot].address = client_addr;
        g_daemon_state.clients[client_slot].connect_time = time(NULL);
        g_daemon_state.clients[client_slot].last_activity = time(NULL);
        g_daemon_state.clients[client_slot].authenticated = 1;
        g_daemon_state.clients[client_slot].active = 1;
        if (pthread_create(&g_daemon_state.clients[client_slot].thread,
                          NULL, client_handler_thread, &g_daemon_state.clients[client_slot]) != 0) {
            log_error("Failed to create client handler thread");
            close(client_socket);
            g_daemon_state.clients[client_slot].active = 0;
        } else {
            g_daemon_state.client_count++;
        }
        pthread_mutex_unlock(&g_daemon_state.clients_mutex);
    }
    log_info("Network thread stopped");
    return NULL;
}

static int initialize_server_socket(void) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_error("Failed to create socket: %s", strerror(errno));
        return -1;
    }
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(g_daemon_state.config.daemon_port);
    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_error("Bind failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }
    if (listen(server_socket, 10) < 0) {
        log_error("Listen failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }
    log_info("Server socket created on port %d", g_daemon_state.config.daemon_port);
    return server_socket;
}

static void signal_handler(int sig) {
    log_info("Received signal %d, stopping daemon", sig);
    g_daemon_state.running = 0;
}

static int install_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    return 0;
}

static int create_pid_file(const char *pid_file) {
    FILE *fp = fopen(pid_file, "w");
    if (!fp) return -1;
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    return 0;
}

static int remove_pid_file(const char *pid_file) {
    return unlink(pid_file);
}

static void cleanup_on_exit(void) {
    log_info("Cleaning up before exit");
    g_daemon_state.running = 0;

    // Stop all room threads
    pthread_mutex_lock(&g_daemon_state.rooms_mutex);
    for (int i = 0; i < g_daemon_state.config.max_rooms; i++) {
        if (g_daemon_state.rooms[i].active) {
            g_daemon_state.rooms[i].active = 0;
            pthread_join(g_daemon_state.rooms[i].monitor_thread, NULL);
        }
    }
    pthread_mutex_unlock(&g_daemon_state.rooms_mutex);

    // Close server socket
    if (g_daemon_state.server_socket >= 0)
        close(g_daemon_state.server_socket);

    // Clean up client connections
    pthread_mutex_lock(&g_daemon_state.clients_mutex);
    for (int i = 0; i < g_daemon_state.config.max_clients; i++) {
        if (g_daemon_state.clients[i].active) {
            close(g_daemon_state.clients[i].socket_fd);
            g_daemon_state.clients[i].active = 0;
            pthread_join(g_daemon_state.clients[i].thread, NULL);
        }
    }
    pthread_mutex_unlock(&g_daemon_state.clients_mutex);

    // Clean up IPC and logger
    ipc_cleanup();
    remove_pid_file(g_daemon_state.config.pid_file);
    logger_cleanup();
}

int main(int argc, char *argv[]) {
    const char *config_file = DEFAULT_CONFIG_FILE;
    int daemon_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else if (strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = 1;
        }
    }

    // Load configuration
    if (load_configuration(config_file) != 0) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }

    // Initialize logging
    if (logger_init(g_daemon_state.config.log_path,
                   g_daemon_state.config.log_level,
                   g_daemon_state.config.structured_logging) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    log_info("=== Process Monitor Integration Daemon Starting ===");
    log_info("PID: %d", getpid());

    // Initialize global state
    g_daemon_state.running = 1;
    g_daemon_state.start_time = time(NULL);
    pthread_mutex_init(&g_daemon_state.rooms_mutex, NULL);
    pthread_mutex_init(&g_daemon_state.clients_mutex, NULL);

    // Install signal handlers
    install_signal_handlers();

    // Initialize IPC system
    if (ipc_init() != 0) {
        log_error("Failed to initialize IPC");
        return 1;
    }

    // Initialize server socket
    g_daemon_state.server_socket = initialize_server_socket();
    if (g_daemon_state.server_socket < 0) {
        log_error("Failed to create server socket");
        return 1;
    }

    // Create PID file
    create_pid_file(g_daemon_state.config.pid_file);

    // Start network thread
    if (pthread_create(&g_daemon_state.network_thread, NULL, network_thread, NULL) != 0) {
        log_error("Failed to create network thread");
        return 1;
    }

    // Main loop
    while (g_daemon_state.running) {
        sleep(10);
        log_debug("Daemon uptime: %ld seconds, rooms: %d, clients: %d, commands: %lu",
                  time(NULL) - g_daemon_state.start_time,
                  g_daemon_state.room_count,
                  g_daemon_state.client_count,
                  g_daemon_stats.commands_processed);
    }

    // Cleanup on exit
    cleanup_on_exit();

    return 0;
}

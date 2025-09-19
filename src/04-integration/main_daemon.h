#ifndef MAIN_DAEMON_H
#define MAIN_DAEMON_H

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../commom/data_structures.h"
#include "logger.h"
#include "ipc_handler.h"

// Daemon configuration defaults
#define DEFAULT_CONFIG_FILE "config/monitor.conf"
#define DEFAULT_PID_FILE "/tmp/monitor_daemon.pid"
#define DEFAULT_LOG_FILE "logs/monitor.log"
#define DEFAULT_PORT 8080
#define DEFAULT_MAX_ROOMS 10
#define DEFAULT_MAX_CLIENTS 50
#define DEFAULT_COLLECTION_INTERVAL 5

// Thread types
typedef enum {
    THREAD_TYPE_MAIN = 0,
    THREAD_TYPE_NETWORK = 1,
    THREAD_TYPE_MONITOR = 2,
    THREAD_TYPE_CLIENT = 3
} thread_type_t;

// Thread context
typedef struct {
    thread_type_t type;
    int id;
    pthread_t thread;
    int active;
    void *context_data;
    time_t start_time;
} thread_context_t;

// External global daemon state
extern daemon_state_t g_daemon_state;
extern daemon_stats_t g_daemon_stats;

// Configuration functions
int load_configuration(const char *config_file);
int reload_configuration(void);
int validate_configuration(const config_t *config);
void print_configuration(void);

// Room management functions
int create_room(const char *room_name, int collection_interval);
int start_room(const char *room_name);
int stop_room(const char *room_name);
int delete_room(const char *room_name);
room_info_t* find_room(const char *room_name);
int get_room_data(const char *room_name, monitor_data_t *data);
int list_rooms(char *buffer, size_t buffer_size);

// Client management functions
int accept_client_connection(int server_socket);
void* client_handler_thread(void *arg);
int send_response_to_client(int client_socket, const response_t *response);
int receive_command_from_client(int client_socket, command_t *command);
void disconnect_client(int client_id);

// Command processing functions
int process_command(const command_t *command, response_t *response);
int handle_create_room_command(const command_t *command, response_t *response);
int handle_start_room_command(const command_t *command, response_t *response);
int handle_stop_room_command(const command_t *command, response_t *response);
int handle_delete_room_command(const command_t *command, response_t *response);
int handle_show_room_command(const command_t *command, response_t *response);
int handle_list_rooms_command(const command_t *command, response_t *response);
int handle_status_command(const command_t *command, response_t *response);

// Network functions
int initialize_server_socket(void);
void* network_thread(void *arg);
int setup_socket_options(int socket_fd);
int bind_and_listen(int socket_fd, int port);

// Monitor thread functions
void* room_monitor_thread(void *arg);
int collect_room_data(room_info_t *room);
int update_room_statistics(room_info_t *room, const monitor_data_t *data);

// Signal handling
void signal_handler(int sig);
int install_signal_handlers(void);
void cleanup_on_exit(void);

// Statistics functions
void update_daemon_stats(const char *operation, double duration);
void reset_daemon_stats(void);
void get_daemon_statistics(daemon_stats_t *stats);
void log_periodic_statistics(void);

// Utility functions for daemon
int daemonize_process(void);
int create_pid_file(const char *pid_file);
int remove_pid_file(const char *pid_file);
int check_running_instance(const char *pid_file);
void print_daemon_status(void);

// Health check functions
int daemon_health_check(void);
int check_system_resources(void);
int check_component_status(void);

#endif /* MAIN_DAEMON_H */

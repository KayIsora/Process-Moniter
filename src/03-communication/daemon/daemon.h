#ifndef DAEMON_H
#define DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>

// Constants
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define MAX_ROOMS 50
#define MAX_ROOM_NAME 64

// Room status
typedef enum {
    ROOM_CREATED,
    ROOM_RUNNING,
    ROOM_STOPPED
} room_status_t;

// Room structure
typedef struct {
    char name[MAX_ROOM_NAME];
    int size;
    room_status_t status;
    pid_t monitor_pid;  // Process ID of the monitoring process
    int active;         // 1 if room exists, 0 if deleted
} room_t;

// Client connection structure
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    pthread_t thread_id;
} client_conn_t;

// Global variables
extern room_t rooms[MAX_ROOMS];
extern int room_count;
extern pthread_mutex_t rooms_mutex;
extern volatile int daemon_running;

// Function prototypes
// Daemon functions
int daemonize(void);
void signal_handler(int signum);
void cleanup_daemon(void);

// Socket server functions
int server_init(void);
int server_listen(int server_fd);
void *handle_client(void *arg);

// Command handler functions
int handle_command(const char *command, char *response, size_t response_size);
int cmd_create_room(const char *room_name, int room_size, char *response, size_t response_size);
int cmd_start_room(const char *room_name, char *response, size_t response_size);
int cmd_stop_room(const char *room_name, char *response, size_t response_size);
int cmd_show_room(const char *room_name, char *response, size_t response_size);
int cmd_exit_daemon(char *response, size_t response_size);

// Room management functions
room_t *find_room(const char *room_name);
int add_room(const char *room_name, int room_size);
int remove_room(const char *room_name);
void list_rooms(char *response, size_t response_size);

// Utility functions
void log_message(int priority, const char *format, ...);
const char *room_status_string(room_status_t status);

#endif // DAEMON_H

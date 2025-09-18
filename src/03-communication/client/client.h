#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

// Constants
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 10

// Command types
typedef enum {
    CMD_HELP,
    CMD_CREATE,
    CMD_START,
    CMD_STOP,
    CMD_SHOW,
    CMD_EXIT,
    CMD_UNKNOWN
} command_type_t;

// Command structure
typedef struct {
    command_type_t type;
    char room_name;
    int room_size;
    char args[MAX_ARGS];
    int arg_count;
} command_t;

// Response structure
typedef struct {
    int status;     // 0 = OK, -1 = ERROR
    char message[BUFFER_SIZE];
} response_t;

// Function prototypes
// Command parsing functions
command_type_t parse_command(int argc, char *argv[], command_t *cmd);
void print_usage(void);
int validate_command(const command_t *cmd);

// Socket client functions
int client_connect(void);
int client_send_command(int sockfd, const command_t *cmd);
int client_receive_response(int sockfd, response_t *resp);
void client_disconnect(int sockfd);

// Utility functions
void print_error(const char *msg);
void print_response(const response_t *resp);

#endif // CLIENT_H

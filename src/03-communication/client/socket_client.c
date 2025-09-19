#include "client.h"

int client_connect(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sockfd);
        return -1;
    }
    
    // Connect to server  
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to daemon failed");
        close(sockfd);
        return -1;
    }
    
    printf("Connected to daemon at %s:%d\n", SERVER_IP, SERVER_PORT);
    return sockfd;
}

int client_send_command(int sockfd, const command_t *cmd) {
    if (sockfd < 0 || cmd == NULL) {
        return -1;
    }
    
    // Create command string based on command type
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    switch (cmd->type) {
        case CMD_CREATE:
            snprintf(buffer, sizeof(buffer), "CREATE %s %d", 
                    cmd->room_name, cmd->room_size);
            break;
        case CMD_START:
            snprintf(buffer, sizeof(buffer), "START %s", cmd->room_name);
            break;
        case CMD_STOP:
            snprintf(buffer, sizeof(buffer), "STOP %s", cmd->room_name);
            break;
        case CMD_SHOW:
            snprintf(buffer, sizeof(buffer), "SHOW %s", cmd->room_name);
            break;
        case CMD_EXIT:
            snprintf(buffer, sizeof(buffer), "EXIT");
            break;
        default:
            fprintf(stderr, "Unknown command type\n");
            return -1;
    }
    
    // Send command
    ssize_t bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
    if (bytes_sent < 0) {
        perror("Failed to send command");
        return -1;
    }
    
    printf("Sent command: %s\n", buffer);
    return 0;
}

int client_receive_response(int sockfd, response_t *resp) {
    if (sockfd < 0 || resp == NULL) {
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    memset(resp, 0, sizeof(response_t));
    
    // Receive response
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to receive response");
        return -1;
    }
    
    if (bytes_received == 0) {
        fprintf(stderr, "Server closed connection\n");
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    printf("Received response: %s\n", buffer);
    
    // Parse response: format is "OK message" or "ERROR message"
    if (strncmp(buffer, "OK", 2) == 0) {
        resp->status = 0;
        if (strlen(buffer) > 3) {
            strncpy(resp->message, buffer + 3, sizeof(resp->message) - 1);
        } else {
            strcpy(resp->message, "Command executed successfully");
        }
    } else if (strncmp(buffer, "ERROR", 5) == 0) {
        resp->status = -1;
        if (strlen(buffer) > 6) {
            strncpy(resp->message, buffer + 6, sizeof(resp->message) - 1);
        } else {
            strcpy(resp->message, "Unknown error occurred");
        }
    } else {
        resp->status = -1;
        strncpy(resp->message, buffer, sizeof(resp->message) - 1);
    }
    
    resp->message[sizeof(resp->message) - 1] = '\0';
    return 0;
}

void client_disconnect(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
        printf("Disconnected from daemon\n");
    }
}

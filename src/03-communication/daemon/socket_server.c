#include "daemon.h"

int server_init(void) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_message(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }
    
    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message(LOG_WARNING, "Failed to set SO_REUSEADDR: %s", strerror(errno));
    }
    
    // Set socket options to reuse port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message(LOG_WARNING, "Failed to set SO_REUSEPORT: %s", strerror(errno));
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_message(LOG_ERR, "Socket bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    // Start listening
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        log_message(LOG_ERR, "Socket listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    
    log_message(LOG_INFO, "Server socket initialized successfully");
    return server_fd;
}

int server_listen(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_fd;
    pthread_t client_thread;
    client_conn_t *client_conn;
    
    while (daemon_running) {
        client_addr_len = sizeof(client_addr);
        
        // Accept client connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (client_fd < 0) {
            if (errno == EINTR) {
                // Interrupted system call, continue if daemon is still running
                continue;
            }
            log_message(LOG_ERR, "Accept failed: %s", strerror(errno));
            break;
        }
        
        // Log client connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        log_message(LOG_INFO, "Client connected from %s:%d", 
                   client_ip, ntohs(client_addr.sin_port));
        
        // Create client connection structure
        client_conn = malloc(sizeof(client_conn_t));
        if (client_conn == NULL) {
            log_message(LOG_ERR, "Failed to allocate memory for client connection");
            close(client_fd);
            continue;
        }
        
        client_conn->sockfd = client_fd;
        client_conn->addr = client_addr;
        
        // Create thread to handle client
        if (pthread_create(&client_thread, NULL, handle_client, client_conn) != 0) {
            log_message(LOG_ERR, "Failed to create client thread: %s", strerror(errno));
            close(client_fd);
            free(client_conn);
            continue;
        }
        
        // Detach thread so it cleans up automatically
        pthread_detach(client_thread);
    }
    
    log_message(LOG_INFO, "Server stopped listening");
    return 0;
}

void *handle_client(void *arg) {
    client_conn_t *client_conn = (client_conn_t *)arg;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t bytes_received;
    
    if (client_conn == NULL) {
        pthread_exit(NULL);
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_conn->addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    while (daemon_running) {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));
        
        // Receive command from client
        bytes_received = recv(client_conn->sockfd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                log_message(LOG_INFO, "Client %s disconnected", client_ip);
            } else {
                log_message(LOG_WARNING, "recv() failed for client %s: %s", 
                           client_ip, strerror(errno));
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        log_message(LOG_INFO, "Received command from %s: %s", client_ip, buffer);
        
        // Handle the command
        int result = handle_command(buffer, response, sizeof(response));
        
        // Send response back to client
        ssize_t bytes_sent = send(client_conn->sockfd, response, strlen(response), 0);
        if (bytes_sent < 0) {
            log_message(LOG_WARNING, "Failed to send response to client %s: %s", 
                       client_ip, strerror(errno));
            break;
        }
        
        log_message(LOG_INFO, "Sent response to %s: %s", client_ip, response);
        
        // If it was an EXIT command, stop the daemon
        if (result == 1) {
            daemon_running = 0;
            break;
        }
    }
    
    // Clean up client connection
    close(client_conn->sockfd);
    free(client_conn);
    log_message(LOG_INFO, "Client handler thread for %s terminated", client_ip);
    
    pthread_exit(NULL);
}

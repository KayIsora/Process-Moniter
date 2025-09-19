#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int test_basic_connection() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    printf("Testing basic socket connection to %s:%d\n", SERVER_IP, SERVER_PORT);
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sockfd);
        return -1;
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }
    
    printf("✓ Connected successfully\n");
    
    // Send a simple command
    const char *test_command = "SHOW";
    if (send(sockfd, test_command, strlen(test_command), 0) < 0) {
        perror("Send failed");
        close(sockfd);
        return -1;
    }
    
    printf("✓ Sent command: %s\n", test_command);
    
    // Receive response
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(sockfd);
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    printf("✓ Received response: %s\n", buffer);
    
    close(sockfd);
    printf("✓ Connection closed\n");
    
    return 0;
}

int test_multiple_commands() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *commands[] = {
        "CREATE test-room 1000",
        "START test-room", 
        "SHOW test-room",
        "STOP test-room"
    };
    int num_commands = sizeof(commands) / sizeof(commands[0]);
    int i;
    
    printf("\nTesting multiple commands...\n");
    
    for (i = 0; i < num_commands; i++) {
        // Create new connection for each command
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            return -1;
        }
        
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            close(sockfd);
            return -1;
        }
        
        // Send command
        if (send(sockfd, commands[i], strlen(commands[i]), 0) < 0) {
            perror("Send failed");
            close(sockfd);
            return -1;
        }
        
        printf("Sent: %s\n", commands[i]);
        
        // Receive response
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received: %s\n", buffer);
        }
        
        close(sockfd);
        sleep(100000);  // 100ms delay between commands
    }
    
    printf("✓ Multiple commands test completed\n");
    return 0;
}

int main() {
    printf("=== Basic Socket Test for Process Monitor Daemon ===\n");
    
    if (test_basic_connection() < 0) {
        printf("✗ Basic connection test failed\n");
        return EXIT_FAILURE;
    }
    
    if (test_multiple_commands() < 0) {
        printf("✗ Multiple commands test failed\n");
        return EXIT_FAILURE;
    }
    
    printf("\n=== All socket tests passed! ===\n");
    return EXIT_SUCCESS;
}

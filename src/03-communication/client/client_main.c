#include "client.h"
#include <errno.h>
#include <cerrno>

int main(int argc, char *argv[]) {
    command_t cmd;
    response_t response;
    int sockfd = -1;
    int result = 0;
    
    // Parse command line arguments
    if (parse_command(argc, argv, &cmd) == CMD_UNKNOWN) {
        print_usage();
        return EXIT_FAILURE;
    }
    
    // Handle help command locally
    if (cmd.type == CMD_HELP) {
        print_usage();
        return EXIT_SUCCESS;
    }
    
    // Validate command
    if (validate_command(&cmd) != 0) {
        print_error("Invalid command arguments");
        print_usage();
        return EXIT_FAILURE;
    }
    
    // Connect to daemon
    sockfd = client_connect();
    if (sockfd < 0) {
        print_error("Failed to connect to daemon");
        return EXIT_FAILURE;
    }
    
    // Send command to daemon
    if (client_send_command(sockfd, &cmd) < 0) {
        print_error("Failed to send command");
        result = EXIT_FAILURE;
        goto cleanup;
    }
    
    // Receive response from daemon
    if (client_receive_response(sockfd, &response) < 0) {
        print_error("Failed to receive response");
        result = EXIT_FAILURE;
        goto cleanup;
    }
    
    // Print response
    print_response(&response);
    
    // Set exit code based on response status
    result = (response.status == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    
cleanup:
    if (sockfd >= 0) {
        client_disconnect(sockfd);
    }
    
    return result;
}

void print_usage(void) {
    printf("Usage: ./client <command> [arguments]\\n");
    printf("\\nCommands:\\n");
    printf("  help                     Show this help message\\n");
    printf("  create <room_name> <size> Create a new room with specified size\\n");
    printf("  start <room_name>        Start monitoring the specified room\\n");
    printf("  stop <room_name>         Stop monitoring the specified room\\n");
    printf("  show <room_name>         Show status of the specified room\\n");
    printf("  exit                     Shutdown the daemon\\n");
    printf("\\nExamples:\\n");
    printf("  ./client create cpu-room 1000\\n");
    printf("  ./client start cpu-room\\n");
    printf("  ./client show cpu-room\\n");
    printf("  ./client stop cpu-room\\n");
}

void print_error(const char *msg) {
    fprintf(stderr, "Error: %s\\n", msg);
    if (errno != 0) {
        perror("System error");
        errno = 0;  // Reset errno
    }
}

void print_response(const response_t *resp) {
    if (resp->status == 0) {
        printf("OK: %s\\n", resp->message);
    } else {
        printf("ERROR: %s\\n", resp->message);
    }
}

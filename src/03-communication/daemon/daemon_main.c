#include "daemon.h"

// Global variables
room_t rooms[MAX_ROOMS];
int room_count = 0;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int daemon_running = 1;

int main(int argc, char *argv[]) {
    int server_fd;
    
    // Initialize syslog
    openlog("vts-daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    log_message(LOG_INFO, "VTS Daemon starting...");
    
    // Set up signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe signals
    
    // Daemonize the process (unless -f flag is provided for foreground)
    if (argc < 2 || strcmp(argv[1], "-f") != 0) {
        if (daemonize() < 0) {
            fprintf(stderr, "Failed to daemonize process\n");
            return EXIT_FAILURE;
        }
    }
    
    // Initialize room management
    memset(rooms, 0, sizeof(rooms));
    
    // Initialize server socket
    server_fd = server_init();
    if (server_fd < 0) {
        log_message(LOG_ERR, "Failed to initialize server socket");
        return EXIT_FAILURE;
    }
    
    log_message(LOG_INFO, "VTS Daemon listening on port %d", SERVER_PORT);
    
    // Main server loop
    if (server_listen(server_fd) < 0) {
        log_message(LOG_ERR, "Server listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }
    
    // Cleanup
    close(server_fd);
    cleanup_daemon();
    log_message(LOG_INFO, "VTS Daemon shutting down");
    closelog();
    
    return EXIT_SUCCESS;
}

int daemonize(void) {
    pid_t pid;
    
    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        return -1;
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Change the file mode mask
    umask(0);
    
    // Create a new SID for the child process
    if (setsid() < 0) {
        return -1;
    }
    
    // Change the current working directory
    if (chdir("/") < 0) {
        return -1;
    }
    
    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect stdin, stdout, stderr to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    
    return 0;
}

void signal_handler(int signum) {
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            log_message(LOG_INFO, "Received signal %d, shutting down gracefully", signum);
            daemon_running = 0;
            break;
        default:
            log_message(LOG_WARNING, "Received unexpected signal %d", signum);
            break;
    }
}

void cleanup_daemon(void) {
    int i;
    
    log_message(LOG_INFO, "Cleaning up daemon resources");
    
    // Stop all running room monitors
    pthread_mutex_lock(&rooms_mutex);
    for (i = 0; i < room_count; i++) {
        if (rooms[i].active && rooms[i].monitor_pid > 0) {
            log_message(LOG_INFO, "Stopping monitor for room %s (PID: %d)", 
                       rooms[i].name, rooms[i].monitor_pid);
            kill(rooms[i].monitor_pid, SIGTERM);
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    
    // Destroy mutex
    pthread_mutex_destroy(&rooms_mutex);
}

void log_message(int priority, const char *format, ...) {
    va_list args;
    va_start(args, format);
    syslog(priority, format, args);
    va_end(args);
}

const char *room_status_string(room_status_t status) {
    switch (status) {
        case ROOM_CREATED: return "CREATED";
        case ROOM_RUNNING: return "RUNNING";
        case ROOM_STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

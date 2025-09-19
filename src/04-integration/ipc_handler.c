#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "ipc_handler.h"
#include "logger.h"

#define FIFO_PATH "/tmp/monitor_fifo"
#define BUFFER_SIZE 1024

typedef struct {
    int read_fd;
    int write_fd;
    pthread_mutex_t fifo_mutex;
    int initialized;
} ipc_context_t;

static ipc_context_t ipc_ctx = {-1, -1, PTHREAD_MUTEX_INITIALIZER, 0};

// Initialize FIFO communication
int ipc_init(void) {
    // Remove existing FIFO if it exists
    unlink(FIFO_PATH);

    // Create named pipe (FIFO)
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            log_error("Failed to create FIFO: %s", strerror(errno));
            return -1;
        }
    }

    log_info("IPC FIFO initialized at %s", FIFO_PATH);
    ipc_ctx.initialized = 1;
    return 0;
}

// Send command to LOC GEN
int ipc_send_command(const command_t *cmd) {
    if (!ipc_ctx.initialized) {
        log_error("IPC not initialized");
        return -1;
    }

    if (!cmd) {
        log_error("Invalid command pointer");
        return -1;
    }

    pthread_mutex_lock(&ipc_ctx.fifo_mutex);

    // Open FIFO for writing (non-blocking)
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        log_error("Failed to open FIFO for writing: %s", strerror(errno));
        pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
        return -1;
    }

    // Serialize command
    char buffer[BUFFER_SIZE];
    int len = snprintf(buffer, BUFFER_SIZE,
                      "CMD:%d|ROOM:%s|PARAM1:%d|PARAM2:%s\n",
                      cmd->type, cmd->room_name, cmd->param1, cmd->param_str);

    // Write to FIFO
    ssize_t written = write(fd, buffer, len);
    if (written == -1) {
        log_error("Failed to write to FIFO: %s", strerror(errno));
        close(fd);
        pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
        return -1;
    }

    log_debug("Sent command: %s", buffer);

    close(fd);
    pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
    return 0;
}

// Receive data from LOC GEN
int ipc_receive_data(monitor_data_t *data) {
    if (!ipc_ctx.initialized) {
        log_error("IPC not initialized");
        return -1;
    }

    if (!data) {
        log_error("Invalid data pointer");
        return -1;
    }

    // Open FIFO for reading (non-blocking)
    int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        log_error("Failed to open FIFO for reading: %s", strerror(errno));
        return -1;
    }

    // Read from FIFO
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(fd);
        return -1;
    }

    buffer[bytes_read] = '\0';

    // Parse data format: "DATA:room_name|CPU:45.2|MEM:67.8|PROC:123"
    if (strncmp(buffer, "DATA:", 5) == 0) {
        char *token;
        char *saveptr;
        char *line = buffer + 5;  // Skip "DATA:"

        // Extract room name
        token = strtok_r(line, "|", &saveptr);
        if (token) {
            strncpy(data->room_name, token, sizeof(data->room_name) - 1);
            data->room_name[sizeof(data->room_name) - 1] = '\0';
        }

        // Parse metrics
        while ((token = strtok_r(NULL, "|", &saveptr)) != NULL) {
            if (strncmp(token, "CPU:", 4) == 0) {
                data->cpu_usage = atof(token + 4);
            } else if (strncmp(token, "MEM:", 4) == 0) {
                data->memory_usage = atof(token + 4);
            } else if (strncmp(token, "PROC:", 5) == 0) {
                data->process_count = atoi(token + 5);
            }
        }

        data->timestamp = time(NULL);
        data->valid = 1;

        log_debug("Received data from LOC GEN: room=%s, cpu=%.2f, mem=%.2f, proc=%d",
                  data->room_name, data->cpu_usage, data->memory_usage, data->process_count);
    }

    close(fd);
    return 0;
}

// Interface with kernel module via procfs
int ipc_read_kernel_data(const char *room_name, monitor_data_t *data) {
    if (!room_name || !data) {
        log_error("Invalid parameters for kernel data read");
        return -1;
    }

    // For now, simulate kernel data collection
    // In a real implementation, this would read from /proc/sysmonitor

    memset(data, 0, sizeof(monitor_data_t));
    strncpy(data->room_name, room_name, sizeof(data->room_name) - 1);
    data->room_name[sizeof(data->room_name) - 1] = '\0';

    // Try to read from procfs
    FILE *fp = fopen("/proc/sysmonitor", "r");
    if (fp) {
        char line[512];
        if (fgets(line, sizeof(line), fp)) {
            // Parse kernel module output
            // Expected format: "cpu_usage:45.2 memory_free:2048 processes:123"
            char *token = strtok(line, " ");
            while (token) {
                if (strncmp(token, "cpu_usage:", 10) == 0) {
                    data->cpu_usage = atof(token + 10);
                } else if (strncmp(token, "memory_free:", 12) == 0) {
                    data->memory_free = strtoul(token + 12, NULL, 10);
                } else if (strncmp(token, "processes:", 10) == 0) {
                    data->process_count = atoi(token + 10);
                }
                token = strtok(NULL, " ");
            }
            data->timestamp = time(NULL);
            data->valid = 1;
            log_debug("Read kernel data: cpu=%.2f, mem_free=%lu, proc=%d",
                     data->cpu_usage, data->memory_free, data->process_count);
        }
        fclose(fp);
    } else {
        // Fallback: read from system files
        data->cpu_usage = get_cpu_usage_from_proc();
        data->memory_free = get_memory_free_from_proc();
        data->process_count = get_process_count_from_proc();
        data->timestamp = time(NULL);
        data->valid = 1;
        log_debug("Read system data (fallback): cpu=%.2f, mem_free=%lu, proc=%d",
                 data->cpu_usage, data->memory_free, data->process_count);
    }

    return 0;
}

// Write control signals to kernel module
int ipc_write_kernel_control(const char *room_name, int enable) {
    if (!room_name) {
        log_error("Invalid room name for kernel control");
        return -1;
    }

    FILE *fp = fopen("/proc/sysmonitor", "w");
    if (fp) {
        fprintf(fp, "%d", enable ? 1 : 0);
        fclose(fp);
        log_info("Kernel monitoring %s for room: %s",
                enable ? "enabled" : "disabled", room_name);
        return 0;
    } else {
        log_warn("Cannot write to kernel module (procfs not available)");
        return -1;
    }
}

// Helper functions to read system information
static float get_cpu_usage_from_proc(void) {
    static unsigned long long prev_idle = 0;
    static unsigned long long prev_total = 0;

    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 8) {
        return 0.0;
    }

    unsigned long long current_idle = idle + iowait;
    unsigned long long current_total = user + nice + system + idle + iowait + irq + softirq + steal;

    if (prev_total == 0) {
        prev_idle = current_idle;
        prev_total = current_total;
        return 0.0;
    }

    unsigned long long idle_diff = current_idle - prev_idle;
    unsigned long long total_diff = current_total - prev_total;

    prev_idle = current_idle;
    prev_total = current_total;

    if (total_diff == 0) return 0.0;

    return (100.0 * (total_diff - idle_diff)) / total_diff;
}

static unsigned long get_memory_free_from_proc(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;

    char line[256];
    unsigned long mem_free = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line, "MemFree: %lu kB", &mem_free);
            break;
        }
    }

    fclose(fp);
    return mem_free;
}

static int get_process_count_from_proc(void) {
    FILE *fp = fopen("/proc/loadavg", "r");
    if (!fp) return 0;

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    float load1, load5, load15;
    int running, total;

    if (sscanf(line, "%f %f %f %d/%d", &load1, &load5, &load15, &running, &total) < 5) {
        return 0;
    }

    return total;
}

// Send raw IPC message
int ipc_send_raw_message(const ipc_message_t *msg) {
    if (!ipc_ctx.initialized || !msg) {
        return -1;
    }

    pthread_mutex_lock(&ipc_ctx.fifo_mutex);

    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
        return -1;
    }

    ssize_t written = write(fd, msg, sizeof(ipc_message_t));
    close(fd);

    pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
    return (written == sizeof(ipc_message_t)) ? 0 : -1;
}

// Receive raw IPC message
int ipc_receive_raw_message(ipc_message_t *msg) {
    if (!ipc_ctx.initialized || !msg) {
        return -1;
    }

    int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    ssize_t bytes_read = read(fd, msg, sizeof(ipc_message_t));
    close(fd);

    return (bytes_read == sizeof(ipc_message_t)) ? 0 : -1;
}

// Set non-blocking mode
int ipc_set_nonblocking(int enable) {
    // Not implemented in this demo
    (void)enable;
    return 0;
}

// Get IPC status
int ipc_get_status(void) {
    return ipc_ctx.initialized;
}

// Cleanup IPC resources
void ipc_cleanup(void) {
    pthread_mutex_lock(&ipc_ctx.fifo_mutex);

    // Remove FIFO
    unlink(FIFO_PATH);

    ipc_ctx.initialized = 0;
    pthread_mutex_unlock(&ipc_ctx.fifo_mutex);
    log_info("IPC cleanup completed");
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <ctype.h>
#include "logger.h"

#define MAX_LOG_LENGTH 2048
#define LOG_FILE_SIZE_LIMIT 10485760  // 10MB
#define LOG_FILES_TO_KEEP 5

typedef struct {
    FILE *log_file;
    char log_path[256];
    log_level_t current_level;
    pthread_mutex_t log_mutex;
    int initialized;
    int structured_mode;  // 0: plain text, 1: JSON
} logger_context_t;

static logger_context_t logger_ctx = {
    .log_file = NULL,
    .current_level = LOG_INFO,
    .log_mutex = PTHREAD_MUTEX_INITIALIZER,
    .initialized = 0,
    .structured_mode = 0
};

static const char* level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

// Initialize logger
int logger_init(const char *log_path, log_level_t level, int structured) {
    pthread_mutex_lock(&logger_ctx.log_mutex);

    // Create logs directory if it doesn't exist
    char dir_path[256];
    strncpy(dir_path, log_path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }

    // Open log file
    logger_ctx.log_file = fopen(log_path, "a");
    if (!logger_ctx.log_file) {
        pthread_mutex_unlock(&logger_ctx.log_mutex);
        return -1;
    }

    strncpy(logger_ctx.log_path, log_path, sizeof(logger_ctx.log_path) - 1);
    logger_ctx.log_path[sizeof(logger_ctx.log_path) - 1] = '\0';
    logger_ctx.current_level = level;
    logger_ctx.structured_mode = structured;
    logger_ctx.initialized = 1;

    pthread_mutex_unlock(&logger_ctx.log_mutex);

    log_info("Logger initialized: path=%s, level=%s, structured=%s",
             log_path, level_strings[level], structured ? "yes" : "no");

    return 0;
}

// Get current timestamp string
static void get_timestamp(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Rotate log file if needed
static void rotate_log_if_needed() {
    if (!logger_ctx.log_file) return;

    // Check file size
    fseek(logger_ctx.log_file, 0, SEEK_END);
    long size = ftell(logger_ctx.log_file);

    if (size >= LOG_FILE_SIZE_LIMIT) {
        fclose(logger_ctx.log_file);

        // Rotate existing files
        for (int i = LOG_FILES_TO_KEEP - 1; i > 0; i--) {
            char old_name[300], new_name[300];
            snprintf(old_name, sizeof(old_name), "%s.%d", logger_ctx.log_path, i - 1);
            snprintf(new_name, sizeof(new_name), "%s.%d", logger_ctx.log_path, i);
            rename(old_name, new_name);
        }

        // Move current log to .0
        char backup_name[300];
        snprintf(backup_name, sizeof(backup_name), "%s.0", logger_ctx.log_path);
        rename(logger_ctx.log_path, backup_name);

        // Open new log file
        logger_ctx.log_file = fopen(logger_ctx.log_path, "a");
    }
}

// Escape string for JSON
static void escape_json_string(const char *input, char *output, size_t output_size) {
    size_t input_len = strlen(input);
    size_t j = 0;

    for (size_t i = 0; i < input_len && j < output_size - 2; i++) {
        switch (input[i]) {
            case '\n':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 'n';
                }
                break;
            case '\r':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 'r';
                }
                break;
            case '\t':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 't';
                }
                break;
            case '\"':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = '\"';
                }
                break;
            case '\\':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = '\\';
                }
                break;
            default:
                if (isprint(input[i])) {
                    output[j++] = input[i];
                }
                break;
        }
    }
    output[j] = '\0';
}

// Log message with structured or plain format
static void log_message(log_level_t level, const char *file, int line,
                       const char *func, const char *format, va_list args) {
    if (!logger_ctx.initialized || level < logger_ctx.current_level) {
        return;
    }

    pthread_mutex_lock(&logger_ctx.log_mutex);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    char message[MAX_LOG_LENGTH];
    vsnprintf(message, sizeof(message), format, args);

    if (logger_ctx.structured_mode) {
        // JSON structured logging
        char escaped_message[MAX_LOG_LENGTH * 2];
        escape_json_string(message, escaped_message, sizeof(escaped_message));

        fprintf(logger_ctx.log_file,
                "{\"timestamp\":\"%s\","
                "\"level\":\"%s\","
                "\"file\":\"%s\","
                "\"line\":%d,"
                "\"function\":\"%s\","
                "\"message\":\"%s\","
                "\"pid\":%d,"
                "\"thread\":\"0x%lx\"}\n",
                timestamp, level_strings[level], file, line, func, escaped_message,
                getpid(), pthread_self());
    } else {
        // Plain text logging
        fprintf(logger_ctx.log_file, "[%s] [%s] [%s:%d:%s] [PID:%d] %s\n",
                timestamp, level_strings[level], file, line, func, getpid(), message);
    }

    fflush(logger_ctx.log_file);
    rotate_log_if_needed();
    pthread_mutex_unlock(&logger_ctx.log_mutex);
}

// Public logging functions
void log_trace(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_TRACE, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARN, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_ERROR, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

void log_fatal(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_FATAL, __FILE__, __LINE__, __func__, format, args);
    va_end(args);
}

// Advanced logging function with context
void log_with_context(log_level_t level, const char *component,
                     const char *function, int line, const char *format, ...) {
    if (!logger_ctx.initialized || level < logger_ctx.current_level) {
        return;
    }

    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&logger_ctx.log_mutex);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    char message[MAX_LOG_LENGTH];
    vsnprintf(message, sizeof(message), format, args);

    if (logger_ctx.structured_mode) {
        char escaped_message[MAX_LOG_LENGTH * 2];
        escape_json_string(message, escaped_message, sizeof(escaped_message));

        fprintf(logger_ctx.log_file,
                "{\"timestamp\":\"%s\","
                "\"level\":\"%s\","
                "\"component\":\"%s\","
                "\"function\":\"%s\","
                "\"line\":%d,"
                "\"message\":\"%s\","
                "\"pid\":%d}\n",
                timestamp, level_strings[level], component, function, line, escaped_message, getpid());
    } else {
        fprintf(logger_ctx.log_file, "[%s] [%s] [%s:%s:%d] %s\n",
                timestamp, level_strings[level], component, function, line, message);
    }

    fflush(logger_ctx.log_file);
    rotate_log_if_needed();
    pthread_mutex_unlock(&logger_ctx.log_mutex);
    va_end(args);
}

// Performance logging
void log_performance(const char *operation, double duration_ms) {
    if (!logger_ctx.initialized || LOG_DEBUG < logger_ctx.current_level) {
        return;
    }

    pthread_mutex_lock(&logger_ctx.log_mutex);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    if (logger_ctx.structured_mode) {
        fprintf(logger_ctx.log_file,
                "{\"timestamp\":\"%s\","
                "\"level\":\"PERF\","
                "\"operation\":\"%s\","
                "\"duration_ms\":%.3f,"
                "\"pid\":%d}\n",
                timestamp, operation, duration_ms, getpid());
    } else {
        fprintf(logger_ctx.log_file, "[%s] [PERF] %s took %.3f ms\n",
                timestamp, operation, duration_ms);
    }

    fflush(logger_ctx.log_file);
    rotate_log_if_needed();
    pthread_mutex_unlock(&logger_ctx.log_mutex);
}

// Set log level
void logger_set_level(log_level_t level) {
    pthread_mutex_lock(&logger_ctx.log_mutex);
    logger_ctx.current_level = level;
    pthread_mutex_unlock(&logger_ctx.log_mutex);
}

// Set structured mode
void logger_set_structured_mode(int enable) {
    pthread_mutex_lock(&logger_ctx.log_mutex);
    logger_ctx.structured_mode = enable;
    pthread_mutex_unlock(&logger_ctx.log_mutex);
}

// Log level string conversion
const char* log_level_to_string(log_level_t level) {
    if (level >= 0 && level <= LOG_FATAL) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

log_level_t log_level_from_string(const char *level_str) {
    if (!level_str) return LOG_INFO;

    for (int i = 0; i <= LOG_FATAL; i++) {
        if (strcasecmp(level_str, level_strings[i]) == 0) {
            return (log_level_t)i;
        }
    }
    return LOG_INFO;
}

// Cleanup logger
void logger_cleanup(void) {
    pthread_mutex_lock(&logger_ctx.log_mutex);

    if (logger_ctx.log_file) {
        log_info("Logger shutting down");
        fclose(logger_ctx.log_file);
        logger_ctx.log_file = NULL;
    }

    logger_ctx.initialized = 0;
    pthread_mutex_unlock(&logger_ctx.log_mutex);
}

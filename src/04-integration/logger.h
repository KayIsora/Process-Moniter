#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

// Log levels
typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARN = 3,
    LOG_ERROR = 4,
    LOG_FATAL = 5
} log_level_t;

// Logger configuration
typedef struct {
    char log_path[256];
    log_level_t level;
    int structured_mode;
    int enable_rotation;
    unsigned long max_file_size;
    int max_files;
} logger_config_t;

// Function declarations
int logger_init(const char *log_path, log_level_t level, int structured);
int logger_init_with_config(const logger_config_t *config);
void logger_set_level(log_level_t level);
void logger_set_structured_mode(int enable);
void logger_cleanup(void);

// Logging functions
void log_trace(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_debug(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_info(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_warn(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));
void log_fatal(const char *format, ...) __attribute__((format(printf, 1, 2)));

// Advanced logging functions
void log_with_context(log_level_t level, const char *component, 
                     const char *function, int line, const char *format, ...);
void log_performance(const char *operation, double duration_ms);
const char* log_level_to_string(log_level_t level);
log_level_t log_level_from_string(const char *level_str);

#endif /* LOGGER_H */

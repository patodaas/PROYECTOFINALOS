#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

/* Códigos de error */
#define SUCCESS 0
#define ERROR_GENERIC -1
#define ERROR_NOT_FOUND -2
#define ERROR_PERMISSION -3
#define ERROR_INVALID_PARAM -4
#define ERROR_SYSTEM_CALL -5

/* Tamaños de buffer */
#define MAX_PATH 256
#define MAX_COMMAND 1024
#define MAX_OUTPUT 4096
#define MAX_NAME 64

/* Macros de utilidad */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Logging */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;

void log_message(log_level_t level, const char *format, ...);

/* Utilidades */
int execute_command(const char *cmd, char *output, size_t output_size);
int file_exists(const char *path);
int is_root(void);
unsigned long long parse_size(const char *size_str);

#endif /* COMMON_H */

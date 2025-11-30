#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

/* Colores para logging */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GRAY    "\033[90m"

/**
 * Función de logging con niveles
 * (usa log_level_t definido en common.h)
 */
void log_message(log_level_t level, const char *format, ...) {
    va_list args;
    time_t now = time(NULL);
    char time_buf[64];
    const char *level_str;
    const char *color;

    /* Formatear timestamp */
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    /* Determinar nivel y color */
    switch (level) {
        case LOG_DEBUG:
            level_str = "DEBUG";
            color = COLOR_GRAY;
            break;
        case LOG_INFO:
            level_str = "INFO";
            color = COLOR_BLUE;
            break;
        case LOG_WARNING:
            level_str = "WARNING";
            color = COLOR_YELLOW;
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            color = COLOR_RED;
            break;
        default:
            level_str = "UNKNOWN";
            color = COLOR_RESET;
    }

    /* Imprimir con formato */
    fprintf(stderr, "%s[%s] [%s]%s ", color, time_buf, level_str, COLOR_RESET);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

/**
 * Ejecuta un comando del sistema y captura su salida
 * Devuelve SUCCESS o ERROR_SYSTEM_CALL definidos en common.h
 */
int execute_command(const char *cmd, char *output, size_t output_size) {
    FILE *fp;
    char buffer[256];
    size_t total = 0;

    log_message(LOG_DEBUG, "Executing command: %s", cmd);

    /* Abrir pipe para leer salida del comando */
    fp = popen(cmd, "r");
    if (fp == NULL) {
        log_message(LOG_ERROR, "Failed to execute command: %s", strerror(errno));
        return ERROR_SYSTEM_CALL;
    }

    /* Leer salida */
    if (output != NULL && output_size > 0) {
        output[0] = '\0';
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            size_t len = strlen(buffer);
            if (total + len < output_size - 1) {
                strcat(output, buffer);
                total += len;
            } else {
                break;
            }
        }
    }

    /* Cerrar pipe y obtener código de retorno */
    int status = pclose(fp);
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            log_message(LOG_WARNING, "Command exited with code %d", exit_code);
            return ERROR_SYSTEM_CALL;
        }
    } else {
        log_message(LOG_ERROR, "Command did not exit normally");
        return ERROR_SYSTEM_CALL;
    }

    return SUCCESS;
}

/**
 * Verifica si un archivo existe
 */
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/**
 * Verifica si el proceso tiene privilegios de root
 */
int is_root(void) {
    return (geteuid() == 0);
}

/**
 * Versión "de sistema" para verificar root con mensajes de error
 */
int check_root(void) {
    if (!is_root()) {
        fprintf(stderr, "Error: This operation requires root privileges\n");
        fprintf(stderr, "Please run with sudo\n");
        return -1;
    }
    return 0;
}

/**
 * Crear directorio si no existe
 */
int ensure_directory(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;  /* Ya existe */
        } else {
            fprintf(stderr, "Error: %s exists but is not a directory\n", path);
            return -1;
        }
    }

    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return -1;
    }

    return 0;
}

/**
 * Verificar si un dispositivo existe
 */
int device_exists(const char *device) {
    struct stat st;
    return stat(device, &st) == 0;
}

/**
 * Obtener timestamp como string (útil si no quieres usar el logger)
 */
void get_timestamp_string(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/**
 * Formatear bytes a human-readable
 */
void format_bytes(unsigned long long bytes, char *buf, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double value = (double)bytes;

    while (value >= 1024.0 && unit_index < 4) {
        value /= 1024.0;
        unit_index++;
    }

    snprintf(buf, size, "%.2f %s", value, units[unit_index]);
}

/**
 * Parsea una cadena de tamaño (e.g., "100M", "1G") a bytes
 * Usa la versión más robusta (double + sufijo) del código 1
 */
unsigned long long parse_size(const char *size_str) {
    char *endptr;
    double value = strtod(size_str, &endptr);

    if (value < 0) {
        return 0;
    }

    /* Multiplicador basado en sufijo */
    unsigned long long multiplier = 1;
    if (*endptr != '\0') {
        switch (*endptr) {
            case 'K': case 'k':
                multiplier = 1024ULL;
                break;
            case 'M': case 'm':
                multiplier = 1024ULL * 1024ULL;
                break;
            case 'G': case 'g':
                multiplier = 1024ULL * 1024ULL * 1024ULL;
                break;
            case 'T': case 't':
                multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
                break;
            default:
                log_message(LOG_WARNING, "Unknown size suffix: %c", *endptr);
                return 0;
        }
    }

    return (unsigned long long)(value * multiplier);
}

/**
 * Verificar si un proceso está corriendo
 */
int is_process_running(pid_t pid) {
    if (kill(pid, 0) == 0) {
        return 1;  /* Proceso existe */
    }
    return 0;      /* No existe */
}

/**
 * Leer PID desde archivo
 */
pid_t read_pid_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return pid;
}

/**
 * Escribir PID a archivo
 */
int write_pid_file(const char *path, pid_t pid) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }

    fprintf(fp, "%d\n", pid);
    fclose(fp);
    return 0;
}


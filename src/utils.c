#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>  // Para logger
#include <signal.h>  // Para kill()
#include <sys/wait.h>
#include <fcntl.h>   // Por si se usa open() en utils en el futuro


// Verificar si somos root
int check_root(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "Error: This operation requires root privileges\n");
        fprintf(stderr, "Please run with sudo\n");
        return -1;
    }
    return 0;
}

// Crear directorio si no existe
int ensure_directory(const char *path) {
    struct stat st;
    
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;  // Ya existe
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

// Ejecutar comando y capturar salida
int execute_command(const char *cmd, char *output, size_t output_size) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }
    
    if (output && output_size > 0) {
        size_t pos = 0;
        char line[256];
        
        while (fgets(line, sizeof(line), fp) && pos < output_size - 1) {
            size_t len = strlen(line);
            if (pos + len >= output_size - 1) {
                len = output_size - pos - 1;
            }
            memcpy(output + pos, line, len);
            pos += len;
        }
        output[pos] = '\0';
    }
    
    int status = pclose(fp);
    return WEXITSTATUS(status);
}

// Verificar si un dispositivo existe
int device_exists(const char *device) {
    struct stat st;
    return stat(device, &st) == 0;
}

// Obtener timestamp como string
void get_timestamp_string(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Formatear bytes a human-readable
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

// Logger simple
void log_message(const char *level, const char *format, ...) {
    char timestamp[64];
    get_timestamp_string(timestamp, sizeof(timestamp));
    
    fprintf(stderr, "[%s] [%s] ", timestamp, level);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

// Verificar si un proceso está corriendo
int is_process_running(pid_t pid) {
    if (kill(pid, 0) == 0) {
        return 1;  // Proceso existe
    }
    return 0;  // No existe
}

// Leer PID desde archivo
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

// Escribir PID a archivo
int write_pid_file(const char *path, pid_t pid) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "%d\n", pid);
    fclose(fp);
    return 0;
}

// Parsear tamaño con unidades (ej: "1G", "500M")
unsigned long long parse_size(const char *str) {
    char *endptr;
    unsigned long long value = strtoull(str, &endptr, 10);
    
    if (*endptr) {
        switch (*endptr) {
            case 'K':
            case 'k':
                value *= 1024;
                break;
            case 'M':
            case 'm':
                value *= 1024 * 1024;
                break;
            case 'G':
            case 'g':
                value *= 1024 * 1024 * 1024;
                break;
            case 'T':
            case 't':
                value *= 1024ULL * 1024 * 1024 * 1024;
                break;
        }
    }
    
    return value;
}

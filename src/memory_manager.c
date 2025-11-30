#include "../include/memory_manager.h"

/* ========== Swap Management ========== */

int swap_create_file(const char *path, unsigned long long size_mb) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Creating swap file: %s (%llu MB)", path, size_mb);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    // Crear archivo con dd
    snprintf(cmd, sizeof(cmd),
             "dd if=/dev/zero of=%s bs=1M count=%llu 2>&1",
             path, size_mb);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create swap file");
        return ERROR_SYSTEM_CALL;
    }
    
    // Establecer permisos
    chmod(path, 0600);
    
    log_message(LOG_INFO, "Swap file created successfully");
    return SUCCESS;
}

int swap_make(const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Formatting swap: %s", device);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "mkswap %s 2>&1", device);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to format swap");
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int swap_enable(const char *path, int priority) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Enabling swap: %s", path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    if (priority >= 0) {
        snprintf(cmd, sizeof(cmd), "swapon -p %d %s 2>&1", priority, path);
    } else {
        snprintf(cmd, sizeof(cmd), "swapon %s 2>&1", path);
    }
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to enable swap: %s", output);
        return ERROR_SYSTEM_CALL;
    }
    
    log_message(LOG_INFO, "Swap enabled successfully");
    return SUCCESS;
}

int swap_disable(const char *path) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Disabling swap: %s", path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "swapoff %s 2>&1", path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to disable swap");
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int swap_list(swap_info_t *swaps, int max_swaps, int *count) {
    FILE *fp = fopen("/proc/swaps", "r");
    if (fp == NULL) return ERROR_SYSTEM_CALL;
    
    *count = 0;
    char line[256];
    
    // Saltar cabecera
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp) && *count < max_swaps) {
        swap_info_t *swap = &swaps[*count];
        memset(swap, 0, sizeof(swap_info_t));
        
        unsigned long size, used;
        sscanf(line, "%255s %15s %lu %lu %d",
               swap->path, swap->type, &size, &used, &swap->priority);
        
        swap->size_kb = size;
        swap->used_kb = used;
        swap->is_active = 1;
        
        (*count)++;
    }
    
    fclose(fp);
    return SUCCESS;
}

int swap_remove_file(const char *path) {
    if (unlink(path) != 0) {
        log_message(LOG_ERROR, "Failed to remove swap file: %s", strerror(errno));
        return ERROR_SYSTEM_CALL;
    }
    return SUCCESS;
}

/* ========== Memory Monitoring ========== */

int memory_parse_meminfo(memory_info_t *info) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return ERROR_SYSTEM_CALL;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu", &info->total_kb) == 1) continue;
        if (sscanf(line, "MemFree: %llu", &info->free_kb) == 1) continue;
        if (sscanf(line, "MemAvailable: %llu", &info->available_kb) == 1) continue;
        if (sscanf(line, "Cached: %llu", &info->cached_kb) == 1) continue;
        if (sscanf(line, "Buffers: %llu", &info->buffers_kb) == 1) continue;
        if (sscanf(line, "SwapTotal: %llu", &info->swap_total_kb) == 1) continue;
        if (sscanf(line, "SwapFree: %llu", &info->swap_free_kb) == 1) continue;
    }
    
    fclose(fp);
    
    info->swap_used_kb = info->swap_total_kb - info->swap_free_kb;
    
    if (info->swap_total_kb > 0) {
        info->swap_usage_percent = 
            (float)info->swap_used_kb / info->swap_total_kb * 100.0;
    }
    
    return SUCCESS;
}

float memory_calculate_pressure(const memory_info_t *info) {
    if (info->total_kb == 0) return 0.0;
    
    // Presión basada en memoria disponible
    float available_ratio = (float)info->available_kb / info->total_kb;
    float pressure = 1.0 - available_ratio;
    
    // Aumentar presión si hay uso de swap
    if (info->swap_total_kb > 0 && info->swap_used_kb > 0) {
        float swap_factor = (float)info->swap_used_kb / info->swap_total_kb;
        pressure = pressure * 0.7 + swap_factor * 0.3;
    }
    
    return pressure > 1.0 ? 1.0 : pressure;
}

int memory_get_info(memory_info_t *info) {
    if (info == NULL) return ERROR_INVALID_PARAM;
    
    memset(info, 0, sizeof(memory_info_t));
    
    int result = memory_parse_meminfo(info);
    if (result != SUCCESS) return result;
    
    info->memory_pressure = memory_calculate_pressure(info);
    
    return SUCCESS;
}

int memory_check_low(unsigned long long threshold_mb) {
    memory_info_t info;
    
    if (memory_get_info(&info) != SUCCESS) {
        return -1;
    }
    
    unsigned long long threshold_kb = threshold_mb * 1024;
    return (info.available_kb < threshold_kb) ? 1 : 0;
}

int memory_parse_swaps(swap_info_t *swaps, int max_swaps, int *count) {
    return swap_list(swaps, max_swaps, count);
}

/* ========== Gestión Automática ========== */

int memory_auto_swap(unsigned long long threshold_mb,
                     unsigned long long swap_size_mb,
                     const char *swap_path) {
    
    log_message(LOG_INFO, "Checking if automatic swap is needed...");
    
    // Verificar memoria
    int low_memory = memory_check_low(threshold_mb);
    
    if (low_memory <= 0) {
        log_message(LOG_INFO, "Memory is sufficient, no swap needed");
        return 0;
    }
    
    log_message(LOG_WARNING, "Low memory detected, creating swap...");
    
    // Crear swap
    if (swap_create_file(swap_path, swap_size_mb) != SUCCESS) {
        return ERROR_SYSTEM_CALL;
    }
    
    if (swap_make(swap_path) != SUCCESS) {
        swap_remove_file(swap_path);
        return ERROR_SYSTEM_CALL;
    }
    
    if (swap_enable(swap_path, -1) != SUCCESS) {
        swap_remove_file(swap_path);
        return ERROR_SYSTEM_CALL;
    }
    
    log_message(LOG_INFO, "Automatic swap created and enabled");
    return SUCCESS;
}

int memory_monitor_continuous(int check_interval_sec,
                               void (*callback)(const memory_info_t *info)) {
    log_message(LOG_INFO, "Starting continuous memory monitoring...");
    
    while (1) {
        memory_info_t info;
        
        if (memory_get_info(&info) == SUCCESS) {
            if (callback != NULL) {
                callback(&info);
            }
            
            // Alertas automáticas
            if (info.memory_pressure > 0.8) {
                log_message(LOG_WARNING,
                           "HIGH memory pressure: %.1f%%",
                           info.memory_pressure * 100);
            }
            
            if (info.swap_usage_percent > 80.0) {
                log_message(LOG_WARNING,
                           "HIGH swap usage: %.1f%%",
                           info.swap_usage_percent);
            }
        }
        
        sleep(check_interval_sec);
    }
    
    return SUCCESS;
}

/* ========== Utilidades ========== */

void memory_format_size(unsigned long long kb, char *buffer, size_t size) {
    if (kb < 1024) {
        snprintf(buffer, size, "%llu KB", kb);
    } else if (kb < 1024 * 1024) {
        snprintf(buffer, size, "%.2f MB", kb / 1024.0);
    } else {
        snprintf(buffer, size, "%.2f GB", kb / (1024.0 * 1024.0));
    }
}

void memory_print_info(const memory_info_t *info) {
    char buf[64];
    
    printf("\n========== Memory Information ==========\n");
    
    memory_format_size(info->total_kb, buf, sizeof(buf));
    printf("Total Memory:     %s\n", buf);
    
    memory_format_size(info->free_kb, buf, sizeof(buf));
    printf("Free Memory:      %s\n", buf);
    
    memory_format_size(info->available_kb, buf, sizeof(buf));
    printf("Available Memory: %s\n", buf);
    
    memory_format_size(info->cached_kb, buf, sizeof(buf));
    printf("Cached:           %s\n", buf);
    
    memory_format_size(info->buffers_kb, buf, sizeof(buf));
    printf("Buffers:          %s\n", buf);
    
    printf("\n");
    
    memory_format_size(info->swap_total_kb, buf, sizeof(buf));
    printf("Total Swap:       %s\n", buf);
    
    memory_format_size(info->swap_used_kb, buf, sizeof(buf));
    printf("Used Swap:        %s\n", buf);
    
    printf("Swap Usage:       %.1f%%\n", info->swap_usage_percent);
    printf("Memory Pressure:  %.1f%%\n", info->memory_pressure * 100);
    
    printf("========================================\n\n");
}

void memory_print_swap(const swap_info_t *swap) {
    char size_buf[64], used_buf[64];
    
    memory_format_size(swap->size_kb, size_buf, sizeof(size_buf));
    memory_format_size(swap->used_kb, used_buf, sizeof(used_buf));
    
    printf("%-30s %-10s %10s %10s %5d\n",
           swap->path, swap->type, size_buf, used_buf, swap->priority);
}

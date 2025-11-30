#include "../include/raid_manager.h"
#include <sys/wait.h>

/**
 * Crea un array RAID
 */
int raid_create(const char *array_name, int level, char **devices, int count) {
    char cmd[MAX_COMMAND];
    char devices_str[MAX_COMMAND] = "";
    
    log_message(LOG_INFO, "Creating RAID%d array: %s with %d devices", 
                level, array_name, count);
    
    // Verificar permisos de root
    if (!is_root()) {
        log_message(LOG_ERROR, "Root privileges required for RAID operations");
        return ERROR_PERMISSION;
    }
    
    // Validar parámetros
    if (array_name == NULL || devices == NULL || count < 2) {
        log_message(LOG_ERROR, "Invalid parameters for RAID creation");
        return ERROR_INVALID_PARAM;
    }
    
    // Validar nivel de RAID
    if (level != 0 && level != 1 && level != 5 && level != 10) {
        log_message(LOG_ERROR, "Unsupported RAID level: %d", level);
        return ERROR_INVALID_PARAM;
    }
    
    // Construir string con dispositivos
    for (int i = 0; i < count; i++) {
        if (!file_exists(devices[i])) {
            log_message(LOG_ERROR, "Device does not exist: %s", devices[i]);
            return ERROR_NOT_FOUND;
        }
        strcat(devices_str, devices[i]);
        if (i < count - 1) {
            strcat(devices_str, " ");
        }
    }
    
    // Limpiar superblocks previos (importante)
    for (int i = 0; i < count; i++) {
        snprintf(cmd, sizeof(cmd), "mdadm --zero-superblock %s 2>/dev/null", devices[i]);
        execute_command(cmd, NULL, 0);
    }
    
    // Crear array RAID
    snprintf(cmd, sizeof(cmd), 
         "mdadm --create %s --level=%d --raid-devices=%d %s --force --run 2>&1",
         array_name, level, count, devices_str);
    
    char output[MAX_OUTPUT];
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create RAID array: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "RAID array created successfully: %s", array_name);
    
    // Esperar un momento para que el sistema reconozca el array
    sleep(1);
    
    return SUCCESS;
}

/**
 * Obtiene el estado de un array RAID
 */
int raid_get_status(const char *array_name, raid_array_t *array) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    if (array_name == NULL || array == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // Verificar que el array existe
    if (!file_exists(array_name)) {
        log_message(LOG_ERROR, "RAID array does not exist: %s", array_name);
        return ERROR_NOT_FOUND;
    }
    
    // Obtener detalles del array
    snprintf(cmd, sizeof(cmd), "mdadm --detail %s 2>&1", array_name);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to get RAID status");
        return result;
    }
    
    // Inicializar estructura
    memset(array, 0, sizeof(raid_array_t));
    strncpy(array->name, array_name, sizeof(array->name) - 1);
    
    // Parsear salida
    char *line = strtok(output, "\n");
    while (line != NULL) {
        // Buscar nivel de RAID
        if (strstr(line, "Raid Level :")) {
            sscanf(line, " Raid Level : raid%d", &array->raid_level);
        }
        // Buscar estado
        else if (strstr(line, "State :")) {
            if (strstr(line, "active")) {
                if (strstr(line, "degraded")) {
                    strcpy(array->status, RAID_STATUS_DEGRADED);
                } else {
                    strcpy(array->status, RAID_STATUS_ACTIVE);
                }
            } else {
                strcpy(array->status, RAID_STATUS_FAILED);
            }
        }
        // Contar dispositivos
        else if (strstr(line, "Raid Devices :")) {
            sscanf(line, " Raid Devices : %d", &array->num_devices);
        }
        // Contar dispositivos fallidos
        else if (strstr(line, "Failed Devices :")) {
            sscanf(line, " Failed Devices : %d", &array->num_failed);
        }
        // Contar dispositivos activos
        else if (strstr(line, "Active Devices :")) {
            sscanf(line, " Active Devices : %d", &array->num_active);
        }
        // Obtener tamaño
        else if (strstr(line, "Array Size :")) {
            sscanf(line, " Array Size : %llu", &array->size_kb);
        }
        // Obtener dispositivos del array
        else if (strstr(line, "/dev/")) {
            char *dev_start = strstr(line, "/dev/");
            if (dev_start && array->num_devices < 16) {
                char device[MAX_PATH];
                sscanf(dev_start, "%s", device);
                // Solo agregar si no está ya en la lista
                int found = 0;
                for (int i = 0; i < 16 && array->devices[i][0] != '\0'; i++) {
                    if (strcmp(array->devices[i], device) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    for (int i = 0; i < 16; i++) {
                        if (array->devices[i][0] == '\0') {
                            strncpy(array->devices[i], device, MAX_PATH - 1);
                            break;
                        }
                    }
                }
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    log_message(LOG_INFO, "RAID %s status: %s (level: %d, devices: %d/%d)",
                array->name, array->status, array->raid_level, 
                array->num_active, array->num_devices);
    
    return SUCCESS;
}

/**
 * Monitorea el estado del array (wrapper)
 */
int raid_monitor(raid_array_t *array) {
    if (array == NULL || array->name[0] == '\0') {
        return ERROR_INVALID_PARAM;
    }
    
    return raid_get_status(array->name, array);
}

/**
 * Añade un disco al array RAID
 */
int raid_add_disk(const char *array_name, const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Adding disk %s to RAID array %s", device, array_name);
    
    if (!is_root()) {
        log_message(LOG_ERROR, "Root privileges required");
        return ERROR_PERMISSION;
    }
    
    if (!file_exists(array_name)) {
        log_message(LOG_ERROR, "RAID array does not exist: %s", array_name);
        return ERROR_NOT_FOUND;
    }
    
    if (!file_exists(device)) {
        log_message(LOG_ERROR, "Device does not exist: %s", device);
        return ERROR_NOT_FOUND;
    }
    
    // Limpiar superblock del nuevo dispositivo
    snprintf(cmd, sizeof(cmd), "mdadm --zero-superblock %s 2>/dev/null", device);
    execute_command(cmd, NULL, 0);
    
    // Añadir disco
    snprintf(cmd, sizeof(cmd), "mdadm --add %s %s 2>&1", array_name, device);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to add disk: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Disk added successfully");
    return SUCCESS;
}

/**
 * Marca un disco como fallido
 */
int raid_fail_disk(const char *array_name, const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Marking disk %s as failed in array %s", device, array_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "mdadm --fail %s %s 2>&1", array_name, device);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to mark disk as failed: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Disk marked as failed");
    return SUCCESS;
}

/**
 * Remueve un disco del array
 */
int raid_remove_disk(const char *array_name, const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing disk %s from array %s", device, array_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "mdadm --remove %s %s 2>&1", array_name, device);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to remove disk: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Disk removed successfully");
    return SUCCESS;
}

/**
 * Detiene un array RAID
 */
int raid_stop(const char *array_name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Stopping RAID array: %s", array_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "mdadm --stop %s 2>&1", array_name);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to stop array: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Array stopped successfully");
    return SUCCESS;
}

/**
 * Lista todos los arrays RAID
 */
int raid_list_all(raid_array_t *arrays, int max_arrays, int *count) {
    char output[MAX_OUTPUT];
    
    if (arrays == NULL || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Parsear /proc/mdstat
    FILE *fp = fopen("/proc/mdstat", "r");
    if (fp == NULL) {
        log_message(LOG_WARNING, "Cannot open /proc/mdstat");
        return ERROR_SYSTEM_CALL;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL && *count < max_arrays) {
        // Buscar líneas que empiezan con "md"
        if (strncmp(line, "md", 2) == 0) {
            char md_name[32];
            sscanf(line, "%s", md_name);
            
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "/dev/%s", md_name);
            
            // Obtener detalles completos
            if (raid_get_status(full_path, &arrays[*count]) == SUCCESS) {
                (*count)++;
            }
        }
    }
    
    fclose(fp);
    
    log_message(LOG_INFO, "Found %d RAID arrays", *count);
    return SUCCESS;
}

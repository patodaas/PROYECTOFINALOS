#include "../include/lvm_manager.h"
#include <ctype.h>

/* ========== Physical Volume (PV) Functions ========== */

/**
 * Crea un Physical Volume
 */
int lvm_pv_create(const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Creating Physical Volume: %s", device);
    
    if (!is_root()) {
        log_message(LOG_ERROR, "Root privileges required");
        return ERROR_PERMISSION;
    }
    
    if (!file_exists(device)) {
        log_message(LOG_ERROR, "Device does not exist: %s", device);
        return ERROR_NOT_FOUND;
    }
    
    // Crear PV
    snprintf(cmd, sizeof(cmd), "pvcreate %s 2>&1", device);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create PV: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Physical Volume created successfully");
    return SUCCESS;
}

/**
 * Lista todos los Physical Volumes
 */
int lvm_pv_list(pv_info_t *pvs, int max_pvs, int *count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    if (pvs == NULL || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Obtener lista de PVs en formato parseable
    snprintf(cmd, sizeof(cmd), 
             "pvs --noheadings --units b --nosuffix -o pv_name,pv_size,pv_free,vg_name 2>&1");
    
    int result = execute_command(cmd, output, sizeof(output));
    if (result != SUCCESS) {
        log_message(LOG_WARNING, "No PVs found or error listing PVs");
        return SUCCESS; // No es error, simplemente no hay PVs
    }
    
    // Parsear salida
    char *line = strtok(output, "\n");
    while (line != NULL && *count < max_pvs) {
        // Saltar espacios iniciales
        while (isspace(*line)) line++;
        
        if (strlen(line) > 0) {
            pv_info_t *pv = &pvs[*count];
            memset(pv, 0, sizeof(pv_info_t));
            
            // Parsear línea: pv_name pv_size pv_free vg_name
            char vg_temp[MAX_NAME] = "";
            int parsed = sscanf(line, "%255s %llu %llu %63s",
                               pv->pv_name, &pv->size_bytes, 
                               &pv->free_bytes, vg_temp);
            
            if (parsed >= 3) {
                if (parsed == 4 && strlen(vg_temp) > 0) {
                    strncpy(pv->vg_name, vg_temp, MAX_NAME - 1);
                    pv->is_allocated = 1;
                } else {
                    pv->is_allocated = 0;
                }
                (*count)++;
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    log_message(LOG_INFO, "Found %d Physical Volumes", *count);
    return SUCCESS;
}

/**
 * Obtiene información de un PV específico
 */
int lvm_pv_info(const char *pv_name, pv_info_t *info) {
    pv_info_t pvs[32];
    int count;
    
    if (pv_name == NULL || info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    int result = lvm_pv_list(pvs, 32, &count);
    if (result != SUCCESS) {
        return result;
    }
    
    // Buscar el PV específico
    for (int i = 0; i < count; i++) {
        if (strcmp(pvs[i].pv_name, pv_name) == 0) {
            memcpy(info, &pvs[i], sizeof(pv_info_t));
            return SUCCESS;
        }
    }
    
    log_message(LOG_ERROR, "Physical Volume not found: %s", pv_name);
    return ERROR_NOT_FOUND;
}

/**
 * Remueve un Physical Volume
 */
int lvm_pv_remove(const char *pv_name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing Physical Volume: %s", pv_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    // Verificar que el PV no está en uso
    pv_info_t info;
    if (lvm_pv_info(pv_name, &info) == SUCCESS && info.is_allocated) {
        log_message(LOG_ERROR, "PV is in use by VG: %s", info.vg_name);
        return ERROR_GENERIC;
    }
    
    snprintf(cmd, sizeof(cmd), "pvremove -f %s 2>&1", pv_name);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to remove PV: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Physical Volume removed successfully");
    return SUCCESS;
}

/* ========== Volume Group (VG) Functions ========== */

/**
 * Crea un Volume Group
 */
int lvm_vg_create(const char *vg_name, char **pvs, int pv_count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    char pvs_str[MAX_COMMAND] = "";
    
    log_message(LOG_INFO, "Creating Volume Group: %s with %d PVs", vg_name, pv_count);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (vg_name == NULL || pvs == NULL || pv_count < 1) {
        return ERROR_INVALID_PARAM;
    }
    
    // Construir lista de PVs
    for (int i = 0; i < pv_count; i++) {
        strcat(pvs_str, pvs[i]);
        if (i < pv_count - 1) {
            strcat(pvs_str, " ");
        }
    }
    
    // Crear VG
    snprintf(cmd, sizeof(cmd), "vgcreate %s %s 2>&1", vg_name, pvs_str);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create VG: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Volume Group created successfully");
    return SUCCESS;
}

/**
 * Extiende un Volume Group
 */
int lvm_vg_extend(const char *vg_name, const char *pv) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Extending VG %s with PV %s", vg_name, pv);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "vgextend %s %s 2>&1", vg_name, pv);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to extend VG: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Volume Group extended successfully");
    return SUCCESS;
}

/**
 * Lista todos los Volume Groups
 */
int lvm_vg_list(vg_info_t *vgs, int max_vgs, int *count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    if (vgs == NULL || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Obtener lista de VGs
    snprintf(cmd, sizeof(cmd),
             "vgs --noheadings --units b --nosuffix -o vg_name,vg_size,vg_free,pv_count,lv_count 2>&1");
    
    int result = execute_command(cmd, output, sizeof(output));
    if (result != SUCCESS) {
        log_message(LOG_WARNING, "No VGs found or error listing VGs");
        return SUCCESS;
    }
    
    // Parsear salida
    char *line = strtok(output, "\n");
    while (line != NULL && *count < max_vgs) {
        while (isspace(*line)) line++;
        
        if (strlen(line) > 0) {
            vg_info_t *vg = &vgs[*count];
            memset(vg, 0, sizeof(vg_info_t));
            
            int parsed = sscanf(line, "%63s %llu %llu %d %d",
                               vg->vg_name, &vg->size_bytes, &vg->free_bytes,
                               &vg->pv_count, &vg->lv_count);
            
            if (parsed == 5) {
                (*count)++;
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    log_message(LOG_INFO, "Found %d Volume Groups", *count);
    return SUCCESS;
}

/**
 * Obtiene información de un VG específico
 */
int lvm_vg_info(const char *vg_name, vg_info_t *info) {
    vg_info_t vgs[32];
    int count;
    
    if (vg_name == NULL || info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    int result = lvm_vg_list(vgs, 32, &count);
    if (result != SUCCESS) {
        return result;
    }
    
    for (int i = 0; i < count; i++) {
        if (strcmp(vgs[i].vg_name, vg_name) == 0) {
            memcpy(info, &vgs[i], sizeof(vg_info_t));
            return SUCCESS;
        }
    }
    
    log_message(LOG_ERROR, "Volume Group not found: %s", vg_name);
    return ERROR_NOT_FOUND;
}

/**
 * Remueve un Volume Group
 */
int lvm_vg_remove(const char *vg_name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing Volume Group: %s", vg_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "vgremove -f %s 2>&1", vg_name);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to remove VG: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Volume Group removed successfully");
    return SUCCESS;
}

/* ========== Logical Volume (LV) Functions ========== */

/**
 * Crea un Logical Volume
 */
int lvm_lv_create(const char *vg_name, const char *lv_name,
                  unsigned long long size_mb) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Creating LV %s in VG %s (size: %llu MB)",
                lv_name, vg_name, size_mb);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (vg_name == NULL || lv_name == NULL || size_mb == 0) {
        return ERROR_INVALID_PARAM;
    }
    
    snprintf(cmd, sizeof(cmd), "lvcreate -L %lluM -n %s %s 2>&1",
             size_mb, lv_name, vg_name);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create LV: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Logical Volume created successfully");
    return SUCCESS;
}

/**
 * Extiende un Logical Volume
 */
int lvm_lv_extend(const char *vg_name, const char *lv_name,
                  unsigned long long add_size_mb) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Extending LV %s/%s by %llu MB",
                vg_name, lv_name, add_size_mb);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "lvextend -L +%lluM /dev/%s/%s 2>&1",
             add_size_mb, vg_name, lv_name);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to extend LV: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Logical Volume extended successfully");
    return SUCCESS;
}

/**
 * Lista todos los Logical Volumes
 */
int lvm_lv_list(lv_info_t *lvs, int max_lvs, int *count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    if (lvs == NULL || count == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    snprintf(cmd, sizeof(cmd),
             "lvs --noheadings --units b --nosuffix -o lv_name,vg_name,lv_size,lv_path 2>&1");
    
    int result = execute_command(cmd, output, sizeof(output));
    if (result != SUCCESS) {
        log_message(LOG_WARNING, "No LVs found or error listing LVs");
        return SUCCESS;
    }
    
    char *line = strtok(output, "\n");
    while (line != NULL && *count < max_lvs) {
        while (isspace(*line)) line++;
        
        if (strlen(line) > 0) {
            lv_info_t *lv = &lvs[*count];
            memset(lv, 0, sizeof(lv_info_t));
            
            int parsed = sscanf(line, "%63s %63s %llu %255s",
                               lv->lv_name, lv->vg_name,
                               &lv->size_bytes, lv->lv_path);
            
            if (parsed >= 3) {
                lv->is_active = 1;
                lv->is_snapshot = 0; // Simplificado
                (*count)++;
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    log_message(LOG_INFO, "Found %d Logical Volumes", *count);
    return SUCCESS;
}

/**
 * Obtiene información de un LV específico
 */
int lvm_lv_info(const char *vg_name, const char *lv_name, lv_info_t *info) {
    lv_info_t lvs[64];
    int count;
    
    if (vg_name == NULL || lv_name == NULL || info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    int result = lvm_lv_list(lvs, 64, &count);
    if (result != SUCCESS) {
        return result;
    }
    
    for (int i = 0; i < count; i++) {
        if (strcmp(lvs[i].vg_name, vg_name) == 0 &&
            strcmp(lvs[i].lv_name, lv_name) == 0) {
            memcpy(info, &lvs[i], sizeof(lv_info_t));
            return SUCCESS;
        }
    }
    
    log_message(LOG_ERROR, "Logical Volume not found: %s/%s", vg_name, lv_name);
    return ERROR_NOT_FOUND;
}

/**
 * Remueve un Logical Volume
 */
int lvm_lv_remove(const char *vg_name, const char *lv_name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing LV: %s/%s", vg_name, lv_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "lvremove -f /dev/%s/%s 2>&1", vg_name, lv_name);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to remove LV: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Logical Volume removed successfully");
    return SUCCESS;
}

/* ========== Snapshot Functions ========== */

/**
 * Crea un snapshot de un LV
 */
int lvm_snapshot_create(const char *vg_name, const char *origin_lv,
                        const char *snapshot_name, unsigned long long size_mb) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Creating snapshot %s of %s/%s (size: %llu MB)",
                snapshot_name, vg_name, origin_lv, size_mb);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd),
             "lvcreate -L %lluM -s -n %s /dev/%s/%s 2>&1",
             size_mb, snapshot_name, vg_name, origin_lv);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create snapshot: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Snapshot created successfully");
    return SUCCESS;
}

/**
 * Merge un snapshot
 */
int lvm_snapshot_merge(const char *vg_name, const char *snapshot_name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Merging snapshot: %s/%s", vg_name, snapshot_name);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd),
             "lvconvert --merge /dev/%s/%s 2>&1",
             vg_name, snapshot_name);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to merge snapshot: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Snapshot merged successfully");
    return SUCCESS;
}

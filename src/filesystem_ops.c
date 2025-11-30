#include "../include/filesystem_ops.h"
#include <sys/statvfs.h>
#include <mntent.h>

/* ========== Utilidades ========== */

/**
 * Convierte string a fs_type_t
 */
fs_type_t fs_string_to_type(const char *type_str) {
    if (type_str == NULL) {
        return FS_TYPE_UNKNOWN;
    }
    
    if (strcmp(type_str, "ext4") == 0) return FS_TYPE_EXT4;
    if (strcmp(type_str, "xfs") == 0) return FS_TYPE_XFS;
    if (strcmp(type_str, "btrfs") == 0) return FS_TYPE_BTRFS;
    
    return FS_TYPE_UNKNOWN;
}

/**
 * Convierte fs_type_t a string
 */
const char* fs_type_to_string(fs_type_t type) {
    switch (type) {
        case FS_TYPE_EXT4: return "ext4";
        case FS_TYPE_XFS: return "xfs";
        case FS_TYPE_BTRFS: return "btrfs";
        default: return "unknown";
    }
}

/**
 * Crea un directorio de montaje
 */
int fs_create_mount_point(const char *path) {
    if (path == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    // Verificar si ya existe
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return SUCCESS; // Ya existe
        } else {
            log_message(LOG_ERROR, "Path exists but is not a directory: %s", path);
            return ERROR_GENERIC;
        }
    }
    
    // Crear directorio
    if (mkdir(path, 0755) != 0) {
        log_message(LOG_ERROR, "Failed to create mount point: %s", strerror(errno));
        return ERROR_SYSTEM_CALL;
    }
    
    log_message(LOG_INFO, "Mount point created: %s", path);
    return SUCCESS;
}

/* ========== Creación de Filesystems ========== */

/**
 * Crea un filesystem básico
 */
int fs_create(const char *device, fs_type_t type, const char *label) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    const char *fs_cmd;
    
    log_message(LOG_INFO, "Creating %s filesystem on %s",
                fs_type_to_string(type), device);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (!file_exists(device)) {
        log_message(LOG_ERROR, "Device does not exist: %s", device);
        return ERROR_NOT_FOUND;
    }
    
    // Seleccionar comando según tipo
    switch (type) {
        case FS_TYPE_EXT4:
            fs_cmd = "mkfs.ext4 -F";
            break;
        case FS_TYPE_XFS:
            fs_cmd = "mkfs.xfs -f";
            break;
        case FS_TYPE_BTRFS:
            fs_cmd = "mkfs.btrfs -f";
            break;
        default:
            log_message(LOG_ERROR, "Unsupported filesystem type");
            return ERROR_INVALID_PARAM;
    }
    
    // Construir comando
    if (label != NULL && strlen(label) > 0) {
        if (type == FS_TYPE_EXT4) {
            snprintf(cmd, sizeof(cmd), "%s -L \"%s\" %s 2>&1",
                    fs_cmd, label, device);
        } else if (type == FS_TYPE_XFS) {
            snprintf(cmd, sizeof(cmd), "%s -L \"%s\" %s 2>&1",
                    fs_cmd, label, device);
        } else {
            snprintf(cmd, sizeof(cmd), "%s -L \"%s\" %s 2>&1",
                    fs_cmd, label, device);
        }
    } else {
        snprintf(cmd, sizeof(cmd), "%s %s 2>&1", fs_cmd, device);
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create filesystem: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Filesystem created successfully");
    return SUCCESS;
}

/**
 * Crea ext4 con opciones avanzadas
 */
int fs_create_ext4_advanced(const char *device, int block_size,
                            int inode_ratio, const char *label) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    char opts[256] = "";
    
    log_message(LOG_INFO, "Creating ext4 with advanced options on %s", device);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    // Construir opciones
    if (block_size > 0) {
        snprintf(opts + strlen(opts), sizeof(opts) - strlen(opts),
                "-b %d ", block_size);
    }
    
    if (inode_ratio > 0) {
        snprintf(opts + strlen(opts), sizeof(opts) - strlen(opts),
                "-i %d ", inode_ratio);
    }
    
    if (label != NULL && strlen(label) > 0) {
        snprintf(opts + strlen(opts), sizeof(opts) - strlen(opts),
                "-L \"%s\" ", label);
    }
    
    snprintf(cmd, sizeof(cmd), "mkfs.ext4 -F %s %s 2>&1", opts, device);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create ext4: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "ext4 created successfully with advanced options");
    return SUCCESS;
}

/**
 * Crea xfs con opciones avanzadas
 */
int fs_create_xfs_advanced(const char *device, int block_size, const char *label) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    char opts[256] = "";
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (block_size > 0) {
        snprintf(opts, sizeof(opts), "-b size=%d ", block_size);
    }
    
    if (label != NULL && strlen(label) > 0) {
        snprintf(opts + strlen(opts), sizeof(opts) - strlen(opts),
                "-L \"%s\" ", label);
    }
    
    snprintf(cmd, sizeof(cmd), "mkfs.xfs -f %s %s 2>&1", opts, device);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to create xfs: %s", output);
        return result;
    }
    
    return SUCCESS;
}

/* ========== Montaje y Desmontaje ========== */

/**
 * Monta un filesystem
 */
int fs_mount(const char *device, const char *mount_point,
             fs_type_t type, const char *options) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Mounting %s on %s", device, mount_point);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (!file_exists(device)) {
        log_message(LOG_ERROR, "Device does not exist: %s", device);
        return ERROR_NOT_FOUND;
    }
    
    // Crear punto de montaje si no existe
    fs_create_mount_point(mount_point);
    
    // Construir comando
    if (options != NULL && strlen(options) > 0) {
        snprintf(cmd, sizeof(cmd), "mount -t %s -o %s %s %s 2>&1",
                fs_type_to_string(type), options, device, mount_point);
    } else {
        snprintf(cmd, sizeof(cmd), "mount -t %s %s %s 2>&1",
                fs_type_to_string(type), device, mount_point);
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to mount: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Filesystem mounted successfully");
    return SUCCESS;
}

/**
 * Desmonta un filesystem
 */
int fs_unmount(const char *mount_point, int force) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Unmounting %s", mount_point);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (force) {
        snprintf(cmd, sizeof(cmd), "umount -f %s 2>&1", mount_point);
    } else {
        snprintf(cmd, sizeof(cmd), "umount %s 2>&1", mount_point);
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to unmount: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Filesystem unmounted successfully");
    return SUCCESS;
}

/**
 * Verifica si está montado
 */
int fs_is_mounted(const char *device_or_mount) {
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == NULL) {
        return -1;
    }
    
    struct mntent *mnt;
    int mounted = 0;
    
    while ((mnt = getmntent(fp)) != NULL) {
        if (strcmp(mnt->mnt_fsname, device_or_mount) == 0 ||
            strcmp(mnt->mnt_dir, device_or_mount) == 0) {
            mounted = 1;
            break;
        }
    }
    
    endmntent(fp);
    return mounted;
}

/* ========== Verificación y Reparación ========== */

/**
 * Verifica integridad del filesystem
 */
int fs_check(const char *device, fs_type_t type) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Checking filesystem on %s", device);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    // Verificar que no está montado
    if (fs_is_mounted(device)) {
        log_message(LOG_ERROR, "Device is mounted, cannot check");
        return ERROR_GENERIC;
    }
    
    // Comando según tipo
    switch (type) {
        case FS_TYPE_EXT4:
            snprintf(cmd, sizeof(cmd), "e2fsck -n %s 2>&1", device);
            break;
        case FS_TYPE_XFS:
            snprintf(cmd, sizeof(cmd), "xfs_repair -n %s 2>&1", device);
            break;
        default:
            log_message(LOG_ERROR, "Unsupported filesystem type for check");
            return ERROR_INVALID_PARAM;
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_WARNING, "Filesystem check found issues");
    } else {
        log_message(LOG_INFO, "Filesystem is clean");
    }
    
    return result;
}

/**
 * Repara un filesystem
 */
int fs_repair(const char *device, fs_type_t type, int auto_repair) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Repairing filesystem on %s", device);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    if (fs_is_mounted(device)) {
        log_message(LOG_ERROR, "Device is mounted, cannot repair");
        return ERROR_GENERIC;
    }
    
    switch (type) {
        case FS_TYPE_EXT4:
            if (auto_repair) {
                snprintf(cmd, sizeof(cmd), "e2fsck -y %s 2>&1", device);
            } else {
                snprintf(cmd, sizeof(cmd), "e2fsck -p %s 2>&1", device);
            }
            break;
        case FS_TYPE_XFS:
            snprintf(cmd, sizeof(cmd), "xfs_repair %s 2>&1", device);
            break;
        default:
            return ERROR_INVALID_PARAM;
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Repair failed: %s", output);
    } else {
        log_message(LOG_INFO, "Filesystem repaired successfully");
    }
    
    return result;
}

/* ========== Redimensionamiento ========== */

/**
 * Redimensiona un filesystem
 */
int fs_resize(const char *device, fs_type_t type, unsigned long long new_size_mb) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Resizing %s filesystem on %s",
                fs_type_to_string(type), device);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    switch (type) {
        case FS_TYPE_EXT4:
            // ext4 puede estar montado
            if (new_size_mb == 0) {
                snprintf(cmd, sizeof(cmd), "resize2fs %s 2>&1", device);
            } else {
                snprintf(cmd, sizeof(cmd), "resize2fs %s %lluM 2>&1",
                        device, new_size_mb);
            }
            break;
        case FS_TYPE_XFS:
            log_message(LOG_ERROR, "XFS can only grow, use fs_resize_xfs_online()");
            return ERROR_INVALID_PARAM;
        default:
            return ERROR_INVALID_PARAM;
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Resize failed: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "Filesystem resized successfully");
    return SUCCESS;
}

/**
 * Extiende ext4 online
 */
int fs_resize_ext4_online(const char *device) {
    return fs_resize(device, FS_TYPE_EXT4, 0);
}

/**
 * Extiende xfs online
 */
int fs_resize_xfs_online(const char *mount_point) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Growing XFS on %s", mount_point);
    
    if (!is_root()) {
        return ERROR_PERMISSION;
    }
    
    snprintf(cmd, sizeof(cmd), "xfs_growfs %s 2>&1", mount_point);
    int result = execute_command(cmd, output, sizeof(output));
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "XFS grow failed: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "XFS grown successfully");
    return SUCCESS;
}

/* ========== Información ========== */

/**
 * Obtiene uso de espacio
 */
int fs_get_usage(const char *mount_point, unsigned long long *total,
                 unsigned long long *used, unsigned long long *available) {
    struct statvfs stat;
    
    if (statvfs(mount_point, &stat) != 0) {
        log_message(LOG_ERROR, "Failed to get filesystem stats: %s",
                   strerror(errno));
        return ERROR_SYSTEM_CALL;
    }
    
    if (total) *total = stat.f_blocks * stat.f_frsize;
    if (used) *used = (stat.f_blocks - stat.f_bfree) * stat.f_frsize;
    if (available) *available = stat.f_bavail * stat.f_frsize;
    
    return SUCCESS;
}

/**
 * Obtiene información completa
 */
int fs_get_info(const char *device_or_mount, fs_info_t *info) {
    if (info == NULL) {
        return ERROR_INVALID_PARAM;
    }
    
    memset(info, 0, sizeof(fs_info_t));
    
    // Buscar en /proc/mounts
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == NULL) {
        return ERROR_SYSTEM_CALL;
    }
    
    struct mntent *mnt;
    int found = 0;
    
    while ((mnt = getmntent(fp)) != NULL) {
        if (strcmp(mnt->mnt_fsname, device_or_mount) == 0 ||
            strcmp(mnt->mnt_dir, device_or_mount) == 0) {
            strncpy(info->device, mnt->mnt_fsname, MAX_PATH - 1);
            strncpy(info->mount_point, mnt->mnt_dir, MAX_PATH - 1);
            strncpy(info->type_str, mnt->mnt_type, 15);
            strncpy(info->options, mnt->mnt_opts, 255);
            info->type = fs_string_to_type(mnt->mnt_type);
            info->is_mounted = 1;
            found = 1;
            break;
        }
    }
    
    endmntent(fp);
    
    if (found) {
        fs_get_usage(info->mount_point, &info->total_bytes,
                    &info->used_bytes, &info->available_bytes);
    }
    
    return found ? SUCCESS : ERROR_NOT_FOUND;
}

/**
 * Lista filesystems montados
 */
int fs_list_mounted(fs_info_t *fs_list, int max_fs, int *count) {
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == NULL) {
        return ERROR_SYSTEM_CALL;
    }
    
    *count = 0;
    struct mntent *mnt;
    
    while ((mnt = getmntent(fp)) != NULL && *count < max_fs) {
        fs_info_t *info = &fs_list[*count];
        
        strncpy(info->device, mnt->mnt_fsname, MAX_PATH - 1);
        strncpy(info->mount_point, mnt->mnt_dir, MAX_PATH - 1);
        strncpy(info->type_str, mnt->mnt_type, 15);
        strncpy(info->options, mnt->mnt_opts, 255);
        info->type = fs_string_to_type(mnt->mnt_type);
        info->is_mounted = 1;
        
        fs_get_usage(info->mount_point, &info->total_bytes,
                    &info->used_bytes, &info->available_bytes);
        
        (*count)++;
    }
    
    endmntent(fp);
    return SUCCESS;
}

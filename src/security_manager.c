#include "../include/security_manager.h"
#include <pwd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

/* ========== ACL Functions ========== */

int acl_set(const char *path, const char *user, const char *perms) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Setting ACL for %s on %s: %s", user, path, perms);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    if (!file_exists(path)) {
        log_message(LOG_ERROR, "Path does not exist: %s", path);
        return ERROR_NOT_FOUND;
    }
    
    snprintf(cmd, sizeof(cmd), "setfacl -m u:%s:%s %s 2>&1", user, perms, path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to set ACL: %s", output);
        return ERROR_SYSTEM_CALL;
    }
    
    log_message(LOG_INFO, "ACL set successfully");
    return SUCCESS;
}

int acl_set_default(const char *path, const char *user, const char *perms) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Setting default ACL for %s on %s", user, path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "setfacl -d -m u:%s:%s %s 2>&1", user, perms, path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to set default ACL");
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int acl_get(const char *path, acl_entry_t *entries, int max_entries, int *count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    if (!file_exists(path)) return ERROR_NOT_FOUND;
    
    *count = 0;
    
    snprintf(cmd, sizeof(cmd), "getfacl %s 2>&1", path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        return ERROR_SYSTEM_CALL;
    }
    
    // Parsear salida simplificado
    char *line = strtok(output, "\n");
    while (line != NULL && *count < max_entries) {
        if (strncmp(line, "user:", 5) == 0) {
            acl_entry_t *entry = &entries[*count];
            memset(entry, 0, sizeof(acl_entry_t));
            
            char *user_start = line + 5;
            char *colon = strchr(user_start, ':');
            if (colon) {
                int user_len = colon - user_start;
                if (user_len > 0 && user_len < 64) {
                    strncpy(entry->user, user_start, user_len);
                    entry->user[user_len] = '\0';
                    strncpy(entry->permissions, colon + 1, 15);
                    (*count)++;
                }
            }
        }
        line = strtok(NULL, "\n");
    }
    
    return SUCCESS;
}

int acl_remove(const char *path, const char *user) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing ACL for %s from %s", user, path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "setfacl -x u:%s %s 2>&1", user, path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to remove ACL");
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int acl_remove_all(const char *path) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Removing all ACLs from %s", path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "setfacl -b %s 2>&1", path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int acl_set_recursive(const char *path, const char *user, const char *perms) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Setting ACL recursively for %s on %s", user, path);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "setfacl -R -m u:%s:%s %s 2>&1", user, perms, path);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

/* ========== LUKS Encryption ========== */

int luks_format(const char *device, const char *passphrase) {
    return luks_format_advanced(device, passphrase, NULL, 0);
}

int luks_format_advanced(const char *device, const char *passphrase,
                         const char *cipher, int key_size) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Formatting LUKS on %s", device);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    if (!file_exists(device)) {
        log_message(LOG_ERROR, "Device does not exist");
        return ERROR_NOT_FOUND;
    }
    
    // Crear archivo temporal con contraseña
    char pass_file[] = "/tmp/luks_pass_XXXXXX";
    int fd = mkstemp(pass_file);
    if (fd < 0) return ERROR_SYSTEM_CALL;
    
    write(fd, passphrase, strlen(passphrase));
    close(fd);
    
    // Formatear
    if (cipher && key_size > 0) {
        snprintf(cmd, sizeof(cmd),
                 "cryptsetup luksFormat --batch-mode --key-file %s "
                 "--cipher %s --key-size %d %s 2>&1",
                 pass_file, cipher, key_size, device);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "cryptsetup luksFormat --batch-mode --key-file %s %s 2>&1",
                 pass_file, device);
    }
    
    int result = execute_command(cmd, output, sizeof(output));
    
    // Limpiar archivo de contraseña
    unlink(pass_file);
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to format LUKS: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "LUKS formatted successfully");
    return SUCCESS;
}

int luks_open(const char *device, const char *name, const char *passphrase) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Opening LUKS device %s as %s", device, name);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    // Archivo temporal para contraseña
    char pass_file[] = "/tmp/luks_pass_XXXXXX";
    int fd = mkstemp(pass_file);
    if (fd < 0) return ERROR_SYSTEM_CALL;
    
    write(fd, passphrase, strlen(passphrase));
    close(fd);
    
    snprintf(cmd, sizeof(cmd),
             "cryptsetup luksOpen --key-file %s %s %s 2>&1",
             pass_file, device, name);
    
    int result = execute_command(cmd, output, sizeof(output));
    
    unlink(pass_file);
    
    if (result != SUCCESS) {
        log_message(LOG_ERROR, "Failed to open LUKS: %s", output);
        return result;
    }
    
    log_message(LOG_INFO, "LUKS device opened successfully");
    return SUCCESS;
}

int luks_close(const char *name) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    log_message(LOG_INFO, "Closing LUKS device: %s", name);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    snprintf(cmd, sizeof(cmd), "cryptsetup luksClose %s 2>&1", name);
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        log_message(LOG_ERROR, "Failed to close LUKS");
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int luks_is_luks(const char *device) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    snprintf(cmd, sizeof(cmd), "cryptsetup isLuks %s 2>&1", device);
    
    return (execute_command(cmd, output, sizeof(output)) == SUCCESS) ? 1 : 0;
}

int luks_get_info(const char *device, encrypted_volume_t *info) {
    if (!luks_is_luks(device)) {
        return ERROR_NOT_FOUND;
    }
    
    memset(info, 0, sizeof(encrypted_volume_t));
    strncpy(info->device, device, MAX_PATH - 1);
    
    // Información básica
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    snprintf(cmd, sizeof(cmd), "cryptsetup luksDump %s 2>&1", device);
    execute_command(cmd, output, sizeof(output));
    
    // Parsear salida para obtener cipher
    char *cipher_line = strstr(output, "Cipher:");
    if (cipher_line) {
        sscanf(cipher_line, "Cipher: %63s", info->cipher);
    }
    
    return SUCCESS;
}

int luks_change_password(const char *device, const char *old_pass,
                         const char *new_pass) {
    char cmd[MAX_COMMAND];
    
    log_message(LOG_INFO, "Changing LUKS password on %s", device);
    
    if (!is_root()) return ERROR_PERMISSION;
    
    // Crear archivos temporales
    char old_file[] = "/tmp/luks_old_XXXXXX";
    char new_file[] = "/tmp/luks_new_XXXXXX";
    
    int fd1 = mkstemp(old_file);
    int fd2 = mkstemp(new_file);
    
    if (fd1 < 0 || fd2 < 0) return ERROR_SYSTEM_CALL;
    
    write(fd1, old_pass, strlen(old_pass));
    write(fd2, new_pass, strlen(new_pass));
    close(fd1);
    close(fd2);
    
    snprintf(cmd, sizeof(cmd),
             "cryptsetup luksChangeKey %s --key-file %s %s 2>&1",
             device, old_file, new_file);
    
    int result = execute_command(cmd, NULL, 0);
    
    unlink(old_file);
    unlink(new_file);
    
    return result;
}

int luks_list_open(encrypted_volume_t *volumes, int max_volumes, int *count) {
    char cmd[MAX_COMMAND];
    char output[MAX_OUTPUT];
    
    *count = 0;
    
    snprintf(cmd, sizeof(cmd), "ls -1 /dev/mapper/ 2>&1");
    
    if (execute_command(cmd, output, sizeof(output)) != SUCCESS) {
        return ERROR_SYSTEM_CALL;
    }
    
    char *line = strtok(output, "\n");
    while (line != NULL && *count < max_volumes) {
        if (strcmp(line, "control") != 0) {
            encrypted_volume_t *vol = &volumes[*count];
            memset(vol, 0, sizeof(encrypted_volume_t));
            
            strncpy(vol->name, line, MAX_NAME - 1);
            snprintf(vol->dm_path, MAX_PATH, "/dev/mapper/%s", line);
            vol->is_open = 1;
            
            (*count)++;
        }
        line = strtok(NULL, "\n");
    }
    
    return SUCCESS;
}

/* ========== File Attributes ========== */

int attr_set(const char *path, unsigned int flags) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return ERROR_SYSTEM_CALL;
    
    unsigned long attrs = flags;
    int result = ioctl(fd, FS_IOC_SETFLAGS, &attrs);
    close(fd);
    
    return (result == 0) ? SUCCESS : ERROR_SYSTEM_CALL;
}

int attr_unset(const char *path, unsigned int flags) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return ERROR_SYSTEM_CALL;
    
    unsigned long attrs;
    if (ioctl(fd, FS_IOC_GETFLAGS, &attrs) != 0) {
        close(fd);
        return ERROR_SYSTEM_CALL;
    }
    
    attrs &= ~flags;
    int result = ioctl(fd, FS_IOC_SETFLAGS, &attrs);
    close(fd);
    
    return (result == 0) ? SUCCESS : ERROR_SYSTEM_CALL;
}

int attr_get(const char *path, unsigned int *flags) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return ERROR_SYSTEM_CALL;
    
    unsigned long attrs;
    int result = ioctl(fd, FS_IOC_GETFLAGS, &attrs);
    if (result == 0) {
        *flags = (unsigned int)attrs;
    }
    
    close(fd);
    return (result == 0) ? SUCCESS : ERROR_SYSTEM_CALL;
}

int attr_set_immutable(const char *path) {
    return attr_set(path, ATTR_IMMUTABLE);
}

int attr_set_append_only(const char *path) {
    return attr_set(path, ATTR_APPEND_ONLY);
}

int attr_unset_immutable(const char *path) {
    return attr_unset(path, ATTR_IMMUTABLE);
}

/* ========== Audit Functions ========== */

#define AUDIT_LOG_FILE "/var/log/storage_audit.log"

int audit_log(audit_operation_t operation, const char *user, const char *details) {
    FILE *fp = fopen(AUDIT_LOG_FILE, "a");
    if (fp == NULL) {
        // Intentar crear
        fp = fopen(AUDIT_LOG_FILE, "w");
        if (fp == NULL) return ERROR_SYSTEM_CALL;
    }
    
    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    const char *op_str = "UNKNOWN";
    switch (operation) {
        case AUDIT_RAID_CREATE: op_str = "RAID_CREATE"; break;
        case AUDIT_RAID_MODIFY: op_str = "RAID_MODIFY"; break;
        case AUDIT_LVM_CREATE: op_str = "LVM_CREATE"; break;
        case AUDIT_LVM_MODIFY: op_str = "LVM_MODIFY"; break;
        case AUDIT_FS_MOUNT: op_str = "FS_MOUNT"; break;
        case AUDIT_FS_UNMOUNT: op_str = "FS_UNMOUNT"; break;
        case AUDIT_ENCRYPT: op_str = "ENCRYPT"; break;
        case AUDIT_DECRYPT: op_str = "DECRYPT"; break;
        case AUDIT_ACL_CHANGE: op_str = "ACL_CHANGE"; break;
        case AUDIT_SECURITY_EVENT: op_str = "SECURITY"; break;
    }
    
    fprintf(fp, "[%s] %s by %s: %s\n", time_buf, op_str, user, details);
    fclose(fp);
    
    return SUCCESS;
}

int audit_get_log(char *output, size_t size, int num_entries) {
    FILE *fp = fopen(AUDIT_LOG_FILE, "r");
    if (fp == NULL) {
        strncpy(output, "No audit log found\n", size);
        return SUCCESS;
    }
    
    char line[512];
    output[0] = '\0';
    int count = 0;
    
    while (fgets(line, sizeof(line), fp) && (num_entries == 0 || count < num_entries)) {
        strncat(output, line, size - strlen(output) - 1);
        count++;
    }
    
    fclose(fp);
    return SUCCESS;
}

int audit_clear_log(void) {
    if (!is_root()) return ERROR_PERMISSION;
    
    if (truncate(AUDIT_LOG_FILE, 0) != 0) {
        return ERROR_SYSTEM_CALL;
    }
    
    return SUCCESS;
}

int audit_verify_integrity(void) {
    // Simplified - would need checksums in production
    return file_exists(AUDIT_LOG_FILE) ? SUCCESS : ERROR_NOT_FOUND;
}

/* ========== Utilities ========== */

int security_get_current_user(char *buffer, size_t size) {
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        return ERROR_SYSTEM_CALL;
    }
    
    strncpy(buffer, pw->pw_name, size - 1);
    buffer[size - 1] = '\0';
    
    return SUCCESS;
}

int security_is_root(void) {
    return is_root();
}

void security_attrs_to_string(unsigned int flags, char *buffer, size_t size) {
    buffer[0] = '\0';
    
    if (flags & ATTR_IMMUTABLE) strncat(buffer, "immutable ", size - strlen(buffer) - 1);
    if (flags & ATTR_APPEND_ONLY) strncat(buffer, "append-only ", size - strlen(buffer) - 1);
    if (flags & ATTR_NO_DUMP) strncat(buffer, "no-dump ", size - strlen(buffer) - 1);
    if (flags & ATTR_SECURE_DEL) strncat(buffer, "secure-delete ", size - strlen(buffer) - 1);
    
    if (strlen(buffer) == 0) {
        strncpy(buffer, "none", size - 1);
        buffer[size - 1] = '\0';
    }
}

#include "backup_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

// Silenciar warnings de deprecación
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define BACKUP_DB_PATH "/var/lib/storage_mgr/backups.db"
#define BACKUP_BASE_DIR "/backup"

static sqlite3 *backup_db = NULL;
static pthread_t scheduler_thread = 0;
static int scheduler_active = 0;
static pthread_mutex_t backup_mutex = PTHREAD_MUTEX_INITIALIZER;

// Generar ID único para backup
char* backup_generate_id(void) {
    static char id[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    snprintf(id, sizeof(id), "backup-%04d%02d%02d-%02d%02d%02d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    return id;
}

// Calcular SHA256 checksum
int backup_calculate_checksum(const char *path, char *checksum_out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    unsigned char buffer[8192];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        SHA256_Update(&sha256, buffer, bytes);
    }
    
    SHA256_Final(hash, &sha256);
    fclose(fp);
    
    // Convertir a hexadecimal
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(checksum_out + (i * 2), "%02x", hash[i]);
    }
    checksum_out[64] = '\0';
    
    return 0;
    #pragma GCC diagnostic pop
}

// Calcular tamaño de directorio
unsigned long long backup_get_directory_size(const char *path) {
    unsigned long long total_size = 0;
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "du -sb \"%s\" 2>/dev/null | cut -f1", path);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fscanf(fp, "%llu", &total_size) != 1) {
            total_size = 0;
        }
        pclose(fp);
    }
    
    return total_size;
}

// Inicialización
int backup_init(const char *db_path) {
    int rc;
    const char *sql_create =
        "CREATE TABLE IF NOT EXISTS backups ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "backup_id TEXT UNIQUE NOT NULL,"
        "timestamp INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "source_path TEXT NOT NULL,"
        "dest_path TEXT NOT NULL,"
        "size_bytes INTEGER,"
        "checksum TEXT,"
        "success INTEGER,"
        "error_msg TEXT,"
        "parent_backup_id TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS schedules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "enabled INTEGER DEFAULT 1,"
        "cron_expression TEXT,"
        "type INTEGER,"
        "source TEXT,"
        "destination TEXT,"
        "keep_count INTEGER"
        ");";
    
    system("mkdir -p /var/lib/storage_mgr");
    system("mkdir -p " BACKUP_BASE_DIR);
    
    const char *path = db_path ? db_path : BACKUP_DB_PATH;
    rc = sqlite3_open(path, &backup_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open backup database: %s\n", sqlite3_errmsg(backup_db));
        return -1;
    }
    
    char *err_msg = NULL;
    rc = sqlite3_exec(backup_db, sql_create, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(backup_db);
        return -1;
    }
    
    printf("Backup: Initialized successfully\n");
    return 0;
}

void backup_cleanup(void) {
    if (scheduler_active) {
        backup_stop_scheduler();
    }
    
    if (backup_db) {
        sqlite3_close(backup_db);
        backup_db = NULL;
    }
}

// Crear snapshot LVM
int backup_create_snapshot(const char *vg_name, const char *lv_name,
                          const char *snapshot_name, unsigned long long size_mb) {
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd),
             "lvcreate -L %lluM -s -n %s /dev/%s/%s 2>&1",
             size_mb, snapshot_name, vg_name, lv_name);
    
    printf("Creating LVM snapshot: %s\n", cmd);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return -1;
    }
    
    char output[1024];
    while (fgets(output, sizeof(output), fp)) {
        printf("  %s", output);
    }
    
    int status = pclose(fp);
    if (status != 0) {
        fprintf(stderr, "Failed to create snapshot (exit code: %d)\n", status);
        return -1;
    }
    
    printf("Snapshot created successfully: /dev/%s/%s\n", vg_name, snapshot_name);
    return 0;
}

// Remover snapshot
int backup_remove_snapshot(const char *vg_name, const char *snapshot_name) {
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "lvremove -f /dev/%s/%s 2>&1", vg_name, snapshot_name);
    
    printf("Removing snapshot: %s\n", cmd);
    
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Failed to remove snapshot\n");
        return -1;
    }
    
    printf("Snapshot removed successfully\n");
    return 0;
}

// Montar snapshot
int backup_mount_snapshot(const char *vg_name, const char *snapshot_name,
                         const char *mount_point) {
    char cmd[512];
    
    // Crear punto de montaje si no existe
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", mount_point);
    system(cmd);
    
    // Montar snapshot
    snprintf(cmd, sizeof(cmd), "mount /dev/%s/%s \"%s\" 2>&1",
             vg_name, snapshot_name, mount_point);
    
    printf("Mounting snapshot: %s\n", cmd);
    
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Failed to mount snapshot\n");
        return -1;
    }
    
    printf("Snapshot mounted at %s\n", mount_point);
    return 0;
}

// Desmontar snapshot
int backup_unmount_snapshot(const char *mount_point) {
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "umount \"%s\" 2>&1", mount_point);
    
    printf("Unmounting snapshot: %s\n", cmd);
    
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Warning: Failed to unmount snapshot\n");
        return -1;
    }
    
    printf("Snapshot unmounted successfully\n");
    return 0;
}

// Crear backup (full o incremental)
int backup_create(const char *source, const char *dest, backup_type_t type) {
    backup_info_t info;
    char cmd[2048];
    char dest_path[512];
    time_t now = time(NULL);
    
    memset(&info, 0, sizeof(info));
    strcpy(info.backup_id, backup_generate_id());
    info.timestamp = now;
    info.type = type;
    strncpy(info.source_path, source, sizeof(info.source_path) - 1);
    
    // Crear directorio de destino
    snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, info.backup_id);
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", dest_path);
    system(cmd);
    
    strncpy(info.dest_path, dest_path, sizeof(info.dest_path) - 1);
    
    printf("\n=== Starting Backup ===\n");
    printf("ID:     %s\n", info.backup_id);
    printf("Type:   %s\n", type == BACKUP_FULL ? "FULL" : 
           type == BACKUP_INCREMENTAL ? "INCREMENTAL" : "DIFFERENTIAL");
    printf("Source: %s\n", source);
    printf("Dest:   %s\n", dest_path);
    
    // Construir comando rsync según el tipo
    if (type == BACKUP_FULL) {
        snprintf(cmd, sizeof(cmd),
                 "rsync -av --stats \"%s/\" \"%s/\" 2>&1",
                 source, dest_path);
    } else if (type == BACKUP_INCREMENTAL) {
        // Buscar último backup para link-dest
        backup_info_t *backups = NULL;
        int count = 0;
        
        if (backup_list(&backups, &count) == 0 && count > 0) {
            // Usar el backup más reciente como referencia
            strncpy(info.parent_backup_id, backups[0].backup_id,
                   sizeof(info.parent_backup_id) - 1);
            
            snprintf(cmd, sizeof(cmd),
                     "rsync -av --stats --link-dest=\"%s\" \"%s/\" \"%s/\" 2>&1",
                     backups[0].dest_path, source, dest_path);
            
            free(backups);
        } else {
            // Si no hay backup previo, hacer full
            printf("No previous backup found, performing full backup\n");
            snprintf(cmd, sizeof(cmd),
                     "rsync -av --stats \"%s/\" \"%s/\" 2>&1",
                     source, dest_path);
        }
    }
    
    printf("\nExecuting: %s\n\n", cmd);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        info.success = 0;
        snprintf(info.error_msg, sizeof(info.error_msg), "Failed to execute rsync");
        goto save_info;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    int status = pclose(fp);
    if (status == 0) {
        info.success = 1;
        printf("\nBackup completed successfully!\n");
    } else {
        info.success = 0;
        snprintf(info.error_msg, sizeof(info.error_msg), 
                 "rsync failed with exit code %d", status);
        fprintf(stderr, "\nBackup failed!\n");
    }
    
    // Calcular tamaño y checksum
    info.size_bytes = backup_get_directory_size(dest_path);
    printf("Backup size: %.2f MB\n", info.size_bytes / (1024.0 * 1024.0));
    
save_info:
    // Guardar info en base de datos
    if (backup_db) {
        const char *sql = "INSERT INTO backups "
                         "(backup_id, timestamp, type, source_path, dest_path, "
                         "size_bytes, checksum, success, error_msg, parent_backup_id) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(backup_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, info.backup_id, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 2, info.timestamp);
            sqlite3_bind_int(stmt, 3, info.type);
            sqlite3_bind_text(stmt, 4, info.source_path, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, info.dest_path, -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 6, info.size_bytes);
            sqlite3_bind_text(stmt, 7, info.checksum, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 8, info.success);
            sqlite3_bind_text(stmt, 9, info.error_msg, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 10, info.parent_backup_id, -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    
    return info.success ? 0 : -1;
}

// Crear backup con snapshot
int backup_create_with_snapshot(const char *vg_name, const char *lv_name,
                                const char *source, const char *dest,
                                backup_type_t type) {
    char snapshot_name[128];
    char mount_point[256];
    int result = -1;
    
    snprintf(snapshot_name, sizeof(snapshot_name), "%s_snap_%ld", lv_name, time(NULL));
    snprintf(mount_point, sizeof(mount_point), "/mnt/backup_snapshot_%ld", time(NULL));
    
    printf("Creating snapshot for consistent backup...\n");
    
    // Crear snapshot (500MB por defecto)
    if (backup_create_snapshot(vg_name, lv_name, snapshot_name, 500) != 0) {
        return -1;
    }
    
    // Montar snapshot
    if (backup_mount_snapshot(vg_name, snapshot_name, mount_point) != 0) {
        backup_remove_snapshot(vg_name, snapshot_name);
        return -1;
    }
    
    // Realizar backup desde el snapshot
    result = backup_create(mount_point, dest, type);
    
    // Limpiar
    backup_unmount_snapshot(mount_point);
    backup_remove_snapshot(vg_name, snapshot_name);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rmdir \"%s\"", mount_point);
    system(cmd);
    
    return result;
}

// Listar backups
int backup_list(backup_info_t **backups, int *count) {
    if (!backup_db) {
        return -1;
    }
    
    const char *sql = "SELECT backup_id, timestamp, type, source_path, dest_path, "
                     "size_bytes, checksum, success, error_msg, parent_backup_id "
                     "FROM backups ORDER BY timestamp DESC;";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(backup_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    // Contar resultados
    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }
    
    if (*count == 0) {
        sqlite3_finalize(stmt);
        *backups = NULL;
        return 0;
    }
    
    // Leer datos
    sqlite3_reset(stmt);
    *backups = malloc(*count * sizeof(backup_info_t));
    if (!*backups) {
        sqlite3_finalize(stmt);
        return -ENOMEM;
    }
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        const char *text;
        
        text = (const char*)sqlite3_column_text(stmt, 0);
        strncpy((*backups)[i].backup_id, text, sizeof((*backups)[i].backup_id) - 1);
        
        (*backups)[i].timestamp = sqlite3_column_int64(stmt, 1);
        (*backups)[i].type = sqlite3_column_int(stmt, 2);
        
        text = (const char*)sqlite3_column_text(stmt, 3);
        strncpy((*backups)[i].source_path, text, sizeof((*backups)[i].source_path) - 1);
        
        text = (const char*)sqlite3_column_text(stmt, 4);
        strncpy((*backups)[i].dest_path, text, sizeof((*backups)[i].dest_path) - 1);
        
        (*backups)[i].size_bytes = sqlite3_column_int64(stmt, 5);
        
        text = (const char*)sqlite3_column_text(stmt, 6);
        if (text)
            strncpy((*backups)[i].checksum, text, sizeof((*backups)[i].checksum) - 1);
        
        (*backups)[i].success = sqlite3_column_int(stmt, 7);
        
        text = (const char*)sqlite3_column_text(stmt, 8);
        if (text)
            strncpy((*backups)[i].error_msg, text, sizeof((*backups)[i].error_msg) - 1);
        
        text = (const char*)sqlite3_column_text(stmt, 9);
        if (text)
            strncpy((*backups)[i].parent_backup_id, text, 
                   sizeof((*backups)[i].parent_backup_id) - 1);
        
        i++;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

// Obtener info de backup específico
int backup_get_info(const char *backup_id, backup_info_t *info) {
    if (!backup_db || !backup_id || !info) {
        return -1;
    }
    
    const char *sql = "SELECT timestamp, type, source_path, dest_path, "
                     "size_bytes, checksum, success, error_msg, parent_backup_id "
                     "FROM backups WHERE backup_id = ?;";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(backup_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, backup_id, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    memset(info, 0, sizeof(backup_info_t));
    strncpy(info->backup_id, backup_id, sizeof(info->backup_id) - 1);
    info->timestamp = sqlite3_column_int64(stmt, 0);
    info->type = sqlite3_column_int(stmt, 1);
    
    const char *text = (const char*)sqlite3_column_text(stmt, 2);
    strncpy(info->source_path, text, sizeof(info->source_path) - 1);
    
    text = (const char*)sqlite3_column_text(stmt, 3);
    strncpy(info->dest_path, text, sizeof(info->dest_path) - 1);
    
    info->size_bytes = sqlite3_column_int64(stmt, 4);
    
    text = (const char*)sqlite3_column_text(stmt, 5);
    if (text)
        strncpy(info->checksum, text, sizeof(info->checksum) - 1);
    
    info->success = sqlite3_column_int(stmt, 6);
    
    text = (const char*)sqlite3_column_text(stmt, 7);
    if (text)
        strncpy(info->error_msg, text, sizeof(info->error_msg) - 1);
    
    sqlite3_finalize(stmt);
    return 0;
}

// Verificar backup
int backup_verify(const char *backup_id) {
    backup_info_t info;
    
    if (backup_get_info(backup_id, &info) != 0) {
        fprintf(stderr, "Backup not found: %s\n", backup_id);
        return -1;
    }
    
    printf("Verifying backup: %s\n", backup_id);
    printf("Path: %s\n", info.dest_path);
    
    // Verificar que el directorio existe
    struct stat st;
    if (stat(info.dest_path, &st) != 0) {
        fprintf(stderr, "Backup directory not found!\n");
        return -1;
    }
    
    // Verificar tamaño
    unsigned long long current_size = backup_get_directory_size(info.dest_path);
    printf("Recorded size: %.2f MB\n", info.size_bytes / (1024.0 * 1024.0));
    printf("Current size:  %.2f MB\n", current_size / (1024.0 * 1024.0));
    
    if (current_size == 0) {
        fprintf(stderr, "Warning: Backup appears to be empty!\n");
        return -1;
    }
    
    printf("Backup verification passed!\n");
    return 0;
}

// Restaurar backup
int backup_restore(const char *backup_id, const char *dest) {
    backup_info_t info;
    char cmd[2048];
    
    if (backup_get_info(backup_id, &info) != 0) {
        fprintf(stderr, "Backup not found: %s\n", backup_id);
        return -1;
    }
    
    printf("\n=== Restoring Backup ===\n");
    printf("Backup ID: %s\n", backup_id);
    printf("From:      %s\n", info.dest_path);
    printf("To:        %s\n", dest);
    
    // Crear directorio de destino
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", dest);
    system(cmd);
    
    // Usar rsync para restaurar
    snprintf(cmd, sizeof(cmd),
             "rsync -av --stats \"%s/\" \"%s/\" 2>&1",
             info.dest_path, dest);
    
    printf("\nExecuting: %s\n\n", cmd);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen");
        return -1;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    int status = pclose(fp);
    if (status != 0) {
        fprintf(stderr, "\nRestore failed!\n");
        return -1;
    }
    
    printf("\nRestore completed successfully!\n");
    return 0;
}

// Eliminar backup viejo
int backup_cleanup_old(int keep_count) {
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_list(&backups, &count) != 0) {
        return -1;
    }
    
    if (count <= keep_count) {
        free(backups);
        printf("No backups to clean up (have %d, keep %d)\n", count, keep_count);
        return 0;
    }
    
    printf("Cleaning up old backups (have %d, keep %d)\n", count, keep_count);
    
    // Eliminar los más antiguos
    for (int i = keep_count; i < count; i++) {
        printf("Removing backup: %s\n", backups[i].backup_id);
        
        // Eliminar directorio
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", backups[i].dest_path);
        system(cmd);
        
        // Eliminar de DB
        if (backup_db) {
            const char *sql = "DELETE FROM backups WHERE backup_id = ?;";
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(backup_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, backups[i].backup_id, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }
    
    free(backups);
    printf("Cleanup completed\n");
    return 0;
}

// Thread de scheduler (simplificado)
void* backup_scheduler_thread(void *arg) {
    printf("Backup scheduler thread started\n");
    
    while (scheduler_active) {
        // En una implementación real, aquí se verificarían
        // las expresiones cron y se ejecutarían backups programados
        sleep(60);  // Revisar cada minuto
    }
    
    printf("Backup scheduler thread stopped\n");
    return NULL;
}

int backup_start_scheduler(void) {
    pthread_mutex_lock(&backup_mutex);
    
    if (scheduler_active) {
        pthread_mutex_unlock(&backup_mutex);
        return -1;
    }
    
    scheduler_active = 1;
    
    if (pthread_create(&scheduler_thread, NULL, backup_scheduler_thread, NULL) != 0) {
        scheduler_active = 0;
        pthread_mutex_unlock(&backup_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&backup_mutex);
    printf("Backup scheduler started\n");
    return 0;
}

int backup_stop_scheduler(void) {
    pthread_mutex_lock(&backup_mutex);
    
    if (!scheduler_active) {
        pthread_mutex_unlock(&backup_mutex);
        return 0;
    }
    
    scheduler_active = 0;
    pthread_mutex_unlock(&backup_mutex);
    
    if (scheduler_thread) {
        pthread_join(scheduler_thread, NULL);
        scheduler_thread = 0;
    }
    
    printf("Backup scheduler stopped\n");
    return 0;
}

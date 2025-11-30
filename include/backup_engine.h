#ifndef BACKUP_ENGINE_H
#define BACKUP_ENGINE_H

#include <time.h>
#include <stdint.h>

// Tipos de backup
typedef enum {
    BACKUP_FULL,
    BACKUP_INCREMENTAL,
    BACKUP_DIFFERENTIAL
} backup_type_t;

// Información de backup
typedef struct {
    char backup_id[64];
    time_t timestamp;
    backup_type_t type;
    char source_path[256];
    char dest_path[256];
    unsigned long long size_bytes;
    char checksum[65];  // SHA256
    int success;
    char error_msg[256];
    char parent_backup_id[64];  // Para incrementales
} backup_info_t;

// Configuración de schedule
typedef struct {
    int enabled;
    char cron_expression[128];
    backup_type_t type;
    char source[256];
    char destination[256];
    int keep_count;
} backup_schedule_t;

// Inicialización
int backup_init(const char *db_path);
void backup_cleanup(void);

// Operaciones de backup
int backup_create(const char *source, const char *dest, backup_type_t type);
int backup_create_with_snapshot(const char *vg_name, const char *lv_name,
                                 const char *source, const char *dest,
                                 backup_type_t type);

// Gestión de snapshots LVM
int backup_create_snapshot(const char *vg_name, const char *lv_name,
                           const char *snapshot_name, unsigned long long size_mb);
int backup_remove_snapshot(const char *vg_name, const char *snapshot_name);
int backup_mount_snapshot(const char *vg_name, const char *snapshot_name,
                          const char *mount_point);
int backup_unmount_snapshot(const char *mount_point);

// Verificación y restauración
int backup_verify(const char *backup_id);
int backup_restore(const char *backup_id, const char *dest);
int backup_restore_file(const char *backup_id, const char *file_path, 
                        const char *dest);

// Gestión de backups
int backup_list(backup_info_t **backups, int *count);
int backup_get_info(const char *backup_id, backup_info_t *info);
int backup_delete(const char *backup_id);
int backup_cleanup_old(int keep_count);

// Scheduling
int backup_schedule_add(const backup_schedule_t *schedule);
int backup_schedule_list(backup_schedule_t **schedules, int *count);
int backup_schedule_remove(int schedule_id);
int backup_schedule_run(void);  // Ejecutar backups programados

// Utilidades
char* backup_generate_id(void);
int backup_calculate_checksum(const char *path, char *checksum_out);
unsigned long long backup_get_directory_size(const char *path);

// Thread de scheduling
void* backup_scheduler_thread(void *arg);
int backup_start_scheduler(void);
int backup_stop_scheduler(void);

#endif // BACKUP_ENGINE_H

#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>
#include <stdint.h>

// Estructura para estadísticas de dispositivo
typedef struct {
    char device[64];
    unsigned long long reads;
    unsigned long long writes;
    unsigned long long read_bytes;
    unsigned long long write_bytes;
    double avg_read_latency_ms;
    double avg_write_latency_ms;
    int queue_depth;
    time_t last_update;
} device_stats_t;

// Estructura para muestra de rendimiento
typedef struct {
    time_t timestamp;
    double iops;
    double throughput_mbs;
    double latency_ms;
    int active_requests;
} performance_sample_t;

// Estructura para uso de disco
typedef struct {
    char mount_point[256];
    char device[64];
    unsigned long long total_bytes;
    unsigned long long used_bytes;
    unsigned long long available_bytes;
    double usage_percent;
    unsigned long long total_inodes;
    unsigned long long used_inodes;
    unsigned long long free_inodes;
} disk_usage_t;

// Estructura para archivos abiertos
typedef struct {
    char path[512];
    pid_t pid;
    char process_name[256];
    int fd;
    char mode[8];
} open_file_t;

// Inicialización del sistema de monitoreo
int monitor_init(void);
void monitor_cleanup(void);

// Funciones de estadísticas de dispositivos
int monitor_get_device_stats(const char *device, device_stats_t *stats);
int monitor_get_io_stats(const char *device, device_stats_t *stats);
int monitor_reset_stats(const char *device);

// Funciones de rendimiento
int monitor_track_performance(const char *device, performance_sample_t *sample);
int monitor_get_current_performance(const char *device, performance_sample_t *sample);

// Funciones de uso de disco
int monitor_get_disk_usage(const char *mount_point, disk_usage_t *usage);
int monitor_check_disk_space(const char *mount_point, double threshold);

// Funciones de archivos abiertos
int monitor_list_open_files(const char *mount_point, open_file_t **files, int *count);
int monitor_get_process_io(pid_t pid, device_stats_t *stats);

// Funciones de datos históricos
int monitor_save_sample(const char *device, const performance_sample_t *sample);
int monitor_get_history(const char *device, time_t start, time_t end, 
                        performance_sample_t **samples, int *count);
int monitor_cleanup_old_data(int keep_days);

// Funciones de reportes
int monitor_generate_report(const char *output_file, time_t start, time_t end);
void monitor_print_stats(const device_stats_t *stats);
void monitor_print_performance(const performance_sample_t *sample);

// Thread de monitoreo continuo
void* monitor_thread_func(void *arg);
int monitor_start_continuous(int interval_seconds);
int monitor_stop_continuous(void);

#endif // MONITOR_H
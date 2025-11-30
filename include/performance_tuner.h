#ifndef PERFORMANCE_TUNER_H
#define PERFORMANCE_TUNER_H

#include <stdint.h>
#include <stddef.h>

// Tipos de workload
typedef enum {
    WORKLOAD_DATABASE,
    WORKLOAD_WEB_SERVER,
    WORKLOAD_FILE_SERVER,
    WORKLOAD_GENERAL,
    WORKLOAD_RANDOM_IO,
    WORKLOAD_SEQUENTIAL_IO
} workload_type_t;

// Schedulers de I/O disponibles
typedef enum {
    SCHED_NOOP,
    SCHED_DEADLINE,
    SCHED_CFQ,
    SCHED_BFQ,
    SCHED_KYBER,
    SCHED_MQ_DEADLINE
} io_scheduler_t;

// Perfil de optimización
typedef struct {
    char scheduler[32];
    int read_ahead_kb;
    int queue_depth;
    int nr_requests;
    int vm_swappiness;
    int vm_dirty_ratio;
    int vm_dirty_background_ratio;
    int vm_vfs_cache_pressure;
} tuning_profile_t;

// Resultados de benchmark
typedef struct {
    double seq_read_mbs;
    double seq_write_mbs;
    double rand_read_iops;
    double rand_write_iops;
    double avg_latency_ms;
    double p95_latency_ms;
    double p99_latency_ms;
} benchmark_results_t;

// Inicialización
int perf_init(void);
void perf_cleanup(void);

// Gestión de schedulers
int perf_set_scheduler(const char *device, const char *scheduler);
int perf_get_scheduler(const char *device, char *scheduler, size_t size);
int perf_list_schedulers(const char *device, char ***schedulers, int *count);

// Configuración de dispositivos
int perf_set_readahead(const char *device, int size_kb);
int perf_get_readahead(const char *device);
int perf_set_queue_depth(const char *device, int depth);
int perf_get_queue_depth(const char *device);
int perf_set_nr_requests(const char *device, int requests);

// Parámetros del kernel
int perf_set_vm_swappiness(int value);
int perf_get_vm_swappiness(void);
int perf_set_vm_dirty_ratio(int value);
int perf_get_vm_dirty_ratio(void);
int perf_set_vm_dirty_background_ratio(int value);
int perf_set_vm_vfs_cache_pressure(int value);

// Benchmarking
int perf_benchmark(const char *device, const char *test_file, 
                   benchmark_results_t *results);
int perf_benchmark_sequential_read(const char *file, double *mbs);
int perf_benchmark_sequential_write(const char *file, double *mbs);
int perf_benchmark_random_read(const char *file, double *iops);
int perf_benchmark_random_write(const char *file, double *iops);

// Perfiles y recomendaciones
int perf_get_default_profile(workload_type_t workload, tuning_profile_t *profile);
int perf_recommend(const char *device, workload_type_t workload,
                   tuning_profile_t *profile);
int perf_apply_profile(const char *device, const tuning_profile_t *profile);
int perf_save_profile(const char *name, const tuning_profile_t *profile);
int perf_load_profile(const char *name, tuning_profile_t *profile);

// Comparación de resultados
int perf_compare_benchmarks(const benchmark_results_t *before,
                           const benchmark_results_t *after,
                           char *report, size_t report_size);

// Información del sistema
int perf_get_device_info(const char *device, char *info, size_t info_size);
int perf_check_dma_status(const char *device);

// Utilidades
const char* perf_workload_to_string(workload_type_t workload);
const char* perf_scheduler_to_string(io_scheduler_t scheduler);

#endif // PERFORMANCE_TUNER_H

#include "performance_tuner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifndef O_DIRECT
#define O_DIRECT 040000  // ← AGREGAR ESTAS LÍNEAS
#endif

#define SYSFS_BLOCK_PATH "/sys/block"
#define SYSCTL_PATH "/proc/sys"
#define TEST_FILE_SIZE (100 * 1024 * 1024)  // 100 MB

// Inicialización
int perf_init(void) {
    printf("Performance Tuner: Initialized\n");
    return 0;
}

void perf_cleanup(void) {
    printf("Performance Tuner: Cleanup\n");
}

// Obtener nombre base del dispositivo (sin /dev/)
static const char* get_device_basename(const char *device) {
    const char *base = strrchr(device, '/');
    return base ? base + 1 : device;
}

// Establecer scheduler de I/O
int perf_set_scheduler(const char *device, const char *scheduler) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/queue/scheduler", SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen scheduler");
        return -1;
    }
    
    fprintf(fp, "%s", scheduler);
    fclose(fp);
    
    printf("Set I/O scheduler to '%s' for %s\n", scheduler, device);
    return 0;
}

// Obtener scheduler actual
int perf_get_scheduler(const char *device, char *scheduler, size_t size) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/queue/scheduler", SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("fopen scheduler");
        return -1;
    }
    
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // El scheduler actual está entre corchetes: [mq-deadline] none
    char *start = strchr(line, '[');
    char *end = strchr(line, ']');
    
    if (start && end && end > start) {
        size_t len = end - start - 1;
        if (len >= size) len = size - 1;
        strncpy(scheduler, start + 1, len);
        scheduler[len] = '\0';
    } else {
        strncpy(scheduler, "unknown", size - 1);
    }
    
    return 0;
}

// Listar schedulers disponibles
int perf_list_schedulers(const char *device, char ***schedulers, int *count) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/queue/scheduler", SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // Parsear schedulers: [mq-deadline] none kyber bfq
    *count = 0;
    char *token = strtok(line, " \t\n[]");
    while (token && *count < 10) {
        (*count)++;
        token = strtok(NULL, " \t\n[]");
    }
    
    // Implementación simplificada
    printf("Available schedulers for %s: %s", device, line);
    return 0;
}

// Establecer read-ahead
int perf_set_readahead(const char *device, int size_kb) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/queue/read_ahead_kb", 
             SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen read_ahead_kb");
        return -1;
    }
    
    fprintf(fp, "%d", size_kb);
    fclose(fp);
    
    printf("Set read-ahead to %d KB for %s\n", size_kb, device);
    return 0;
}

// Obtener read-ahead actual
int perf_get_readahead(const char *device) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    int value = -1;
    
    snprintf(path, sizeof(path), "%s/%s/queue/read_ahead_kb",
             SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "r");
    if (fp) {
        fscanf(fp, "%d", &value);
        fclose(fp);
    }
    
    return value;
}

// Establecer profundidad de cola
int perf_set_queue_depth(const char *device, int depth) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/device/queue_depth",
             SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        // Intentar ruta alternativa
        snprintf(path, sizeof(path), "%s/%s/queue/nr_requests",
                 SYSFS_BLOCK_PATH, dev_name);
        fp = fopen(path, "w");
        if (!fp) {
            perror("fopen queue_depth");
            return -1;
        }
    }
    
    fprintf(fp, "%d", depth);
    fclose(fp);
    
    printf("Set queue depth to %d for %s\n", depth, device);
    return 0;
}

// Obtener profundidad de cola
int perf_get_queue_depth(const char *device) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    int value = -1;
    
    snprintf(path, sizeof(path), "%s/%s/device/queue_depth",
             SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        snprintf(path, sizeof(path), "%s/%s/queue/nr_requests",
                 SYSFS_BLOCK_PATH, dev_name);
        fp = fopen(path, "r");
    }
    
    if (fp) {
        fscanf(fp, "%d", &value);
        fclose(fp);
    }
    
    return value;
}

// Establecer nr_requests
int perf_set_nr_requests(const char *device, int requests) {
    char path[256];
    const char *dev_name = get_device_basename(device);
    
    snprintf(path, sizeof(path), "%s/%s/queue/nr_requests",
             SYSFS_BLOCK_PATH, dev_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen nr_requests");
        return -1;
    }
    
    fprintf(fp, "%d", requests);
    fclose(fp);
    
    printf("Set nr_requests to %d for %s\n", requests, device);
    return 0;
}

// Parámetros VM del kernel
int perf_set_vm_swappiness(int value) {
    FILE *fp = fopen("/proc/sys/vm/swappiness", "w");
    if (!fp) {
        perror("fopen swappiness");
        return -1;
    }
    
    fprintf(fp, "%d", value);
    fclose(fp);
    
    printf("Set vm.swappiness to %d\n", value);
    return 0;
}

int perf_get_vm_swappiness(void) {
    FILE *fp = fopen("/proc/sys/vm/swappiness", "r");
    int value = -1;
    
    if (fp) {
        fscanf(fp, "%d", &value);
        fclose(fp);
    }
    
    return value;
}

int perf_set_vm_dirty_ratio(int value) {
    FILE *fp = fopen("/proc/sys/vm/dirty_ratio", "w");
    if (!fp) {
        perror("fopen dirty_ratio");
        return -1;
    }
    
    fprintf(fp, "%d", value);
    fclose(fp);
    
    printf("Set vm.dirty_ratio to %d\n", value);
    return 0;
}

int perf_get_vm_dirty_ratio(void) {
    FILE *fp = fopen("/proc/sys/vm/dirty_ratio", "r");
    int value = -1;
    
    if (fp) {
        fscanf(fp, "%d", &value);
        fclose(fp);
    }
    
    return value;
}

int perf_set_vm_dirty_background_ratio(int value) {
    FILE *fp = fopen("/proc/sys/vm/dirty_background_ratio", "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "%d", value);
    fclose(fp);
    
    printf("Set vm.dirty_background_ratio to %d\n", value);
    return 0;
}

int perf_set_vm_vfs_cache_pressure(int value) {
    FILE *fp = fopen("/proc/sys/vm/vfs_cache_pressure", "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "%d", value);
    fclose(fp);
    
    printf("Set vm.vfs_cache_pressure to %d\n", value);
    return 0;
}

// Benchmark: lectura secuencial
int perf_benchmark_sequential_read(const char *file, double *mbs) {
    int fd = open(file, O_RDONLY | O_DIRECT);
    if (fd < 0) {
        // Si O_DIRECT falla, intentar sin él
        fd = open(file, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return -1;
        }
    }
    
    const size_t buffer_size = 1024 * 1024;  // 1 MB
    void *buffer = aligned_alloc(4096, buffer_size);
    if (!buffer) {
        close(fd);
        return -ENOMEM;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    ssize_t total_read = 0;
    ssize_t bytes;
    
    while ((bytes = read(fd, buffer, buffer_size)) > 0) {
        total_read += bytes;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    
    *mbs = (total_read / (1024.0 * 1024.0)) / elapsed;
    
    free(buffer);
    close(fd);
    
    printf("Sequential Read: %.2f MB/s (%.2f MB in %.2f seconds)\n",
           *mbs, total_read / (1024.0 * 1024.0), elapsed);
    
    return 0;
}

// Benchmark: escritura secuencial
int perf_benchmark_sequential_write(const char *file, double *mbs) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0644);
    if (fd < 0) {
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            return -1;
        }
    }
    
    const size_t buffer_size = 1024 * 1024;  // 1 MB
    void *buffer = aligned_alloc(4096, buffer_size);
    if (!buffer) {
        close(fd);
        return -ENOMEM;
    }
    
    memset(buffer, 0xAA, buffer_size);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    size_t total_written = 0;
    size_t target_size = TEST_FILE_SIZE;
    
    while (total_written < target_size) {
        ssize_t bytes = write(fd, buffer, buffer_size);
        if (bytes < 0) {
            perror("write");
            break;
        }
        total_written += bytes;
    }
    
    fsync(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    
    *mbs = (total_written / (1024.0 * 1024.0)) / elapsed;
    
    free(buffer);
    close(fd);
    
    printf("Sequential Write: %.2f MB/s (%.2f MB in %.2f seconds)\n",
           *mbs, total_written / (1024.0 * 1024.0), elapsed);
    
    return 0;
}

// Benchmark completo
int perf_benchmark(const char *device, const char *test_file,
                  benchmark_results_t *results) {
    char read_file[512], write_file[512];
    
    printf("\n=== Performance Benchmark ===\n");
    printf("Device: %s\n", device);
    printf("Test file: %s\n\n", test_file);
    
    memset(results, 0, sizeof(benchmark_results_t));
    
    // Crear archivo de prueba para lectura
    snprintf(write_file, sizeof(write_file), "%s.write_test", test_file);
    snprintf(read_file, sizeof(read_file), "%s.read_test", test_file);
    
    // Test: escritura secuencial
    printf("Running sequential write test...\n");
    if (perf_benchmark_sequential_write(write_file, &results->seq_write_mbs) == 0) {
        printf("✓ Write test completed\n\n");
    }
    
    // Copiar archivo para test de lectura
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", write_file, read_file);
    system(cmd);
    
    // Limpiar cache
    system("sync");
    system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
    sleep(1);
    
    // Test: lectura secuencial
    printf("Running sequential read test...\n");
    if (perf_benchmark_sequential_read(read_file, &results->seq_read_mbs) == 0) {
        printf("✓ Read test completed\n\n");
    }
    
    // Cleanup
    unlink(write_file);
    unlink(read_file);
    
    // Resumen
    printf("=== Benchmark Results ===\n");
    printf("Sequential Read:  %.2f MB/s\n", results->seq_read_mbs);
    printf("Sequential Write: %.2f MB/s\n", results->seq_write_mbs);
    
    return 0;
}

// Obtener perfil por defecto según workload
int perf_get_default_profile(workload_type_t workload, tuning_profile_t *profile) {
    memset(profile, 0, sizeof(tuning_profile_t));
    
    switch (workload) {
        case WORKLOAD_DATABASE:
            strcpy(profile->scheduler, "deadline");
            profile->read_ahead_kb = 256;
            profile->queue_depth = 128;
            profile->nr_requests = 256;
            profile->vm_swappiness = 10;
            profile->vm_dirty_ratio = 15;
            profile->vm_dirty_background_ratio = 5;
            profile->vm_vfs_cache_pressure = 50;
            break;
            
        case WORKLOAD_WEB_SERVER:
            strcpy(profile->scheduler, "deadline");
            profile->read_ahead_kb = 512;
            profile->queue_depth = 64;
            profile->nr_requests = 128;
            profile->vm_swappiness = 10;
            profile->vm_dirty_ratio = 20;
            profile->vm_dirty_background_ratio = 10;
            profile->vm_vfs_cache_pressure = 100;
            break;
            
        case WORKLOAD_FILE_SERVER:
            strcpy(profile->scheduler, "cfq");
            profile->read_ahead_kb = 2048;
            profile->queue_depth = 64;
            profile->nr_requests = 128;
            profile->vm_swappiness = 1;
            profile->vm_dirty_ratio = 40;
            profile->vm_dirty_background_ratio = 10;
            profile->vm_vfs_cache_pressure = 50;
            break;
            
        default:  // WORKLOAD_GENERAL
            strcpy(profile->scheduler, "mq-deadline");
            profile->read_ahead_kb = 128;
            profile->queue_depth = 32;
            profile->nr_requests = 128;
            profile->vm_swappiness = 60;
            profile->vm_dirty_ratio = 20;
            profile->vm_dirty_background_ratio = 10;
            profile->vm_vfs_cache_pressure = 100;
            break;
    }
    
    return 0;
}

// Aplicar perfil de optimización
int perf_apply_profile(const char *device, const tuning_profile_t *profile) {
    printf("\n=== Applying Performance Profile ===\n");
    printf("Device: %s\n\n", device);
    
    // Aplicar configuraciones
    if (strlen(profile->scheduler) > 0) {
        perf_set_scheduler(device, profile->scheduler);
    }
    
    if (profile->read_ahead_kb > 0) {
        perf_set_readahead(device, profile->read_ahead_kb);
    }
    
    if (profile->queue_depth > 0) {
        perf_set_queue_depth(device, profile->queue_depth);
    }
    
    if (profile->nr_requests > 0) {
        perf_set_nr_requests(device, profile->nr_requests);
    }
    
    // Parámetros VM
    if (profile->vm_swappiness >= 0) {
        perf_set_vm_swappiness(profile->vm_swappiness);
    }
    
    if (profile->vm_dirty_ratio > 0) {
        perf_set_vm_dirty_ratio(profile->vm_dirty_ratio);
    }
    
    if (profile->vm_dirty_background_ratio > 0) {
        perf_set_vm_dirty_background_ratio(profile->vm_dirty_background_ratio);
    }
    
    if (profile->vm_vfs_cache_pressure > 0) {
        perf_set_vm_vfs_cache_pressure(profile->vm_vfs_cache_pressure);
    }
    
    printf("\nProfile applied successfully!\n");
    return 0;
}

// Recomendar perfil
int perf_recommend(const char *device, workload_type_t workload,
                  tuning_profile_t *profile) {
    printf("\n=== Performance Recommendation ===\n");
    printf("Device: %s\n", device);
    printf("Workload: %s\n\n", perf_workload_to_string(workload));
    
    perf_get_default_profile(workload, profile);
    
    printf("Recommended settings:\n");
    printf("  I/O Scheduler:           %s\n", profile->scheduler);
    printf("  Read-ahead:              %d KB\n", profile->read_ahead_kb);
    printf("  Queue Depth:             %d\n", profile->queue_depth);
    printf("  NR Requests:             %d\n", profile->nr_requests);
    printf("  VM Swappiness:           %d\n", profile->vm_swappiness);
    printf("  VM Dirty Ratio:          %d\n", profile->vm_dirty_ratio);
    printf("  VM Dirty Background:     %d\n", profile->vm_dirty_background_ratio);
    printf("  VM VFS Cache Pressure:   %d\n", profile->vm_vfs_cache_pressure);
    
    return 0;
}

// Convertir workload a string
const char* perf_workload_to_string(workload_type_t workload) {
    switch (workload) {
        case WORKLOAD_DATABASE: return "Database";
        case WORKLOAD_WEB_SERVER: return "Web Server";
        case WORKLOAD_FILE_SERVER: return "File Server";
        case WORKLOAD_RANDOM_IO: return "Random I/O";
        case WORKLOAD_SEQUENTIAL_IO: return "Sequential I/O";
        default: return "General";
    }
}

// Comparar benchmarks
int perf_compare_benchmarks(const benchmark_results_t *before,
                           const benchmark_results_t *after,
                           char *report, size_t report_size) {
    double read_improvement = ((after->seq_read_mbs - before->seq_read_mbs) / 
                               before->seq_read_mbs) * 100.0;
    double write_improvement = ((after->seq_write_mbs - before->seq_write_mbs) /
                                before->seq_write_mbs) * 100.0;
    
    snprintf(report, report_size,
             "\n=== Performance Comparison ===\n"
             "Sequential Read:\n"
             "  Before: %.2f MB/s\n"
             "  After:  %.2f MB/s\n"
             "  Change: %+.2f%%\n\n"
             "Sequential Write:\n"
             "  Before: %.2f MB/s\n"
             "  After:  %.2f MB/s\n"
             "  Change: %+.2f%%\n",
             before->seq_read_mbs, after->seq_read_mbs, read_improvement,
             before->seq_write_mbs, after->seq_write_mbs, write_improvement);
    
    return 0;
}

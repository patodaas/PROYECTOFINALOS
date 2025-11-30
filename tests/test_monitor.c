#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/monitor.h"

void test_device_stats(void) {
    printf("\n=== Test 1: Device Statistics ===\n");
    
    device_stats_t stats;
    const char *device = "sda";  // Cambiar según tu sistema
    
    if (monitor_get_device_stats(device, &stats) == 0) {
        printf("✓ Successfully retrieved stats for %s\n", device);
        monitor_print_stats(&stats);
    } else {
        printf("✗ Failed to get stats for %s\n", device);
        printf("  Try: loop0, sdb, or another device on your system\n");
    }
}

void test_disk_usage(void) {
    printf("\n=== Test 2: Disk Usage ===\n");
    
    disk_usage_t usage;
    const char *mount_point = "/";
    
    if (monitor_get_disk_usage(mount_point, &usage) == 0) {
        printf("✓ Successfully retrieved disk usage for %s\n", mount_point);
        printf("\nMount Point: %s\n", usage.mount_point);
        printf("Device:      %s\n", usage.device);
        printf("Total:       %.2f GB\n", usage.total_bytes / (1024.0 * 1024.0 * 1024.0));
        printf("Used:        %.2f GB\n", usage.used_bytes / (1024.0 * 1024.0 * 1024.0));
        printf("Available:   %.2f GB\n", usage.available_bytes / (1024.0 * 1024.0 * 1024.0));
        printf("Usage:       %.2f%%\n", usage.usage_percent);
        printf("Total Inodes: %llu\n", usage.total_inodes);
        printf("Used Inodes:  %llu\n", usage.used_inodes);
        printf("Free Inodes:  %llu\n", usage.free_inodes);
    } else {
        printf("✗ Failed to get disk usage\n");
    }
}

void test_performance_tracking(void) {
    printf("\n=== Test 3: Performance Tracking ===\n");
    
    performance_sample_t sample;
    const char *device = "sda";
    
    printf("Collecting performance samples (10 seconds)...\n");
    
    for (int i = 0; i < 10; i++) {
        if (monitor_get_current_performance(device, &sample) == 0) {
            printf("Sample %d: IOPS=%.2f, Throughput=%.2f MB/s\n",
                   i + 1, sample.iops, sample.throughput_mbs);
            
            monitor_save_sample(device, &sample);
        }
        
        sleep(1);
    }
    
    printf("✓ Performance tracking test completed\n");
}

void test_history(void) {
    printf("\n=== Test 4: Historical Data ===\n");
    
    const char *device = "sda";
    time_t end = time(NULL);
    time_t start = end - 3600;  // Última hora
    
    performance_sample_t *samples = NULL;
    int count = 0;
    
    if (monitor_get_history(device, start, end, &samples, &count) == 0) {
        printf("✓ Retrieved %d historical samples\n", count);
        
        if (count > 0) {
            printf("\nFirst 5 samples:\n");
            for (int i = 0; i < count && i < 5; i++) {
                printf("  %s", ctime(&samples[i].timestamp));
                printf("    IOPS: %.2f, Throughput: %.2f MB/s\n",
                       samples[i].iops, samples[i].throughput_mbs);
            }
            
            free(samples);
        }
    } else {
        printf("✗ Failed to retrieve historical data\n");
    }
}

void test_continuous_monitoring(void) {
    printf("\n=== Test 5: Continuous Monitoring ===\n");
    
    printf("Starting continuous monitoring for 15 seconds...\n");
    
    if (monitor_start_continuous(3) == 0) {
        printf("✓ Monitoring started\n");
        sleep(15);
        
        if (monitor_stop_continuous() == 0) {
            printf("✓ Monitoring stopped\n");
        }
    } else {
        printf("✗ Failed to start monitoring\n");
    }
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Storage Monitor Test Suite           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    // Verificar permisos
    if (geteuid() != 0) {
        printf("\n⚠️  Warning: Some tests may require root privileges\n");
        printf("   Run with sudo for full functionality\n\n");
    }
    
    // Inicializar monitor
    if (monitor_init() != 0) {
        fprintf(stderr, "Failed to initialize monitor\n");
        return 1;
    }
    
    // Ejecutar tests
    test_device_stats();
    test_disk_usage();
    test_performance_tracking();
    test_history();
    test_continuous_monitoring();
    
    // Limpiar
    monitor_cleanup();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Monitor Tests Completed               ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Next steps:\n");
    printf("  1. Check database: /var/lib/storage_mgr/monitoring.db\n");
    printf("  2. Review historical data with: sqlite3 /var/lib/storage_mgr/monitoring.db\n");
    printf("  3. Try CLI: ./bin/storage_cli monitor stats sda\n");
    printf("\n");
    
    return 0;
}
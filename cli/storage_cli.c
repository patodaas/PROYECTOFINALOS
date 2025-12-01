#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#include "../include/ipc_server.h"
#include "../include/monitor.h"
#include "../include/backup_engine.h"
#include "../include/performance_tuner.h"
#include "../include/raid_manager.h"
#include "../include/lvm_manager.h"
#include "../include/filesystem_ops.h"
#include "../include/memory_manager.h"
#include "../include/security_manager.h"

// Conectar al daemon
int connect_to_daemon(void) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        fprintf(stderr, "Is the storage_daemon running?\n");
        close(sock);
        return -1;
    }
    
    return sock;
}

// Enviar comando genérico al daemon (protocolo request_t/response_t)
int send_command(command_type_t cmd, const char *payload) {
    int sock = connect_to_daemon();
    if (sock < 0) {
        return -1;
    }
    
    request_t req;
    memset(&req, 0, sizeof(req));
    req.version = IPC_PROTOCOL_VERSION;
    req.request_id = (uint32_t)time(NULL);
    req.command = cmd;
    
    if (payload) {
        req.payload_size = strlen(payload) + 1;
        if (req.payload_size > IPC_MAX_PAYLOAD_SIZE)
            req.payload_size = IPC_MAX_PAYLOAD_SIZE;
        strncpy(req.payload, payload, req.payload_size - 1);
    }
    
    if (write(sock, &req, sizeof(req)) != (ssize_t)sizeof(req)) {
        perror("write");
        close(sock);
        return -1;
    }
    
    response_t resp;
    if (read(sock, &resp, sizeof(resp)) != (ssize_t)sizeof(resp)) {
        perror("read");
        close(sock);
        return -1;
    }
    
    if (resp.status == STATUS_OK) {
        if (resp.data_size > 0 && resp.data_size <= IPC_MAX_PAYLOAD_SIZE) {
            printf("%s\n", resp.data);
        } else {
            printf("OK\n");
        }
    } else {
        fprintf(stderr, "Error: %s\n", resp.error_msg);
    }
    
    close(sock);
    return resp.status == STATUS_OK ? 0 : -1;
}

// ===================
// Comando de status (IPC con daemon)
// ===================
int cmd_status(void) {
    return send_command(CMD_STATUS, NULL);
}

// ===================
// Comandos de monitoreo
// ===================
int cmd_monitor_stats(const char *device) {
    device_stats_t stats;
    
    if (monitor_init() != 0) {
        return -1;
    }
    
    if (monitor_get_device_stats(device, &stats) != 0) {
        fprintf(stderr, "Failed to get stats for %s\n", device);
        monitor_cleanup();
        return -1;
    }
    
    monitor_print_stats(&stats);
    monitor_cleanup();
    
    return 0;
}

int cmd_monitor_start(int interval) {
    printf("Starting continuous monitoring (interval: %d seconds)\n", interval);
    // Placeholder: si quisieras delegarlo al daemon, usarías send_command con CMD_MONITOR_START
    return 0;
}

int cmd_monitor_stop(void) {
    printf("Stopping continuous monitoring\n");
    // Placeholder similar para CMD_MONITOR_STOP
    return 0;
}

// ===================
// Comandos de backup
// ===================
int cmd_backup_create(const char *source, const char *dest, const char *type_str) {
    backup_type_t type = BACKUP_FULL;
    
    if (strcmp(type_str, "incremental") == 0) {
        type = BACKUP_INCREMENTAL;
    } else if (strcmp(type_str, "differential") == 0) {
        type = BACKUP_DIFFERENTIAL;
    }
    
    if (backup_init(NULL) != 0) {
        return -1;
    }
    
    int result = backup_create(source, dest, type);
    
    backup_cleanup();
    return result;
}

int cmd_backup_list(void) {
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_init(NULL) != 0) {
        return -1;
    }
    
    if (backup_list(&backups, &count) != 0) {
        fprintf(stderr, "Failed to list backups\n");
        backup_cleanup();
        return -1;
    }
    
    printf("\n=== Available Backups ===\n\n");
    
    if (count == 0) {
        printf("No backups found.\n");
    } else {
        for (int i = 0; i < count; i++) {
            const char *type_str = backups[i].type == BACKUP_FULL ? "Full" :
                                  backups[i].type == BACKUP_INCREMENTAL ? "Incremental" :
                                  "Differential";
            
            printf("Backup: %s\n", backups[i].backup_id);
            printf("  Type:      %s\n", type_str);
            printf("  Date:      %s", ctime(&backups[i].timestamp));
            printf("  Source:    %s\n", backups[i].source_path);
            printf("  Size:      %.2f MB\n", backups[i].size_bytes / (1024.0 * 1024.0));
            printf("  Success:   %s\n", backups[i].success ? "Yes" : "No");
            if (!backups[i].success && strlen(backups[i].error_msg) > 0) {
                printf("  Error:     %s\n", backups[i].error_msg);
            }
            printf("\n");
        }
        
        free(backups);
    }
    
    backup_cleanup();
    return 0;
}

int cmd_backup_restore(const char *backup_id, const char *dest) {
    if (backup_init(NULL) != 0) {
        return -1;
    }
    
    int result = backup_restore(backup_id, dest);
    
    backup_cleanup();
    return result;
}

int cmd_backup_verify(const char *backup_id) {
    if (backup_init(NULL) != 0) {
        return -1;
    }
    
    int result = backup_verify(backup_id);
    
    backup_cleanup();
    return result;
}

// ===================
// Comandos de performance
// ===================
int cmd_perf_benchmark(const char *device, const char *test_file) {
    benchmark_results_t results;
    
    if (perf_init() != 0) {
        return -1;
    }
    
    int result = perf_benchmark(device, test_file, &results);
    
    perf_cleanup();
    return result;
}

int cmd_perf_tune(const char *device, const char *scheduler, int readahead) {
    if (perf_init() != 0) {
        return -1;
    }
    
    if (scheduler) {
        perf_set_scheduler(device, scheduler);
    }
    
    if (readahead > 0) {
        perf_set_readahead(device, readahead);
    }
    
    perf_cleanup();
    return 0;
}

int cmd_perf_recommend(const char *device, const char *workload_str) {
    workload_type_t workload = WORKLOAD_GENERAL;
    
    if (strcmp(workload_str, "database") == 0) {
        workload = WORKLOAD_DATABASE;
    } else if (strcmp(workload_str, "web") == 0) {
        workload = WORKLOAD_WEB_SERVER;
    } else if (strcmp(workload_str, "fileserver") == 0) {
        workload = WORKLOAD_FILE_SERVER;
    }
    
    if (perf_init() != 0) {
        return -1;
    }
    
    tuning_profile_t profile;
    int result = perf_recommend(device, workload, &profile);
    
    if (result == 0) {
        printf("\nRecommended profile:\n");
        printf("  Scheduler : %s\n", profile.scheduler);
        printf("  Readahead : %d KB\n", profile.read_ahead_kb);
        printf("  QueueDepth: %d\n", profile.queue_depth);
        printf("\nApply these settings? (y/n): ");
        char response;
        scanf(" %c", &response);
        
        if (response == 'y' || response == 'Y') {
            perf_apply_profile(device, &profile);
        }
    }
    
    perf_cleanup();
    return result;
}

// ===================
// Comandos RAID (básicos)
// ===================
int cmd_raid_create(const char *array_name, int level, char **devices, int count) {
    return raid_create(array_name, level, devices, count);
}

int cmd_raid_status(const char *array_name) {
    raid_array_t array;
    memset(&array, 0, sizeof(array));
    strncpy(array.name, array_name, sizeof(array.name) - 1);
    if (raid_monitor(&array) != 0) {
        fprintf(stderr, "Failed to get RAID status for %s\n", array_name);
        return -1;
    }
    printf("RAID Array: %s\n", array.name);
    printf("  Level     : %d\n", array.raid_level);
    printf("  Devices   : %d\n", array.num_devices);
    printf("  Status    : %s\n", array.status);
    printf("  Failed    : %d\n", array.num_failed);
    return 0;
}

// ===================
// Comandos LVM (básicos)
// ===================
int cmd_lvm_pv_create(const char *device) {
    return lvm_pv_create(device);
}

int cmd_lvm_vg_create(const char *vg_name, char **pvs, int pv_count) {
    return lvm_vg_create(vg_name, pvs, pv_count);
}

int cmd_lvm_lv_create(const char *vg_name, const char *lv_name, unsigned long long size_mb) {
    return lvm_lv_create(vg_name, lv_name, size_mb);
}

// ===================
// Comandos Filesystem (básicos)
// ===================
fs_type_t parse_fs_type(const char *type_str) {
    if (strcmp(type_str, "ext4") == 0) return FS_TYPE_EXT4;
    if (strcmp(type_str, "xfs") == 0)  return FS_TYPE_XFS;
    if (strcmp(type_str, "btrfs") == 0) return FS_TYPE_BTRFS;
    return FS_TYPE_EXT4;
}

int cmd_fs_create(const char *device, const char *type_str, const char *label) {
    fs_type_t t = parse_fs_type(type_str);
    return fs_create(device, t, label);
}

int cmd_fs_mount(const char *device, const char *mount_point, const char *type_str) {
    fs_type_t t = parse_fs_type(type_str);
    return fs_mount(device, mount_point, t, "");
}

int cmd_fs_unmount(const char *mount_point) {
    return fs_unmount(mount_point, 0);
}

int cmd_fs_check(const char *device, const char *type_str) {
    fs_type_t t = parse_fs_type(type_str);
    return fs_check(device, t);
}

// ===================
// Comandos Memory (básico)
// ===================
int cmd_memory_status(void) {
    memory_info_t info;
    if (memory_get_info(&info) != 0) {
        fprintf(stderr, "Failed to get memory info\n");
        return -1;
    }
    printf("Memory (KB): total=%llu free=%llu available=%llu cached=%llu buffers=%llu\n",
           info.total_kb, info.free_kb, info.available_kb,
           info.cached_kb, info.buffers_kb);
    printf("Swap  (KB): total=%llu free=%llu\n",
           info.swap_total_kb, info.swap_free_kb);
    printf("Memory pressure: %.2f\n", info.memory_pressure);
    return 0;
}

// ===================
// Ayuda
// ===================
void show_help(const char *prog_name) {
    printf("Usage: %s mmand> [options]\n\n", prog_name);
    printf("Storage Manager CLI\n\n");
    
    printf("Monitor Commands:\n");
    printf("  monitor stats <device>       - Show device statistics\n");
    printf("  monitor start [interval]     - Start continuous monitoring\n");
    printf("  monitor stop                 - Stop continuous monitoring\n\n");
    
    printf("Backup Commands:\n");
    printf("  backup create <src> <dest> <type>  - Create backup (full/incremental/differential)\n");
    printf("  backup list                         - List all backups\n");
    printf("  backup restore <id> <dest>          - Restore backup\n");
    printf("  backup verify <id>                  - Verify backup integrity\n\n");
    
    printf("Performance Commands:\n");
    printf("  perf benchmark <device> <file>     - Run performance benchmark\n");
    printf("  perf tune <device> --scheduler=X --readahead=Y  - Tune device\n");
    printf("  perf recommend <device> <workload> - Get tuning recommendations\n");
    printf("                                       (workload: database/web/fileserver/general)\n\n");
    
    printf("RAID Commands (básicos):\n");
    printf("  raid create <array> <level> <dev1> [dev2 ...]\n");
    printf("  raid status <array>\n\n");
    
    printf("LVM Commands (básicos):\n");
    printf("  lvm pv-create <device>\n");
    printf("  lvm vg-create <vg_name> <pv1> [pv2 ...]\n");
    printf("  lvm lv-create <vg_name> <lv_name> <size_mb>\n\n");
    
    printf("Filesystem Commands (básicos):\n");
    printf("  fs create <device> <type> [--label=NAME]\n");
    printf("  fs mount <device> <mount_point> <type>\n");
    printf("  fs unmount <mount_point>\n");
    printf("  fs check <device> <type>\n\n");
    
    printf("Memory Commands:\n");
    printf("  memory status                  - Show memory/swap status\n\n");
    
    printf("General Commands:\n");
    printf("  status                       - Show daemon status\n");
    printf("  help                         - Show this help\n\n");
    
    printf("Examples:\n");
    printf("  %s monitor stats sda\n", prog_name);
    printf("  %s backup create /data /backup full\n", prog_name);
    printf("  %s perf benchmark sda /mnt/data/test\n", prog_name);
    printf("  %s raid status /dev/md0\n", prog_name);
}

// ===================
// CLI principal
// ===================
int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    // Monitor
    if (strcmp(command, "monitor") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s monitor <stats|start|stop> [args]\n", argv[0]);
            return 1;
        }
        
        const char *subcmd = argv[2];
        
        if (strcmp(subcmd, "stats") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s monitor stats <device>\n", argv[0]);
                return 1;
            }
            return cmd_monitor_stats(argv[3]);
        } else if (strcmp(subcmd, "start") == 0) {
            int interval = argc >= 4 ? atoi(argv[3]) : 5;
            return cmd_monitor_start(interval);
        } else if (strcmp(subcmd, "stop") == 0) {
            return cmd_monitor_stop();
        }
    }
    
    // Backup
    else if (strcmp(command, "backup") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s backup reate|list|restore|verify> [args]\n", argv[0]);
            return 1;
        }
        
        const char *subcmd = argv[2];
        
        if (strcmp(subcmd, "create") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage: %s backup create <source> <dest> <type>\n", argv[0]);
                fprintf(stderr, "Types: full, incremental, differential\n");
                return 1;
            }
            return cmd_backup_create(argv[3], argv[4], argv[5]);
        } else if (strcmp(subcmd, "list") == 0) {
            return cmd_backup_list();
        } else if (strcmp(subcmd, "restore") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s backup restore <backup_id> <dest>\n", argv[0]);
                return 1;
            }
            return cmd_backup_restore(argv[3], argv[4]);
        } else if (strcmp(subcmd, "verify") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s backup verify <backup_id>\n", argv[0]);
                return 1;
            }
            return cmd_backup_verify(argv[3]);
        }
    }
    
    // Performance
    else if (strcmp(command, "perf") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s perf <benchmark|tune|recommend> [args]\n", argv[0]);
            return 1;
        }
        
        const char *subcmd = argv[2];
        
        if (strcmp(subcmd, "benchmark") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s perf benchmark <device> <test_file>\n", argv[0]);
                return 1;
            }
            return cmd_perf_benchmark(argv[3], argv[4]);
        } else if (strcmp(subcmd, "tune") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s perf tune <device> [--scheduler=X] [--readahead=Y]\n", argv[0]);
                return 1;
            }
            
            const char *device = argv[3];
            const char *scheduler = NULL;
            int readahead = 0;
            
            for (int i = 4; i < argc; i++) {
                if (strncmp(argv[i], "--scheduler=", 12) == 0) {
                    scheduler = argv[i] + 12;
                } else if (strncmp(argv[i], "--readahead=", 12) == 0) {
                    readahead = atoi(argv[i] + 12);
                }
            }
            
            return cmd_perf_tune(device, scheduler, readahead);
        } else if (strcmp(subcmd, "recommend") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s perf recommend <device> <workload>\n", argv[0]);
                fprintf(stderr, "Workloads: database, web, fileserver, general\n");
                return 1;
            }
            return cmd_perf_recommend(argv[3], argv[4]);
        }
    }
    
    // RAID
    else if (strcmp(command, "raid") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s raid reate|status> [args]\n", argv[0]);
            return 1;
        }
        const char *subcmd = argv[2];
        if (strcmp(subcmd, "create") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage: %s raid create <array> <level> <dev1> [dev2 ...]\n", argv[0]);
                return 1;
            }
            const char *array = argv[3];
            int level = atoi(argv[4]);
            char **devs = &argv[5];
            int count = argc - 5;
            return cmd_raid_create(array, level, devs, count);
        } else if (strcmp(subcmd, "status") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s raid status <array>\n", argv[0]);
                return 1;
            }
            return cmd_raid_status(argv[3]);
        }
    }
    
    // LVM
    else if (strcmp(command, "lvm") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s lvm <pv-create|vg-create|lv-create> [args]\n", argv[0]);
            return 1;
        }
        const char *subcmd = argv[2];
        if (strcmp(subcmd, "pv-create") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s lvm pv-create <device>\n", argv[0]);
                return 1;
            }
            return cmd_lvm_pv_create(argv[3]);
        } else if (strcmp(subcmd, "vg-create") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s lvm vg-create <vg_name> <pv1> [pv2 ...]\n", argv[0]);
                return 1;
            }
            const char *vg_name = argv[3];
            char **pvs = &argv[4];
            int count = argc - 4;
            return cmd_lvm_vg_create(vg_name, pvs, count);
        } else if (strcmp(subcmd, "lv-create") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage: %s lvm lv-create <vg_name> <lv_name> <size_mb>\n", argv[0]);
                return 1;
            }
            const char *vg_name = argv[3];
            const char *lv_name = argv[4];
            unsigned long long size_mb = strtoull(argv[5], NULL, 10);
            return cmd_lvm_lv_create(vg_name, lv_name, size_mb);
        }
    }
    
    // Filesystem
    else if (strcmp(command, "fs") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s fs reate|mount|unmount|check> [args]\n", argv[0]);
            return 1;
        }
        const char *subcmd = argv[2];
        if (strcmp(subcmd, "create") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s fs create <device> <type> [--label=NAME]\n", argv[0]);
                return 1;
            }
            const char *device = argv[3];
            const char *type = argv[4];
            const char *label = NULL;
            for (int i = 5; i < argc; i++) {
                if (strncmp(argv[i], "--label=", 8) == 0) {
                    label = argv[i] + 8;
                }
            }
            if (!label) label = "storage_vol";
            return cmd_fs_create(device, type, label);
        } else if (strcmp(subcmd, "mount") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage: %s fs mount <device> <mount_point> <type>\n", argv[0]);
                return 1;
            }
            return cmd_fs_mount(argv[3], argv[4], argv[5]);
        } else if (strcmp(subcmd, "unmount") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s fs unmount <mount_point>\n", argv[0]);
                return 1;
            }
            return cmd_fs_unmount(argv[3]);
        } else if (strcmp(subcmd, "check") == 0) {
            if (argc < 5) {
                fprintf(stderr, "Usage: %s fs check <device> <type>\n", argv[0]);
                return 1;
            }
            return cmd_fs_check(argv[3], argv[4]);
        }
    }
    
    // Memory
    else if (strcmp(command, "memory") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s memory status\n", argv[0]);
            return 1;
        }
        const char *subcmd = argv[2];
        if (strcmp(subcmd, "status") == 0) {
            return cmd_memory_status();
        }
    }
    
    // Generales
    else if (strcmp(command, "status") == 0) {
        return cmd_status();
    } else if (strcmp(command, "help") == 0 ||
               strcmp(command, "--help") == 0 ||
               strcmp(command, "-h") == 0) {
        show_help(argv[0]);
        return 0;
    }
    
    fprintf(stderr, "Unknown command: %s\n", command);
    fprintf(stderr, "Run '%s help' for usage information\n", argv[0]);
    return 1;
}


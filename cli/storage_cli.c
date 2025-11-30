#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "../include/ipc_server.h"
#include "../include/monitor.h"
#include "../include/backup_engine.h"
#include "../include/performance_tuner.h"

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

// Enviar comando al daemon
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
        strncpy(req.payload, payload, sizeof(req.payload) - 1);
    }
    
    // Enviar request
    if (write(sock, &req, sizeof(req)) != sizeof(req)) {
        perror("write");
        close(sock);
        return -1;
    }
    
    // Recibir response
    response_t resp;
    if (read(sock, &resp, sizeof(resp)) != sizeof(resp)) {
        perror("read");
        close(sock);
        return -1;
    }
    
    // Mostrar resultado
    if (resp.status == STATUS_OK) {
        printf("%s", resp.data);
    } else {
        fprintf(stderr, "Error: %s\n", resp.error_msg);
    }
    
    close(sock);
    return resp.status == STATUS_OK ? 0 : -1;
}

// Comandos de monitoreo
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
    return send_command(CMD_MONITOR_START, NULL);
}

int cmd_monitor_stop(void) {
    printf("Stopping continuous monitoring\n");
    return send_command(CMD_MONITOR_STOP, NULL);
}

// Comandos de backup
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

// Comandos de performance
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

// Comando de status general
int cmd_status(void) {
    return send_command(CMD_STATUS, NULL);
}

// Mostrar ayuda
void show_help(const char *prog_name) {
    printf("Usage: %s <command> [options]\n\n", prog_name);
    printf("Storage Manager CLI - Parts 6-10\n\n");
    
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
    
    printf("General Commands:\n");
    printf("  status                       - Show daemon status\n");
    printf("  help                         - Show this help\n\n");
    
    printf("Examples:\n");
    printf("  %s monitor stats sda\n", prog_name);
    printf("  %s backup create /data /backup full\n", prog_name);
    printf("  %s backup list\n", prog_name);
    printf("  %s perf benchmark sda /mnt/data/test\n", prog_name);
    printf("  %s perf recommend sda database\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    // Comandos de monitor
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
    
    // Comandos de backup
    else if (strcmp(command, "backup") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s backup <create|list|restore|verify> [args]\n", argv[0]);
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
    
    // Comandos de performance
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
    
    // Comandos generales
    else if (strcmp(command, "status") == 0) {
        return cmd_status();
    } else if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        show_help(argv[0]);
        return 0;
    }
    
    // Comando desconocido
    fprintf(stderr, "Unknown command: %s\n", command);
    fprintf(stderr, "Run '%s help' for usage information\n", argv[0]);
    return 1;
}
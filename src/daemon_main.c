#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <getopt.h>
#include <syslog.h>

#include "monitor.h"
#include "backup_engine.h"
#include "performance_tuner.h"
#include "ipc_server.h"
#include "daemon.h"

/* Constantes y variables externas del daemon framework */
extern volatile sig_atomic_t daemon_running;
extern volatile sig_atomic_t reload_config;
extern int daemon_init(void);
extern int daemon_create_pidfile(const char *pidfile);
extern int daemon_setup_signals(void);
extern int daemon_monitor_workers(worker_t *workers, int max_workers);
extern int daemon_spawn_worker(void (*func)(void *), void *arg);
extern void daemon_shutdown(void);
extern int daemon_set_resource_limits(void);
extern int daemon_reload_config(void);

/* IPC socket */
static const char *socket_path = IPC_SOCKET_PATH;

/* Hilo IPC */
static pthread_t ipc_thread;

/* Worker de prueba */
void test_worker(void *arg) {
    int task_id = arg ? *((int *)arg) : 0;
    syslog(LOG_INFO, "Test worker %d starting", task_id);
    sleep(3);
    syslog(LOG_INFO, "Test worker %d finished", task_id);
}

/* Uso/ayuda */
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Storage Manager Daemon\n\n");
    printf("Options:\n");
    printf("  -f, --foreground    Run in foreground (don't daemonize)\n");
    printf("  -p, --pidfile PATH  Specify PID file path\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show version information\n");
    printf("\n");
    printf("Signals:\n");
    printf("  SIGTERM/SIGINT      Graceful shutdown\n");
    printf("  SIGHUP              Reload configuration\n");
    printf("  SIGUSR1             Dump status\n");
    printf("\n");
}

/* Limpieza específica */
void cleanup(void)
{
    ipc_server_cleanup();
    fflush(stdout);
}

/* Manejador simple (no se usa en el flujo principal, pero se conserva) */
static void handle_signal_simple(int sig)
{
    (void)sig;
    cleanup();
    exit(0);
}

/* Hilo que corre el servidor IPC */
static void* ipc_server_thread(void *arg) {
    (void)arg;
    syslog(LOG_INFO, "IPC server thread started");
    ipc_server_run();
    syslog(LOG_INFO, "IPC server thread exiting");
    return NULL;
}

int main(int argc, char *argv[])
{
    int foreground = 0;
    const char *pidfile = NULL;
    int opt;

    static struct option long_options[] = {
        {"foreground", no_argument, 0, 'f'},
        {"pidfile", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "fp:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                foreground = 1;
                break;
            case 'p':
                pidfile = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                printf("Storage Manager Daemon v1.0\n");
                printf("Part of Linux Systems Programming Project\n");
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Error: this daemon must be run as root\n");
        return 1;
    }

    printf("Starting Storage Manager Daemon...\n");

    /* Inicialización de IPC y monitor antes de daemonizar */
    if (ipc_server_init(socket_path) != 0) {
        fprintf(stderr, "ipc_server_init failed (socket=%s)\n", socket_path);
        return 1;
    }

    if (monitor_init() != 0) {
        fprintf(stderr, "monitor_init failed\n");
        ipc_server_cleanup();
        return 1;
    }

    if (monitor_start_continuous(5) != 0) {
        fprintf(stderr, "monitor_start_continuous failed\n");
        ipc_server_cleanup();
        return 1;
    }

    /* Daemonización */
    if (!foreground) {
        printf("Daemonizing process...\n");
        if (daemon_init() < 0) {
            fprintf(stderr, "Error: Failed to daemonize\n");
            cleanup();
            return 1;
        }
    } else {
        printf("Running in foreground mode\n");
        openlog(DAEMON_NAME, LOG_PID | LOG_PERROR, LOG_DAEMON);
    }

    /* PID file */
    if (daemon_create_pidfile(pidfile) < 0) {
        syslog(LOG_ERR, "Error creating PID file");
        cleanup();
        return 1;
    }

    /* Señales */
    if (daemon_setup_signals() < 0) {
        syslog(LOG_ERR, "Error setting up signal handlers");
        daemon_remove_pidfile(pidfile);
        cleanup();
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    daemon_set_resource_limits();

    syslog(LOG_INFO, "Storage Manager Daemon started successfully");
    syslog(LOG_INFO, "PID: %d", getpid());
    printf("Storage Manager Daemon started (%s)\n",
           foreground ? "foreground" : "background");
    fflush(stdout);

    /* Lanzar el servidor IPC en un hilo separado */
    if (pthread_create(&ipc_thread, NULL, ipc_server_thread, NULL) != 0) {
        fprintf(stderr, "Failed to start IPC server thread\n");
        cleanup();
        return 1;
    }

    /* Bucle principal del daemon */
    int loop_count = 0;
    while (daemon_running) {
        if (reload_config) {
            daemon_reload_config();
        }

        worker_t worker_list[MAX_WORKERS];
        int active = daemon_monitor_workers(worker_list, MAX_WORKERS);

        if (loop_count % 30 == 0 && active < 3) {
            syslog(LOG_INFO, "Spawning test worker");
            int *task_id = malloc(sizeof(int));
            if (task_id) {
                *task_id = loop_count;
                daemon_spawn_worker(test_worker, task_id);
            } else {
                syslog(LOG_ERR, "Failed to allocate memory for task_id");
            }
        }

        sleep(1);
        loop_count++;
    }

    syslog(LOG_INFO, "Daemon received shutdown signal");

    /* Parar IPC y esperar al hilo */
    ipc_server_stop();
    pthread_join(ipc_thread, NULL);

    /* Apagado ordenado */
    daemon_shutdown();
    daemon_remove_pidfile(pidfile);
    cleanup();

    return 0;
}


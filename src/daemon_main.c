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
extern const char *DAEMON_NAME;
extern void daemon_reload_config(void);
extern int daemon_init(void);
extern int daemon_create_pidfile(const char *pidfile);
extern void daemon_remove_pidfile(const char *pidfile);
extern int daemon_setup_signals(void);
extern void daemon_set_resource_limits(void);
extern int daemon_monitor_workers(worker_t *workers, int max_workers);
extern int daemon_spawn_worker(void (*func)(void *), void *arg);
extern void daemon_shutdown(void);

/* IPC socket */
static const char *socket_path = IPC_SOCKET_PATH;

/* Worker de prueba (se mantiene igual) */
void test_worker(void *arg) {
    int task_id = arg ? *((int *)arg) : 0;
    syslog(LOG_INFO, "Test worker %d starting", task_id);
    sleep(3);
    syslog(LOG_INFO, "Test worker %d finished", task_id);
}

/* Uso/ayuda combinada */
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

/* Limpieza específica de tu Storage Manager (IPC + stdout) */
void cleanup(void)
{
    ipc_server_cleanup();
    /* ipc_server_cleanup() ya hace close + unlink del socket si corresponde */
    fflush(stdout);
}

/* Manejador de señal simple para la primera parte (si lo quieres seguir usando) */
static void handle_signal_simple(int sig)
{
    (void)sig;
    cleanup();
    exit(0);
}

int main(int argc, char *argv[])
{
    int foreground = 0;
    const char *pidfile = NULL;
    int opt;

    /* Opciones largas unificadas */
    static struct option long_options[] = {
        {"foreground", no_argument, 0, 'f'},
        {"pidfile", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    /* Parseo de parámetros estilo segundo código (pero conserva -f y -h originales) */
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

    /* Inicialización de IPC y monitor ANTES de daemonizar para poder reportar errores por stderr */
    if (ipc_server_init(socket_path) != 0) {
        fprintf(stderr, "ipc_server_init failed (socket=%s)\n", socket_path);
        return 1;
    }

    if (monitor_init() != 0) {
        fprintf(stderr, "monitor_init failed\n");
        ipc_server_cleanup();
        return 1;
    }

    /* Inicia el monitoreo continuo en background (intervalo 5s) */
    if (monitor_start_continuous(5) != 0) {
        fprintf(stderr, "monitor_start_continuous failed\n");
        ipc_server_cleanup();
        return 1;
    }

    /* Daemonización usando tu framework */
    if (!foreground) {
        printf("Daemonizing process...\n");
        if (daemon_init() < 0) {
            fprintf(stderr, "Error: Failed to daemonize\n");
            cleanup();
            return 1;
        }
        /* syslog se abrirá dentro de daemon_init() o en daemon_setup_signals(), según tu implementación */
    } else {
        printf("Running in foreground mode\n");
        openlog(DAEMON_NAME, LOG_PID | LOG_PERROR, LOG_DAEMON);
    }

    /* PID file opcional */
    if (daemon_create_pidfile(pidfile) < 0) {
        syslog(LOG_ERR, "Error creating PID file");
        cleanup();
        return 1;
    }

    /* Configuración de señales del framework de daemon
       (puede internamente manejar SIGTERM, SIGINT, SIGHUP, etc. y setear daemon_running/reload_config) */
    if (daemon_setup_signals() < 0) {
        syslog(LOG_ERR, "Error setting up signal handlers");
        daemon_remove_pidfile(pidfile);
        cleanup();
        return 1;
    }

    /* Si quieres mantener los SIGPIPE/SIGHUP ignorados como en el primer código,
       puedes hacerlo aquí, siempre que no rompa daemon_setup_signals(): */
    signal(SIGPIPE, SIG_IGN);

    daemon_set_resource_limits();

    syslog(LOG_INFO, "Storage Manager Daemon started successfully");
    syslog(LOG_INFO, "PID: %d", getpid());
    printf("Storage Manager Daemon started (%s)\n",
           foreground ? "foreground" : "background");
    fflush(stdout);

    /* Bucle principal: combina el loop de workers del segundo código
       con el hecho de que el servidor IPC vive internamente. */
    int loop_count = 0;
    while (daemon_running) {
        if (reload_config) {
            daemon_reload_config();
        }

        worker_t worker_list[MAX_WORKERS];
        int active = daemon_monitor_workers(worker_list, MAX_WORKERS);

        /* Ejemplo de spawn de worker de prueba, se mantiene igual */
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

        /* El servidor IPC maneja conexiones en otros hilos o internamente;
           aquí solo mantenemos vivo el daemon. */
        sleep(1);
        loop_count++;
    }

    syslog(LOG_INFO, "Daemon received shutdown signal");

    /* Apagado ordenado del framework y de tus recursos */
    daemon_shutdown();
    daemon_remove_pidfile(pidfile);
    cleanup();

    return 0;
}

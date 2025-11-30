#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "monitor.h"
#include "backup_engine.h"
#include "performance_tuner.h"
#include "ipc_server.h"

static const char *socket_path = IPC_SOCKET_PATH;

void cleanup(void)
{
    ipc_server_cleanup();
    /* ipc_server_cleanup() ya hace close + unlink del socket si corresponde */
    fflush(stdout);
}

static void handle_signal(int sig)
{
    (void)sig;
    cleanup();
    exit(0);
}

int main(int argc, char *argv[])
{
    int foreground = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--foreground") == 0) {
            foreground = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("  -f, --foreground    Run in foreground\n");
            printf("  -h, --help          Show this help\n");
            return 0;
        }
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Error: this daemon must be run as root\n");
        return 1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

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

    printf("Storage Manager Daemon started (%s)\n", foreground ? "foreground" : "background");
    fflush(stdout);

    if (!foreground) {
        if (daemon(0, 0) < 0) {
            perror("daemon");
            cleanup();
            return 1;
        }
    }

    /* Loop principal sencillo: el IPC server gestiona conexiones internamente.
       Aquí solo mantenemos el proceso vivo hasta recibir señal. */
    while (1) {
        pause(); /* espera señal */
    }

    cleanup();
    return 0;
}


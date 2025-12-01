#ifndef DAEMON_H
#define DAEMON_H

#include <sys/types.h>
#include <time.h>
#include <signal.h>

#define PID_FILE "/var/run/storage_mgr.pid"
#define PID_FILE_LOCAL "./storage_mgr.pid"
#define MAX_WORKERS 10
#define DAEMON_NAME "storage_daemon"

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_FAILED -1

typedef struct {
    pid_t pid;
    int status;
    time_t started;
    char task[256];
} worker_t;

int daemon_init(void);
int daemon_create_pidfile(const char *path);
int daemon_remove_pidfile(const char *path);
int daemon_setup_signals(void);
void daemon_signal_handler(int sig);

int daemon_spawn_worker(void (*worker_func)(void *), void *arg);
int daemon_monitor_workers(worker_t *workers, int count);
void daemon_reap_zombies(void);

void daemon_shutdown(void);
int daemon_reload_config(void);
int daemon_is_running(const char *pid_file);
pid_t daemon_read_pid(const char *pid_file);

void daemon_log(const char *format, ...);
int daemon_set_resource_limits(void);

extern volatile sig_atomic_t daemon_running;

#endif


#include "daemon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>

volatile sig_atomic_t daemon_running = 1;
volatile sig_atomic_t reload_config = 0;
static worker_t workers[MAX_WORKERS];
static int worker_count = 0;

void daemon_log(const char *format, ...) {
    va_list args;
    char buffer[1024];
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    syslog(LOG_INFO, "%s", buffer);
    fprintf(stderr, "[DAEMON] %s\n", buffer);
}

void daemon_signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            syslog(LOG_INFO, "Received termination signal (%d)", sig);
            daemon_running = 0;
            break;
        case SIGHUP:
            syslog(LOG_INFO, "Received SIGHUP - reloading configuration");
            reload_config = 1;
            break;
        case SIGCHLD:
            daemon_reap_zombies();
            break;
        case SIGUSR1:
            syslog(LOG_INFO, "Received SIGUSR1 - dumping status");
            daemon_log("Active workers: %d", worker_count);
            for (int i = 0; i < worker_count; i++) {
                if (workers[i].pid > 0) {
                    daemon_log("Worker %d: PID=%d, status=%d, task=%s", 
                              i, workers[i].pid, workers[i].status, workers[i].task);
                }
            }
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal: %d", sig);
            break;
    }
}

int daemon_setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = daemon_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGTERM: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGINT: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGHUP: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGCHLD: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGUSR1: %s", strerror(errno));
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    syslog(LOG_INFO, "Signal handlers configured successfully");
    return 0;
}

int daemon_init(void) {
    pid_t pid, sid;
    int fd;
    
    if (getppid() == 1) {
        daemon_log("Already running as daemon");
        return 0;
    }
    
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error in first fork: %s\n", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ERR, "Error in setsid: %s", strerror(errno));
        return -1;
    }
    
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Error in second fork: %s", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "Error changing to root directory: %s", strerror(errno));
        return -1;
    }
    
    umask(0);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) {
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO) {
        return -1;
    }
    if (dup2(fd, STDERR_FILENO) != STDERR_FILENO) {
        return -1;
    }
    
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon initialized successfully (PID: %d)", getpid());
    
    return 0;
}

int daemon_create_pidfile(const char *path) {
    FILE *fp;
    const char *pidfile_path = path ? path : PID_FILE;
    
    if (daemon_is_running(pidfile_path)) {
        syslog(LOG_ERR, "Daemon already running (PID file exists)");
        return -1;
    }
    
    fp = fopen(pidfile_path, "w");
    if (!fp) {
        pidfile_path = PID_FILE_LOCAL;
        fp = fopen(pidfile_path, "w");
        if (!fp) {
            syslog(LOG_ERR, "Cannot create PID file: %s", strerror(errno));
            return -1;
        }
    }
    
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    chmod(pidfile_path, 0644);
    
    syslog(LOG_INFO, "PID file created: %s", pidfile_path);
    return 0;
}

int daemon_remove_pidfile(const char *path) {
    const char *pidfile_path = path ? path : PID_FILE;
    
    if (unlink(pidfile_path) < 0 && errno != ENOENT) {
        pidfile_path = PID_FILE_LOCAL;
        unlink(pidfile_path);
    }
    
    syslog(LOG_INFO, "PID file removed");
    return 0;
}

int daemon_is_running(const char *pid_file) {
    FILE *fp;
    pid_t pid;
    
    fp = fopen(pid_file, "r");
    if (!fp) {
        fp = fopen(PID_FILE_LOCAL, "r");
        if (!fp) {
            return 0;
        }
    }
    
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    
    if (kill(pid, 0) == 0) {
        return 1;
    }
    
    return 0;
}

pid_t daemon_read_pid(const char *pid_file) {
    FILE *fp;
    pid_t pid;
    
    fp = fopen(pid_file, "r");
    if (!fp) {
        fp = fopen(PID_FILE_LOCAL, "r");
        if (!fp) {
            return -1;
        }
    }
    
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return pid;
}

void daemon_reap_zombies(void) {
    pid_t pid;
    int status;
    int i;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        syslog(LOG_INFO, "Worker terminated: PID=%d, status=%d", pid, status);
        
        for (i = 0; i < worker_count; i++) {
            if (workers[i].pid == pid) {
                if (WIFEXITED(status)) {
                    workers[i].status = WORKER_IDLE;
                    syslog(LOG_INFO, "Worker %d exited normally (code: %d)", 
                           pid, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    workers[i].status = WORKER_FAILED;
                    syslog(LOG_WARNING, "Worker %d terminated by signal %d", 
                           pid, WTERMSIG(status));
                } else {
                    workers[i].status = WORKER_FAILED;
                    syslog(LOG_WARNING, "Worker %d terminated abnormally", pid);
                }
                workers[i].pid = 0;
                break;
            }
        }
    }
}

int daemon_spawn_worker(void (*worker_func)(void *), void *arg) {
    pid_t pid;
    
    if (worker_count >= MAX_WORKERS) {
        syslog(LOG_WARNING, "Maximum number of workers reached (%d)", MAX_WORKERS);
        return -1;
    }
    
    pid = fork();
    
    if (pid < 0) {
        syslog(LOG_ERR, "Error creating worker: %s", strerror(errno));
        return -1;
    }
    
    if (pid == 0) {
        syslog(LOG_INFO, "Worker started: PID=%d", getpid());
        
        if (worker_func) {
            worker_func(arg);
        }
        
        syslog(LOG_INFO, "Worker exiting: PID=%d", getpid());
        exit(EXIT_SUCCESS);
    }
    
    workers[worker_count].pid = pid;
    workers[worker_count].status = WORKER_BUSY;
    workers[worker_count].started = time(NULL);
    snprintf(workers[worker_count].task, sizeof(workers[worker_count].task),
             "Worker task %d", worker_count);
    
    worker_count++;
    
    syslog(LOG_INFO, "Worker spawned: PID=%d, total_workers=%d", pid, worker_count);
    return 0;
}

int daemon_monitor_workers(worker_t *worker_list, int count) {
    int i;
    int active_count = 0;
    
    for (i = 0; i < worker_count && i < MAX_WORKERS; i++) {
        if (workers[i].pid > 0) {
            if (kill(workers[i].pid, 0) < 0) {
                if (errno == ESRCH) {
                    syslog(LOG_WARNING, "Worker %d not found", workers[i].pid);
                    workers[i].status = WORKER_FAILED;
                    workers[i].pid = 0;
                }
            } else {
                active_count++;
            }
            
            if (worker_list && i < count) {
                memcpy(&worker_list[i], &workers[i], sizeof(worker_t));
            }
        }
    }
    
    return active_count;
}

int daemon_reload_config(void) {
    syslog(LOG_INFO, "Reloading configuration...");
    reload_config = 0;
    syslog(LOG_INFO, "Configuration reloaded successfully");
    return 0;
}

void daemon_shutdown(void) {
    int i;
    
    syslog(LOG_INFO, "Initiating daemon shutdown...");
    
    for (i = 0; i < worker_count; i++) {
        if (workers[i].pid > 0) {
            syslog(LOG_INFO, "Terminating worker %d", workers[i].pid);
            kill(workers[i].pid, SIGTERM);
        }
    }
    
    sleep(2);
    
    for (i = 0; i < worker_count; i++) {
        if (workers[i].pid > 0) {
            if (kill(workers[i].pid, 0) == 0) {
                syslog(LOG_WARNING, "Force killing worker %d", workers[i].pid);
                kill(workers[i].pid, SIGKILL);
            }
        }
    }
    
    daemon_reap_zombies();
    daemon_remove_pidfile(NULL);
    
    syslog(LOG_INFO, "Daemon shutdown complete");
    closelog();
}

int daemon_set_resource_limits(void) {
    struct rlimit rl;
    
    rl.rlim_cur = 1024;
    rl.rlim_max = 2048;
    
    if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
        syslog(LOG_WARNING, "Could not set file descriptor limit: %s", 
               strerror(errno));
        return -1;
    }
    
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    
    if (setrlimit(RLIMIT_CORE, &rl) < 0) {
        syslog(LOG_WARNING, "Could not set core dump limit: %s", 
               strerror(errno));
        return -1;
    }
    
    syslog(LOG_INFO, "Resource limits set successfully");
    return 0;
}

void example_worker_func(void *arg) {
    int *work_id = (int *)arg;
    int id = work_id ? *work_id : 0;
    
    syslog(LOG_INFO, "Worker executing task %d", id);
    sleep(5);
    syslog(LOG_INFO, "Worker completed task %d", id);
}

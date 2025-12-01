#include "ipc_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define SHM_NAME "/storage_mgr_shm"
#define SEM_NAME "/storage_mgr_sem"

static ipc_server_state_t server_state;
static system_status_t *shared_status = NULL;
static sem_t *status_sem = NULL;
static int msg_queue_id = -1;

static struct sockaddr_un addr;

int ipc_server_init(const char *socket_path) {
    signal(SIGPIPE, SIG_IGN);

    memset(&server_state, 0, sizeof(server_state));
    pthread_mutex_init(&server_state.clients_mutex, NULL);

    server_state.server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_state.server_fd < 0) {
        perror("socket");
        return -1;
    }

    unlink(socket_path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(server_state.server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_state.server_fd);
        unlink(socket_path);
        return -1;
    }

    if (listen(server_state.server_fd, 10) < 0) {
        perror("listen");
        close(server_state.server_fd);
        unlink(socket_path);
        return -1;
    }

    if (ipc_shm_init() != 0) {
        close(server_state.server_fd);
        unlink(socket_path);
        return -1;
    }

    if (ipc_sem_init() != 0) {
        fprintf(stderr, "Semaphore init failed\n");
    }

    server_state.running = 1;
    server_state.num_clients = 0;

    return 0;
}

void ipc_server_cleanup(void) {
    server_state.running = 0;

    pthread_mutex_lock(&server_state.clients_mutex);
    for (int i = 0; i < IPC_MAX_CLIENTS; i++) {
        if (server_state.clients[i].active) {
            close(server_state.clients[i].fd);
            server_state.clients[i].active = 0;
        }
    }
    pthread_mutex_unlock(&server_state.clients_mutex);

    if (server_state.server_fd >= 0) {
        close(server_state.server_fd);
        unlink(addr.sun_path);
    }

    ipc_shm_cleanup();
    ipc_sem_cleanup();
    pthread_mutex_destroy(&server_state.clients_mutex);
}

int ipc_accept_client(int server_fd) {
    struct sockaddr_un client_addr;
    socklen_t len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return -1;
    }

    pthread_mutex_lock(&server_state.clients_mutex);
    int slot = -1;
    for (int i = 0; i < IPC_MAX_CLIENTS; i++) {
        if (!server_state.clients[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        pthread_mutex_unlock(&server_state.clients_mutex);
        close(client_fd);
        return -1;
    }

    server_state.clients[slot].fd = client_fd;
    server_state.clients[slot].connected_at = time(NULL);
    server_state.clients[slot].active = 1;
    server_state.num_clients++;

    pthread_mutex_unlock(&server_state.clients_mutex);
    return client_fd;
}

void ipc_disconnect_client(int client_fd) {
    pthread_mutex_lock(&server_state.clients_mutex);

    for (int i = 0; i < IPC_MAX_CLIENTS; i++) {
        if (server_state.clients[i].active &&
            server_state.clients[i].fd == client_fd) {
            close(client_fd);
            server_state.clients[i].active = 0;
            server_state.num_clients--;
            break;
        }
    }

    pthread_mutex_unlock(&server_state.clients_mutex);
}

int ipc_read_request(int client_fd, request_t *req) {
    ssize_t n = read(client_fd, req, sizeof(request_t));
    if (n != (ssize_t)sizeof(request_t)) {
        return -1;
    }
    return 0;
}

int ipc_send_response(int client_fd, const response_t *resp) {
    ssize_t n = write(client_fd, resp, sizeof(response_t));
    if (n != (ssize_t)sizeof(response_t)) {
        return -1;
    }
    return 0;
}

int ipc_dispatch_command(command_type_t cmd, const char *payload,
                        char *result, size_t result_size) {
    (void)payload;

    switch (cmd) {
        case CMD_STATUS:
            snprintf(result, result_size,
                     "Running:1 Clients:%d Uptime:%ld",
                     server_state.num_clients,
                     time(NULL) - (shared_status ? shared_status->started_at : 0));
            return STATUS_OK;

        case CMD_SHUTDOWN:
            server_state.running = 0;
            snprintf(result, result_size, "Shutting down daemon");
            return STATUS_OK;

        default:
            snprintf(result, result_size, "Invalid command");
            return STATUS_INVALID_COMMAND;
    }
}

int ipc_handle_request(int client_fd, request_t *req, response_t *resp) {
    (void)client_fd;
    memset(resp, 0, sizeof(response_t));
    resp->request_id = req->request_id;

    char result[IPC_MAX_PAYLOAD_SIZE];
    memset(result, 0, sizeof(result));

    int status = ipc_dispatch_command(req->command,
                                      (const char*)req->payload,
                                      result,
                                      sizeof(result));
    resp->status = status;

    if (status == STATUS_OK) {
        resp->data_size = strlen(result) + 1;
        if (resp->data_size > IPC_MAX_PAYLOAD_SIZE)
            resp->data_size = IPC_MAX_PAYLOAD_SIZE;
        memcpy(resp->data, result, resp->data_size);
    } else {
        snprintf(resp->error_msg, sizeof(resp->error_msg),
                 "%s", "Command failed");
    }

    return 0;
}

int ipc_shm_init(void) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        return -1;
    }

    if (ftruncate(shm_fd, sizeof(system_status_t)) != 0) {
        close(shm_fd);
        return -1;
    }

    shared_status = mmap(NULL, sizeof(system_status_t),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        shm_fd, 0);

    close(shm_fd);
    if (shared_status == MAP_FAILED) {
        return -1;
    }

    shared_status->daemon_running = 1;
    shared_status->started_at = time(NULL);

    return 0;
}

void ipc_shm_cleanup(void) {
    if (shared_status) {
        munmap(shared_status, sizeof(system_status_t));
        shared_status = NULL;
    }
    shm_unlink(SHM_NAME);
}

int ipc_shm_update_status(const system_status_t *status) {
    if (!shared_status || !status) return -1;
    *shared_status = *status;
    return 0;
}

int ipc_shm_get_status(system_status_t *status) {
    if (!shared_status || !status) return -1;
    *status = *shared_status;
    return 0;
}

int ipc_sem_init(void) {
    status_sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (status_sem == SEM_FAILED) {
        return -1;
    }
    return 0;
}

void ipc_sem_cleanup(void) {
    if (status_sem) {
        sem_close(status_sem);
        sem_unlink(SEM_NAME);
        status_sem = NULL;
    }
}

int ipc_sem_wait(const char *name) {
    (void)name;
    if (!status_sem) return -1;
    return sem_wait(status_sem);
}

int ipc_sem_post(const char *name) {
    (void)name;
    if (!status_sem) return -1;
    return sem_post(status_sem);
}

int ipc_server_run(void) {
    fd_set read_fds;
    int max_fd;

    while (server_state.running) {
        FD_ZERO(&read_fds);
        FD_SET(server_state.server_fd, &read_fds);
        max_fd = server_state.server_fd;

        pthread_mutex_lock(&server_state.clients_mutex);
        for (int i = 0; i < IPC_MAX_CLIENTS; i++) {
            if (server_state.clients[i].active) {
                int fd = server_state.clients[i].fd;
                FD_SET(fd, &read_fds);
                if (fd > max_fd) max_fd = fd;
            }
        }
        pthread_mutex_unlock(&server_state.clients_mutex);

        struct timeval timeout = {1, 0};
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity <= 0)
            continue;

        if (FD_ISSET(server_state.server_fd, &read_fds)) {
            ipc_accept_client(server_state.server_fd);
        }

        pthread_mutex_lock(&server_state.clients_mutex);
        for (int i = 0; i < IPC_MAX_CLIENTS; i++) {
            if (!server_state.clients[i].active)
                continue;
            int fd = server_state.clients[i].fd;

            if (FD_ISSET(fd, &read_fds)) {
                request_t req;
                response_t resp;

                if (ipc_read_request(fd, &req) == 0) {
                    ipc_handle_request(fd, &req, &resp);
                    ipc_send_response(fd, &resp);
                }

                /* Siempre cerramos y marcamos como inactivo tras la respuesta */
                close(fd);
                server_state.clients[i].active = 0;
                server_state.num_clients--;
            }
        }
        pthread_mutex_unlock(&server_state.clients_mutex);
    }

    return 0;
}

int ipc_mq_init(void) {
    key_t key = ftok("/tmp", 'S');
    if (key < 0) {
        return -1;
    }

    msg_queue_id = msgget(key, IPC_CREAT | 0666);
    if (msg_queue_id < 0) {
        return -1;
    }

    return 0;
}

void ipc_mq_cleanup(void) {
    if (msg_queue_id >= 0) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
        msg_queue_id = -1;
    }
}

struct msg_buffer_sysv {
    long msg_type;
    job_message_t job;
};

int ipc_mq_send_job(const job_message_t *job) {
    if (msg_queue_id < 0) return -1;

    struct msg_buffer_sysv msg;
    msg.msg_type = job->priority;
    memcpy(&msg.job, job, sizeof(job_message_t));

    if (msgsnd(msg_queue_id, &msg, sizeof(job_message_t), 0) < 0) {
        return -1;
    }

    return 0;
}

int ipc_mq_receive_job(job_message_t *job, int wait) {
    if (msg_queue_id < 0) return -1;

    struct msg_buffer_sysv msg;
    int flags = wait ? 0 : IPC_NOWAIT;

    if (msgrcv(msg_queue_id, &msg, sizeof(job_message_t), 0, flags) < 0) {
        if (errno == ENOMSG) return -1;
        return -1;
    }

    memcpy(job, &msg.job, sizeof(job_message_t));
    return 0;
}

int ipc_server_stop(void) {
    server_state.running = 0;
    return 0;
}

const char* ipc_command_to_string(command_type_t cmd) {
    switch (cmd) {
        case CMD_STATUS: return "STATUS";
        case CMD_SHUTDOWN: return "SHUTDOWN";
        default: return "OTHER";
    }
}

const char* ipc_status_to_string(status_code_t s) {
    switch (s) {
        case STATUS_OK: return "OK";
        case STATUS_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}


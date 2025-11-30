#ifndef IPC_SERVER_H
#define IPC_SERVER_H

#include <stdint.h>
#include <sys/types.h>

#define IPC_SOCKET_PATH "/var/run/storage_mgr.sock"
#define IPC_PROTOCOL_VERSION 1
#define IPC_MAX_PAYLOAD_SIZE 8192
#define IPC_MAX_CLIENTS 64

// Códigos de comando
typedef enum {
    CMD_STATUS,
    CMD_RAID_CREATE,
    CMD_RAID_STATUS,
    CMD_RAID_ADD_DISK,
    CMD_RAID_REMOVE_DISK,
    CMD_RAID_FAIL_DISK,
    CMD_LVM_PV_CREATE,
    CMD_LVM_VG_CREATE,
    CMD_LVM_LV_CREATE,
    CMD_LVM_LV_EXTEND,
    CMD_LVM_SNAPSHOT,
    CMD_FS_CREATE,
    CMD_FS_MOUNT,
    CMD_FS_UNMOUNT,
    CMD_FS_CHECK,
    CMD_BACKUP_CREATE,
    CMD_BACKUP_LIST,
    CMD_BACKUP_RESTORE,
    CMD_MONITOR_STATS,
    CMD_MONITOR_START,
    CMD_MONITOR_STOP,
    CMD_PERF_BENCHMARK,
    CMD_PERF_TUNE,
    CMD_SHUTDOWN
} command_type_t;

// Códigos de estado
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = -1,
    STATUS_INVALID_COMMAND = -2,
    STATUS_PERMISSION_DENIED = -3,
    STATUS_DEVICE_NOT_FOUND = -4,
    STATUS_OPERATION_FAILED = -5,
    STATUS_BUSY = -6,
    STATUS_TIMEOUT = -7
} status_code_t;

// Estructura de request
typedef struct {
    uint32_t version;
    uint32_t request_id;
    command_type_t command;
    uint32_t payload_size;
    char payload[IPC_MAX_PAYLOAD_SIZE];
} request_t;

// Estructura de response
typedef struct {
    uint32_t request_id;
    status_code_t status;
    char error_msg[256];
    uint32_t data_size;
    char data[IPC_MAX_PAYLOAD_SIZE];
} response_t;

// Información del cliente conectado
typedef struct {
    int fd;
    pid_t pid;
    uid_t uid;
    time_t connected_at;
    int active;
} client_info_t;

// Estado del servidor IPC
typedef struct {
    int server_fd;
    int running;
    int num_clients;
    client_info_t clients[IPC_MAX_CLIENTS];
    pthread_mutex_t clients_mutex;
} ipc_server_state_t;

// Shared memory para estado del sistema
typedef struct {
    int daemon_running;
    time_t started_at;
    uint64_t total_requests;
    uint64_t failed_requests;
    int active_operations;
    char current_operation[256];
    double cpu_usage;
    double memory_usage_mb;
} system_status_t;

// Message queue para operaciones asíncronas
typedef struct {
    command_type_t command;
    char params[512];
    int priority;
    time_t queued_at;
} job_message_t;

// Inicialización del servidor
int ipc_server_init(const char *socket_path);
void ipc_server_cleanup(void);

// Ejecución del servidor
int ipc_server_run(void);
int ipc_server_stop(void);

// Gestión de clientes
int ipc_accept_client(int server_fd);
void ipc_disconnect_client(int client_fd);
int ipc_get_client_info(int client_fd, client_info_t *info);

// Procesamiento de requests
int ipc_handle_request(int client_fd, request_t *req, response_t *resp);
int ipc_dispatch_command(command_type_t cmd, const char *payload, 
                        char *result, size_t result_size);

// Funciones de lectura/escritura
int ipc_read_request(int client_fd, request_t *req);
int ipc_send_response(int client_fd, const response_t *resp);

// Shared memory para estado
int ipc_shm_init(void);
void ipc_shm_cleanup(void);
int ipc_shm_update_status(const system_status_t *status);
int ipc_shm_get_status(system_status_t *status);

// Message queue para jobs
int ipc_mq_init(void);
void ipc_mq_cleanup(void);
int ipc_mq_send_job(const job_message_t *job);
int ipc_mq_receive_job(job_message_t *job, int wait);

// Semáforos para sincronización
int ipc_sem_init(void);
void ipc_sem_cleanup(void);
int ipc_sem_wait(const char *sem_name);
int ipc_sem_post(const char *sem_name);

// Utilidades
const char* ipc_command_to_string(command_type_t cmd);
const char* ipc_status_to_string(status_code_t status);

#endif // IPC_SERVER_H
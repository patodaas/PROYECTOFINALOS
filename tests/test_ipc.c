#include "ipc_server.h"
#include "performance_tuner.h"

int main(void) {
    ipc_server_init(IPC_SOCKET_PATH);
    request_t req;
    response_t resp;
    benchmark_results_t results;
    req.version = IPC_PROTOCOL_VERSION;
    req.request_id = 1;
    req.command = CMD_PERF_BENCHMARK;
    perf_benchmark("/dev/sda", "/tmp/testfile", &results);
    ipc_server_cleanup();
    return 0;
}


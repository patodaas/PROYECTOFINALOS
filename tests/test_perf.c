#include "performance_tuner.h"

int main(void) {
    benchmark_results_t results;
    perf_list_schedulers("/dev/sda", NULL, NULL);
    perf_benchmark("/dev/sda", "/tmp/testfile", &results);
    return 0;
}


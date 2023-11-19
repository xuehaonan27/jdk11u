//
// Created by 华子曰 on 2023/11/19.
//

#ifndef SHARE_VM_PERF_PERFLOGGER_HPP
#define SHARE_VM_PERF_PERFLOGGER_HPP

#include <perf/utilities.h>

class PerfLogger {
public:
    static perf_measurement_t *create_measurement(unsigned type, unsigned config, pid_t pid, int cpu);
};


#endif //SHARE_VM_PERF_PERFLOGGER_HPP

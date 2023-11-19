//
// Created by 华子曰 on 2023/11/19.
//

#include "perf/perfLogger.hpp"
#include <perf/utilities.h>

static perf_measurement_t* PerfLogger::create_measurement(unsigned type, unsigned config, pid_t pid, int cpu){
    return perf_create_measurement(type, config, pid, cpu);
}
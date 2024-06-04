#ifndef XHN_JVM_X86_ALLOCATION_COUNTER_HPP
#define XHN_JVM_X86_ALLOCATION_COUNTER_HPP

#include "../../share/gc/shared/memAllocator.hpp"
#include "allocation_counter.hpp"
AtomicSizet RuntimeAllocationCounter::interpreter_fast_tlab_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_fast_tlab_time = AtomicJLong();
size_t RuntimeAllocationCounter::interpreter_fast_tlab_cnt_raw = 0;
size_t RuntimeAllocationCounter::interpreter_fast_eden_cnt_raw = 0;
size_t RuntimeAllocationCounter::interpreter_slow_cnt_raw = 0;

AtomicSizet RuntimeAllocationCounter::interpreter_fast_eden_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_fast_eden_time = AtomicJLong();

AtomicSizet RuntimeAllocationCounter::interpreter_slow_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_slow_time = AtomicJLong();
// Global counter
static RuntimeAllocationCounter runtimeAllocationCounter = RuntimeAllocationCounter();
#endif
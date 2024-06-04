#ifndef XHN_JVM_X86_ALLOCATION_COUNTER_HPP
#define XHN_JVM_X86_ALLOCATION_COUNTER_HPP

#include "allocation_counter.hpp"
AtomicSizet2 RuntimeAllocationCounter::interpreter_fast_tlab_cnt = AtomicSizet2();
AtomicJLong2 RuntimeAllocationCounter::interpreter_fast_tlab_time = AtomicJLong2();
size_t RuntimeAllocationCounter::interpreter_fast_tlab_cnt_raw = 0;
size_t RuntimeAllocationCounter::interpreter_fast_eden_cnt_raw = 0;
size_t RuntimeAllocationCounter::interpreter_slow_cnt_raw = 0;

AtomicSizet2 RuntimeAllocationCounter::interpreter_fast_eden_cnt = AtomicSizet2();
AtomicJLong2 RuntimeAllocationCounter::interpreter_fast_eden_time = AtomicJLong2();

AtomicSizet2 RuntimeAllocationCounter::interpreter_slow_cnt = AtomicSizet2();
AtomicJLong2 RuntimeAllocationCounter::interpreter_slow_time = AtomicJLong2();
#endif
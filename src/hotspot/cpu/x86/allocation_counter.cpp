#ifndef XHN_JVM_X86_ALLOCATION_COUNTER_HPP
#define XHN_JVM_X86_ALLOCATION_COUNTER_HPP

#include "allocation_counter.hpp"
AtomicSizet RuntimeAllocationCounter::interpreter_fast_tlab_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_fast_tlab_time = AtomicJLong();

AtomicSizet RuntimeAllocationCounter::interpreter_fast_eden_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_fast_eden_time = AtomicJLong();

AtomicSizet RuntimeAllocationCounter::interpreter_slow_cnt = AtomicSizet();
AtomicJLong RuntimeAllocationCounter::interpreter_slow_time = AtomicJLong();
#endif
// Allocation counter when using assembly code (interpreter/c1/c2)
#ifndef XHN_JVM_CPU_X86_ALLOCATION_COUNTER_HPP
#define XHN_JVM_CPU_X86_ALLOCATION_COUNTER_HPP
#include "../../share/runtime/atomic.hpp"
#include "../../share/logging/log.hpp"
#include "../../share/gc/shared/memAllocator.hpp"
#include "rdtsc_x86.hpp"

class AtomicSizet;
class AtomicJLong;

class RuntimeAllocationCounter {
private:
  // Fast path tlab
  static AtomicSizet interpreter_fast_tlab_cnt;
  static AtomicJLong interpreter_fast_tlab_time;
  

  // Fast path eden
  static AtomicSizet interpreter_fast_eden_cnt;
  static AtomicJLong interpreter_fast_eden_time;

  // Allocation slow path
  static AtomicSizet interpreter_slow_cnt;
  static AtomicJLong interpreter_slow_time;
public:
  static size_t interpreter_fast_tlab_cnt_raw;
  static size_t interpreter_fast_eden_cnt_raw;
  static size_t interpreter_slow_cnt_raw;
  static inline jlong now() {
    return Rdtsc::raw();
  }

  static void interpreter_fast_tlab_cnt_inc() {interpreter_fast_tlab_cnt.inc();}
  static void interpreter_fast_tlab_cnt_clear() {interpreter_fast_tlab_cnt.clear();}
  size_t get_interpreter_fast_tlab_cnt() {return interpreter_fast_tlab_cnt.inspect();}

  static void interpreter_fast_tlab_time_set(jlong time) {interpreter_fast_tlab_time.store(time);}
  static void interpreter_fast_tlab_time_add(jlong time) {interpreter_fast_tlab_time.add(time);}
  static void interpreter_fast_tlab_time_clear() {interpreter_fast_tlab_time.clear();}
  jlong get_interpreter_fast_tlab_time() {return interpreter_fast_tlab_time.inspect();}

  void log_gc_info() {
    log_info(gc)("[RuntimeAllocationCounter]\nCounter: fast_tlab_raw(%lu) fast_eden_raw(%lu) slow_raw(%lu)\nTimer: fast_tlab(%lu)\n",
      interpreter_fast_tlab_cnt_raw,
      interpreter_fast_eden_cnt_raw,
      interpreter_slow_cnt_raw,
      get_interpreter_fast_tlab_time()
    );
  }

  RuntimeAllocationCounter() {}
};

// Global counter
static RuntimeAllocationCounter runtimeAllocationCounter;
#endif
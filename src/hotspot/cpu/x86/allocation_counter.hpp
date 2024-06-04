// Allocation counter when using assembly code (interpreter/c1/c2)
#ifndef XHN_JVM_CPU_X86_ALLOCATION_COUNTER_HPP
#define XHN_JVM_CPU_X86_ALLOCATION_COUNTER_HPP
#include "../../share/runtime/atomic.hpp"
#include "../../share/logging/log.hpp"
#include "rdtsc_x86.hpp"

#ifndef XHN_JVM_X86_ATOMIC_PROTOTYPE
#define XHN_JVM_X86_ATOMIC_PROTOTYPE
// A new type to constraint the behavior of counters.
// Using inlines to reduce the cost of function calling.
class AtomicSizet2 {
private:
  size_t inner;
public:
  AtomicSizet2(): inner(0) {}
  AtomicSizet2(size_t inner): inner(inner) {}
  inline void add(size_t rhs) {
    Atomic::add<size_t, size_t>(rhs, &inner);
  }
  inline void sub(size_t rhs) {
    Atomic::sub<size_t, size_t>(rhs, &inner);
  }
  inline void inc() {
    Atomic::inc<size_t>(&inner);
  }
  inline void dec() {
    Atomic::dec<size_t>(&inner);
  }
  inline size_t inspect() const {
    return Atomic::load<size_t>(&inner);
  }
  inline void clear() {
    Atomic::store<size_t, size_t>(size_t(0), &inner);
  }
};

class AtomicJLong2 {
private:
  jlong inner;
public:
  AtomicJLong2(): inner(0) {}
  AtomicJLong2(jlong inner): inner(inner) {}
  inline void add(jlong rhs) {
    // Atomic::add<jlong, jlong>(rhs, &inner);
    Atomic::store<jlong, jlong>(rhs, &inner);
  }
  inline void sub(jlong rhs) {
    Atomic::sub<jlong, jlong>(rhs, &inner);
  }
  inline void store(jlong rhs) {
    Atomic::store<jlong, jlong>(rhs, &inner);
  }
  inline jlong inspect() const {
    return Atomic::load<jlong>(&inner);
  }
  inline void clear() {
    Atomic::store<jlong, jlong>(jlong(0), &inner);
  }
};
#endif

class RuntimeAllocationCounter {
private:
  // Fast path tlab
  static AtomicSizet2 interpreter_fast_tlab_cnt;
  static AtomicJLong2 interpreter_fast_tlab_time;
  

  // Fast path eden
  static AtomicSizet2 interpreter_fast_eden_cnt;
  static AtomicJLong2 interpreter_fast_eden_time;

  // Allocation slow path
  static AtomicSizet2 interpreter_slow_cnt;
  static AtomicJLong2 interpreter_slow_time;
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
/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_GC_SHARED_MEM_ALLOCATOR_HPP
#define SHARE_GC_SHARED_MEM_ALLOCATOR_HPP

#include "gc/shared/collectedHeap.hpp"
#include "memory/memRegion.hpp"
#include "oops/oopsHierarchy.hpp"
#include "utilities/exceptions.hpp"
#include "utilities/macros.hpp"
#include "runtime/atomic.hpp"

// A new type to constraint the behavior of counters.
// Using inlines to reduce the cost of function calling.
class AtomicSizet {
private:
  size_t inner;
public:
  AtomicSizet(): inner(0) {}
  AtomicSizet(size_t inner): inner(inner) {}
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

class AtomicJLong {
private:
  jlong inner;
public:
  AtomicJLong(): inner(0) {}
  AtomicJLong(jlong inner): inner(inner) {}
  inline void add(jlong rhs) {
    Atomic::add<jlong, jlong>(rhs, &inner);
  }
  inline void sub(jlong rhs) {
    Atomic::sub<jlong, jlong>(rhs, &inner);
  }
  inline jlong inspect() const {
    return Atomic::load<jlong>(&inner);
  }
  inline void clear() {
    Atomic::store<jlong, jlong>(jlong(0), &inner);
  }
};

// These fascilities are used for allocating, and initializing newly allocated objects.

class MemAllocator: StackObj {
  class Allocation;

protected:
  CollectedHeap* const _heap;
  Thread* const        _thread;
  Klass* const         _klass;
  const size_t         _word_size;

  // Counters of allocations happened
  static AtomicSizet   _cnt_allocate_inside_tlab_direct;
  static AtomicSizet   _cnt_allocate_inside_tlab_slow;
  static AtomicSizet   _cnt_allocate_from_heap;

  // Timers of allocations happened
  // Try to tlab allocation
  static AtomicJLong   _tot_time_goes_allocate_inside_tlab_direct_try;
  // Tlab allocation fail, slow path
  static AtomicJLong   _tot_time_goes_allocate_inside_tlab_slow;
  // Tlab allocation slow path failed, allocate from heap
  static AtomicJLong   _tot_time_goes_allocate_outside_tlab;
  
  static AtomicUInt128t test;

public:
  inline size_t inspect_cnt_allocate_inside_tlab_direct() const {
    return this->_cnt_allocate_inside_tlab_direct.inspect();
  }

  inline size_t inspect_cnt_allocate_inside_tlab_slow() const {
    return this->_cnt_allocate_inside_tlab_slow.inspect();
  }

  inline size_t inspect_cnt_allocate_from_heap() const {
    return this->_cnt_allocate_from_heap.inspect();
  }

  inline jlong inspect_tot_time_goes_allocate_inside_tlab_direct_try() const {
    return this->_tot_time_goes_allocate_inside_tlab_direct_try.inspect();
  }

  inline jlong inspect_tot_time_goes_allocate_inside_tlab_slow() const {
    return this->_tot_time_goes_allocate_inside_tlab_slow.inspect();
  }

  inline jlong inspect_tot_time_goes_allocate_outside_tlab() const {
    return this->_tot_time_goes_allocate_outside_tlab.inspect();
  }

  void log_gc_info() const {
    log_info(gc)("[MemAllocator State]\nCounter: dir(%lu), slow(%lu), heap(%lu)\nTimer: dir(%lu), slow(%lu), heap(%lu)\n",
    inspect_cnt_allocate_inside_tlab_direct(),
    inspect_cnt_allocate_inside_tlab_slow(),
    inspect_cnt_allocate_from_heap(),
    inspect_tot_time_goes_allocate_inside_tlab_direct_try(),
    inspect_tot_time_goes_allocate_inside_tlab_slow(),
    inspect_tot_time_goes_allocate_outside_tlab());
  }

private:
  // Allocate from the current thread's TLAB, with broken-out slow path.
  HeapWord* allocate_inside_tlab(Allocation& allocation) const;
  HeapWord* allocate_inside_tlab_slow(Allocation& allocation) const;
  HeapWord* allocate_inside_tlab_slow_cms(Allocation& allocation) const;
  HeapWord* allocate_outside_tlab(Allocation& allocation) const;

protected:
  MemAllocator(Klass* klass, size_t word_size, Thread* thread)
    : _heap(Universe::heap()),
      _thread(thread),
      _klass(klass),
      _word_size(word_size)
  { }

  // This function clears the memory of the object
  void mem_clear(HeapWord* mem) const;
  // This finish constructing an oop by installing the mark word and the Klass* pointer
  // last. At the point when the Klass pointer is initialized, this is a constructed object
  // that must be parseable as an oop by concurrent collectors.
  oop finish(HeapWord* mem) const;

  // Raw memory allocation. This may or may not use TLAB allocations to satisfy the
  // allocation. A GC implementation may override this function to satisfy the allocation
  // in any way. But the default is to try a TLAB allocation, and otherwise perform
  // mem_allocate.
  virtual HeapWord* mem_allocate(Allocation& allocation) const;

  virtual MemRegion obj_memory_range(oop obj) const {
    return MemRegion((HeapWord*)obj, _word_size);
  }

public:
  oop allocate() const;
  virtual oop initialize(HeapWord* mem) const = 0;
};

class ObjAllocator: public MemAllocator {
public:
  ObjAllocator(Klass* klass, size_t word_size, Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread) {}
  virtual oop initialize(HeapWord* mem) const;
};

class ObjArrayAllocator: public MemAllocator {
  const int  _length;
  const bool _do_zero;
protected:
  virtual MemRegion obj_memory_range(oop obj) const;

public:
  ObjArrayAllocator(Klass* klass, size_t word_size, int length, bool do_zero,
                    Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread),
      _length(length),
      _do_zero(do_zero) {}
  virtual oop initialize(HeapWord* mem) const;
};

class ClassAllocator: public MemAllocator {
public:
  ClassAllocator(Klass* klass, size_t word_size, Thread* thread = Thread::current())
    : MemAllocator(klass, word_size, thread) {}
  virtual oop initialize(HeapWord* mem) const;
};

#endif // SHARE_GC_SHARED_MEM_ALLOCATOR_HPP

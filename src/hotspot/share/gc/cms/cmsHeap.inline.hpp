/*
 * Copyright (c) 2001, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_CMS_CMSHEAP_INLINE_HPP
#define SHARE_GC_CMS_CMSHEAP_INLINE_HPP

#include "gc/cms/cmsHeap.hpp"
#include "gc/cms/concurrentMarkSweepGeneration.inline.hpp"
#include "gc/cms/concurrentMarkSweepGeneration.hpp"
#include "gc/serial/defNewGeneration.inline.hpp"
// #include "gc/shared/gcTraceTime.hpp"
#include "gc/shared/gcTrace.hpp"
#include "gc/shared/gcTraceTime.inline.hpp"
#include "services/memoryService.hpp"

template <typename OopClosureType1, typename OopClosureType2>
void CMSHeap::oop_since_save_marks_iterate(OopClosureType1* cur,
                                           OopClosureType2* older) {
  young_gen()->oop_since_save_marks_iterate(cur);
  old_gen()->oop_since_save_marks_iterate(older);
}

// void CMSHeap::do_collection(bool           full,
//                                      bool           clear_all_soft_refs,
//                                      size_t         size,
//                                      bool           is_tlab,
//                                      GenerationType max_generation) {
//   if (GCLocker::check_active_before_gc()) {
//     return;
//   }

//   assert(Thread::current()->is_VM_thread(), "Should be VM thread");

  
//   uint _full_gc_count_before = 0;
//   {
//     MutexLockerEx x(FullGCCount_lock, Mutex::_no_safepoint_check_flag);
//     _full_gc_count_before = total_full_collections();
//   }
//   bool complete = full && (max_generation == OldGen);

//   TraceMemoryManagerStats tmms(old_gen()->gc_manager(), gc_cause());

//   CMSCollector::request_full_gc( _full_gc_count_before, gc_cause());
//   log_info(gc)("Full gc request send notify");

//   {
//     MutexLockerEx fb(FgBgSync_lock, Mutex::_no_safepoint_check_flag);
//     FgBgSync_lock->notify();
//   }
  

//   wait_for_background(_full_gc_count_before);
//   log_info(gc)("do collection finished");

// }

void CMSHeap::do_old_collection(){

  assert(Thread::current()->is_VM_thread(), "Should be VM thread");

  
  uint _full_gc_count_before = 0;
  {
    MutexLockerEx x(FullGCCount_lock, Mutex::_no_safepoint_check_flag);
    _full_gc_count_before = total_full_collections();
  }

  TraceMemoryManagerStats tmms(old_gen()->gc_manager(), gc_cause());

  CMSCollector::request_full_gc( _full_gc_count_before, gc_cause());
  log_info(gc)("Full gc request send notify");

  {
    MutexLockerEx fb(FgBgSync_lock, Mutex::_no_safepoint_check_flag);
    FgBgSync_lock->notify();
  }

  wait_for_background(_full_gc_count_before);
}

void CMSHeap::do_collection(bool           full,
                                    bool           clear_all_soft_refs,
                                    size_t         size,
                                    bool           is_tlab,
                                    GenerationType max_generation) {
  ResourceMark rm;
  DEBUG_ONLY(Thread* my_thread = Thread::current();)

  assert(SafepointSynchronize::is_at_safepoint(), "should be at safepoint");
  assert(my_thread->is_VM_thread() ||
          my_thread->is_ConcurrentGC_thread(),
          "incorrect thread type capability");
  // assert(Heap_lock->is_locked(),
  //         "the requesting thread should have the Heap_lock");
  guarantee(!is_gc_active(), "collection is not reentrant");

  if (GCLocker::check_active_before_gc()) {
    return; // GC is disabled (e.g. JNI GetXXXCritical operation)
  }


  const bool do_clear_all_soft_refs = clear_all_soft_refs ||
                                      soft_ref_policy()->should_clear_all_soft_refs();

  ClearedAllSoftRefs casr(do_clear_all_soft_refs, soft_ref_policy());

  print_heap_before_gc();

  {
    FlagSetting fl(_is_gc_active, true);

    bool complete = full && (max_generation == OldGen);
    bool old_collects_young = complete && !ScavengeBeforeFullGC;
    bool do_young_collection = !old_collects_young && _young_gen->should_collect(full, size, is_tlab);

    FormatBuffer<> gc_string("%s", "Pause ");
    if (do_young_collection) {
      gc_string.append("Young");
    } else {
      gc_string.append("Full");
    }

    // GCTraceCPUTime tcpu;
    // GCTraceTime(Info, gc) t(gc_string, NULL, gc_cause(), true);

    
    gc_prologue(complete);
    
    increment_total_collections(complete);

    size_t young_prev_used = _young_gen->used();
    size_t old_prev_used = _old_gen->used();
    const metaspace::MetaspaceSizesSnapshot prev_meta_sizes;

    bool run_verification = total_collections() >= VerifyGCStartAt;

    bool prepared_for_verification = false;
    bool collected_old = false;

    if (do_young_collection) {
      GCIdMark gc_id_mark;
      GCTraceCPUTime tcpu;
      GCTraceTime(Info, gc) t(gc_string, NULL, gc_cause(), true);
      if (run_verification && VerifyGCLevel <= 0 && VerifyBeforeGC) {
        prepare_for_verify();
        prepared_for_verification = true;
      }

      collect_generation(_young_gen,
                          full,
                          size,
                          is_tlab,
                          run_verification && VerifyGCLevel <= 0,
                          do_clear_all_soft_refs,
                          false);
      // do_old_collection();

      if (size > 0 && (!is_tlab || _young_gen->supports_tlab_allocation()) &&
          size * HeapWordSize <= _young_gen->unsafe_max_alloc_nogc()) {
        // Allocation request was met by young GC.
        size = 0;
      }
    }

    bool must_restore_marks_for_biased_locking = false;

    if (max_generation == OldGen && _old_gen->should_collect(full, size, is_tlab)) {
      if (!complete) {
        // The full_collections increment was missed above.
        increment_total_full_collections();
      }

      if (!prepared_for_verification && run_verification &&
          VerifyGCLevel <= 1 && VerifyBeforeGC) {
        prepare_for_verify();
      }

      if (do_young_collection) {
        // We did a young GC. Need a new GC id for the old GC.
        // GCIdMark gc_id_mark;
        do_old_collection();
        // collect_generation(_old_gen, full, size, is_tlab, run_verification && VerifyGCLevel <= 1, do_clear_all_soft_refs, true);
      } else {
        // No young GC done. Use the same GC id as was set up earlier in this method.
        do_old_collection();
        // collect_generation(_old_gen, full, size, is_tlab, run_verification && VerifyGCLevel <= 1, do_clear_all_soft_refs, true);
      }

      must_restore_marks_for_biased_locking = true;
      collected_old = true;
    }

    // Update "complete" boolean wrt what actually transpired --
    // for instance, a promotion failure could have led to
    // a whole heap collection.
    complete = complete || collected_old;

    print_heap_change(young_prev_used, old_prev_used);
    MetaspaceUtils::print_metaspace_change(prev_meta_sizes);

    log_info(gc)("increment 20 %d", incremental_collection_failed()?1:0);
    // Adjust generation sizes.
    if (collected_old) {
      _old_gen->compute_new_size();
    }
    _young_gen->compute_new_size();

    if (complete) {
      // Delete metaspaces for unloaded class loaders and clean up loader_data graph
      ClassLoaderDataGraph::purge();
      MetaspaceUtils::verify_metrics();
      // Resize the metaspace capacity after full collections
      MetaspaceGC::compute_new_size();
      update_full_collections_completed();
    }

    // Track memory usage and detect low memory after GC finishes
    MemoryService::track_memory_usage();

    log_info(gc)("increment 21 %d", incremental_collection_failed()?1:0);
    gc_epilogue(complete);

    // if (must_restore_marks_for_biased_locking) {
    //   BiasedLocking::restore_marks();
    // }
  }

  print_heap_after_gc();

  #ifdef TRACESPINNING
  ParallelTaskTerminator::print_termination_counts();
  #endif

  log_info(gc)("increment 22 %d", incremental_collection_failed()?1:0);
}

#endif // SHARE_GC_CMS_CMSHEAP_INLINE_HPP

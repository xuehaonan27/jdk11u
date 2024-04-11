/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "precompiled.hpp"
#include "gc/cms/cmsCardTable.hpp"
#include "gc/cms/compactibleFreeListSpace.hpp"
#include "gc/cms/concurrentMarkSweepGeneration.hpp"
#include "gc/cms/concurrentMarkSweepThread.hpp"
#include "gc/cms/cmsHeap.hpp"
#include "gc/cms/cms_globals.hpp"
#include "gc/cms/parNewGeneration.hpp"
#include "gc/cms/vmCMSOperations.hpp"
#include "gc/shared/genCollectedHeap.hpp"
#include "gc/shared/genMemoryPools.hpp"
#include "gc/shared/genOopClosures.inline.hpp"
#include "gc/shared/strongRootsScope.hpp"
#include "gc/shared/workgroup.hpp"
#include "oops/oop.inline.hpp"
#include "runtime/vmThread.hpp"
#include "services/memoryManager.hpp"
#include "utilities/stack.inline.hpp"

class CompactibleFreeListSpacePool : public CollectedMemoryPool {
private:
  CompactibleFreeListSpace* _space;
public:
  CompactibleFreeListSpacePool(CompactibleFreeListSpace* space,
                               const char* name,
                               size_t max_size,
                               bool support_usage_threshold) :
    CollectedMemoryPool(name, space->capacity(), max_size, support_usage_threshold),
    _space(space) {
  }

  MemoryUsage get_memory_usage() {
    size_t max_heap_size   = (available_for_allocation() ? max_size() : 0);
    size_t used      = used_in_bytes();
    size_t committed = _space->capacity();

    return MemoryUsage(initial_size(), used, committed, max_heap_size);
  }

  size_t used_in_bytes() {
    return _space->used_stable();
  }
};

CMSHeap::CMSHeap(GenCollectorPolicy *policy) :
    GenCollectedHeap(policy,
                     Generation::ParNew,
                     Generation::ConcurrentMarkSweep,
                     "ParNew:CMS"),
    _eden_pool(NULL),
    _survivor_pool(NULL),
    _old_pool(NULL) {
  _workers = new WorkGang("GC Thread", ParallelGCThreads,
                          /* are_GC_task_threads */true,
                          /* are_ConcurrentGC_threads */false);
  _workers->initialize_workers();
}

jint CMSHeap::initialize() {
  jint status = GenCollectedHeap::initialize();
  if (status != JNI_OK) return status;

  // If we are running CMS, create the collector responsible
  // for collecting the CMS generations.
  if (!create_cms_collector()) {
    return JNI_ENOMEM;
  }

  return JNI_OK;
}

CardTableRS* CMSHeap::create_rem_set(const MemRegion& reserved_region) {
  return new CMSCardTable(reserved_region);
}

void CMSHeap::initialize_serviceability() {
  _young_manager = new GCMemoryManager("ParNew", "end of minor GC");
  _old_manager = new GCMemoryManager("ConcurrentMarkSweep", "end of major GC");

  ParNewGeneration* young = young_gen();
  _eden_pool = new ContiguousSpacePool(young->eden(),
                                       "Par Eden Space",
                                       young->max_eden_size(),
                                       false);

  _survivor_pool = new SurvivorContiguousSpacePool(young,
                                                   "Par Survivor Space",
                                                   young->max_survivor_size(),
                                                   false);

  ConcurrentMarkSweepGeneration* old = (ConcurrentMarkSweepGeneration*) old_gen();
  _old_pool = new CompactibleFreeListSpacePool(old->cmsSpace(),
                                               "CMS Old Gen",
                                               old->reserved().byte_size(),
                                               true);

  _young_manager->add_pool(_eden_pool);
  _young_manager->add_pool(_survivor_pool);
  young->set_gc_manager(_young_manager);

  _old_manager->add_pool(_eden_pool);
  _old_manager->add_pool(_survivor_pool);
  _old_manager->add_pool(_old_pool);
  old ->set_gc_manager(_old_manager);

}

CMSHeap* CMSHeap::heap() {
  CollectedHeap* heap = Universe::heap();
  assert(heap != NULL, "Uninitialized access to CMSHeap::heap()");
  assert(heap->kind() == CollectedHeap::CMS, "Invalid name");
  return static_cast<CMSHeap*>(heap);
}

void CMSHeap::gc_threads_do(ThreadClosure* tc) const {
  assert(workers() != NULL, "should have workers here");
  workers()->threads_do(tc);
  ConcurrentMarkSweepThread::threads_do(tc);
}

void CMSHeap::print_gc_threads_on(outputStream* st) const {
  assert(workers() != NULL, "should have workers here");
  workers()->print_worker_threads_on(st);
  ConcurrentMarkSweepThread::print_all_on(st);
}

void CMSHeap::print_on_error(outputStream* st) const {
  GenCollectedHeap::print_on_error(st);
  st->cr();
  CMSCollector::print_on_error(st);
}

bool CMSHeap::create_cms_collector() {
  assert(old_gen()->kind() == Generation::ConcurrentMarkSweep,
         "Unexpected generation kinds");
  CMSCollector* collector =
    new CMSCollector((ConcurrentMarkSweepGeneration*) old_gen(),
                     rem_set(),
                     (ConcurrentMarkSweepPolicy*) gen_policy());

  if (collector == NULL || !collector->completed_initialization()) {
    if (collector) {
      delete collector; // Be nice in embedded situation
    }
    vm_shutdown_during_initialization("Could not create CMS collector");
    return false;
  }
  return true; // success
}

void CMSHeap::collect(GCCause::Cause cause) {
//  if (should_do_concurrent_full_gc(cause)) {
//    // Mostly concurrent full collection.
//    collect_mostly_concurrent(cause);
//  } else {
    GenCollectedHeap::collect(cause);
//  }
}

bool CMSHeap::should_do_concurrent_full_gc(GCCause::Cause cause) {
  assert(false, "Should not reach here");
  switch (cause) {
    case GCCause::_gc_locker:           return GCLockerInvokesConcurrent;
    case GCCause::_java_lang_system_gc:
    case GCCause::_dcmd_gc_run:         return ExplicitGCInvokesConcurrent;
    default:                            return false;
  }
}

void CMSHeap::collect_mostly_concurrent(GCCause::Cause cause) {
  assert(false, "Should not reach here");
  assert(!Heap_lock->owned_by_self(), "Should not own Heap_lock");

  MutexLocker ml(Heap_lock);
  // Read the GC counts while holding the Heap_lock
  unsigned int full_gc_count_before = total_full_collections();
  unsigned int gc_count_before      = total_collections();
  {
    MutexUnlocker mu(Heap_lock);
    VM_GenCollectFullConcurrent op(gc_count_before, full_gc_count_before, cause);
    VMThread::execute(&op);
  }
}

void CMSHeap::stop() {
  // assert(false, "Should not reach here");
  ConcurrentMarkSweepThread::cmst()->stop();
}

void CMSHeap::safepoint_synchronize_begin() {
  ConcurrentMarkSweepThread::synchronize(false);
}

void CMSHeap::safepoint_synchronize_end() {
  ConcurrentMarkSweepThread::desynchronize(false);
}

void CMSHeap::wait_for_background(uint _full_gc_count_before){
  MutexLockerEx x(FullGCCount_lock, Mutex::_no_safepoint_check_flag);
  // Either a concurrent or a stop-world full gc is sufficient
  // witness to our request.
  CMSHeap* heap = this;
  while (heap->total_full_collections_completed() <= _full_gc_count_before) {
    log_info(gc)("completed count still not enough");
    FullGCCount_lock->wait(Mutex::_no_safepoint_check_flag);
    log_info(gc)("notified by background");
  }
  log_info(gc)("foreground received background finished");
}



void CMSHeap::cms_process_roots(StrongRootsScope* scope,
                                bool young_gen_as_roots,
                                ScanningOption so,
                                bool only_strong_roots,
                                OopsInGenClosure* root_closure,
                                CLDClosure* cld_closure,
                                OopStorage::ParState<false, false>* par_state_string) {
  MarkingCodeBlobClosure mark_code_closure(root_closure, !CodeBlobToOopClosure::FixRelocations);
  CLDClosure* weak_cld_closure = only_strong_roots ? NULL : cld_closure;

  process_roots(scope, so, root_closure, cld_closure, weak_cld_closure, &mark_code_closure);
  if (!only_strong_roots) {
    process_string_table_roots(scope, root_closure, par_state_string);
  }

  if (young_gen_as_roots &&
      !_process_strong_tasks->is_task_claimed(GCH_PS_younger_gens)) {
    root_closure->set_generation(young_gen());
    young_gen()->oop_iterate(root_closure);
    root_closure->reset_generation();
  }

  _process_strong_tasks->all_tasks_completed(scope->n_threads());
}

void CMSHeap::gc_prologue(bool full) {
  always_do_update_barrier = false;
  GenCollectedHeap::gc_prologue(full);
};

void CMSHeap::gc_epilogue(bool full) {
  GenCollectedHeap::gc_epilogue(full);
  always_do_update_barrier = true;
};

GrowableArray<GCMemoryManager*> CMSHeap::memory_managers() {
  GrowableArray<GCMemoryManager*> memory_managers(2);
  memory_managers.append(_young_manager);
  memory_managers.append(_old_manager);
  return memory_managers;
}

GrowableArray<MemoryPool*> CMSHeap::memory_pools() {
  GrowableArray<MemoryPool*> memory_pools(3);
  memory_pools.append(_eden_pool);
  memory_pools.append(_survivor_pool);
  memory_pools.append(_old_pool);
  return memory_pools;
}


bool CMSHeap::supports_tlab_allocation() const {
  if (UseConcMarkSweepGC && UseMSOld) {
    return _old_gen->supports_tlab_allocation();
  } else {
    return GenCollectedHeap::supports_tlab_allocation();
  }

}

size_t CMSHeap::tlab_capacity(Thread* thr) const {
  if (UseConcMarkSweepGC && UseMSOld) {
    if (_old_gen->supports_tlab_allocation()) {
      return _old_gen->tlab_capacity();
    }
    return 0;
  } else {
    return GenCollectedHeap::tlab_capacity(thr);
  }

}

size_t CMSHeap::tlab_used(Thread* thr) const {
  if (UseConcMarkSweepGC && UseMSOld) {
    if (_old_gen->supports_tlab_allocation()) {
      return _old_gen->tlab_used();
    }
    return 0;
  } else {
    return GenCollectedHeap::tlab_used(thr);
  }
}

size_t CMSHeap::unsafe_max_tlab_alloc(Thread* thr) const {
  if (UseConcMarkSweepGC && UseMSOld) {
    if (_old_gen->supports_tlab_allocation()) {
      return _old_gen->unsafe_max_tlab_alloc();
    }
    return 0;
  } else {
    return GenCollectedHeap::unsafe_max_tlab_alloc(thr);
  }

}

void CMSHeap::young_process_roots(StrongRootsScope* scope,
                                           OopsInGenClosure* root_closure,
                                           OopsInGenClosure* old_gen_closure,
                                           CLDClosure* cld_closure,
                                           OopStorage::ParState<false, false>* par_state_string,
                                  ParScanThreadState* par_scan_state) {
  MarkingCodeBlobClosure mark_code_closure(root_closure, CodeBlobToOopClosure::FixRelocations);

  process_roots(scope, SO_ScavengeCodeCache, root_closure,
                cld_closure, cld_closure, &mark_code_closure, par_scan_state);
  process_string_table_roots(scope, root_closure, par_state_string);

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_younger_gens)) {
    root_closure->reset_generation();
  }

  // When collection is parallel, all threads get to cooperate to do
  // old generation scanning.
  old_gen_closure->set_generation(_old_gen);
  rem_set()->younger_refs_iterate(_old_gen, old_gen_closure, scope->n_threads());
  old_gen_closure->reset_generation();

  _process_strong_tasks->all_tasks_completed(scope->n_threads());
}

void CMSHeap::process_roots(StrongRootsScope* scope,
                                     ScanningOption so,
                                     OopClosure* strong_roots,
                                     CLDClosure* strong_cld_closure,
                                     CLDClosure* weak_cld_closure,
                                     CodeBlobToOopClosure* code_roots,
                                     ParScanThreadState* par_scan_state) {
  // General roots.
  assert(Threads::thread_claim_parity() != 0, "must have called prologue code");
  assert(code_roots != NULL, "code root closure should always be set");
  // _n_termination for _process_strong_tasks should be set up stream
  // in a method not running in a GC worker.  Otherwise the GC worker
  // could be trying to change the termination condition while the task
  // is executing in another GC worker.

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_ClassLoaderDataGraph_oops_do)) {
    ClassLoaderDataGraph::roots_cld_do(strong_cld_closure, weak_cld_closure);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }

  // Only process code roots from thread stacks if we aren't visiting the entire CodeCache anyway
  CodeBlobToOopClosure* roots_from_code_p = (so & SO_AllCodeCache) ? NULL : code_roots;

  bool is_par = scope->n_threads() > 1;
  Threads::possibly_parallel_oops_do(is_par, strong_roots, roots_from_code_p);

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_Universe_oops_do)) {
    Universe::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }
  // Global (strong) JNI handles
  if (!_process_strong_tasks->is_task_claimed(GCH_PS_JNIHandles_oops_do)) {
    JNIHandles::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_ObjectSynchronizer_oops_do)) {
    ObjectSynchronizer::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }
  if (!_process_strong_tasks->is_task_claimed(GCH_PS_Management_oops_do)) {
    Management::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }
  if (!_process_strong_tasks->is_task_claimed(GCH_PS_jvmti_oops_do)) {
    JvmtiExport::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }
  if (UseAOT && !_process_strong_tasks->is_task_claimed(GCH_PS_aot_oops_do)) {
    AOTLoader::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_SystemDictionary_oops_do)) {
    SystemDictionary::oops_do(strong_roots);
    par_scan_state->trim_queues(GCDrainStackTargetSize);
  }

  if (!_process_strong_tasks->is_task_claimed(GCH_PS_CodeCache_oops_do)) {
    if (so & SO_ScavengeCodeCache) {
      assert(code_roots != NULL, "must supply closure for code cache");

      // We only visit parts of the CodeCache when scavenging.
      CodeCache::scavenge_root_nmethods_do(code_roots);
    }
    if (so & SO_AllCodeCache) {
      assert(code_roots != NULL, "must supply closure for code cache");

      // CMSCollector uses this to do intermediate-strength collections.
      // We scan the entire code cache, since CodeCache::do_unloading is not called.
      CodeCache::blobs_do(code_roots);
    }
    par_scan_state->trim_queues(GCDrainStackTargetSize);
    // Verify that the code cache contents are not subject to
    // movement by a scavenging collection.
    DEBUG_ONLY(CodeBlobToOopClosure assert_code_is_non_scavengable(&assert_is_non_scavengable_closure, !CodeBlobToOopClosure::FixRelocations));
    DEBUG_ONLY(CodeCache::asserted_non_scavengable_nmethods_do(&assert_code_is_non_scavengable));
  }
}
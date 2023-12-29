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
#include "gc/serial/defNewGeneration.inline.hpp"

template <typename OopClosureType1, typename OopClosureType2>
void CMSHeap::oop_since_save_marks_iterate(OopClosureType1* cur,
                                           OopClosureType2* older) {
  young_gen()->oop_since_save_marks_iterate(cur);
  old_gen()->oop_since_save_marks_iterate(older);
}

void CMSHeap::do_collection(bool           full,
                                     bool           clear_all_soft_refs,
                                     size_t         size,
                                     bool           is_tlab,
                                     GenerationType max_generation) {
  if (GCLocker::check_active_before_gc()) {
    return; // GC is disabled (e.g. JNI GetXXXCritical operation)
  }

  assert(Thread::current()->is_VM_thread(), "Should be VM thread");
  // assert(GCLockerInvokesConcurrent || ExplicitGCInvokesConcurrent, "Unexpected");

//  if (_gc_count_before == heap->total_collections()) {
//    // The "full" of do_full_collection call below "forces"
//    // a collection; the second arg, 0, below ensures that
//    // only the young gen is collected. XXX In the future,
//    // we'll probably need to have something in this interface
//    // to say do this only if we are sure we will not bail
//    // out to a full collection in this attempt, but that's
//    // for the future.
//    assert(SafepointSynchronize::is_at_safepoint(),
//           "We can only be executing this arm of if at a safepoint");
//    GCCauseSetter gccs(heap, _gc_cause);
//    heap->do_full_collection(heap->must_clear_all_soft_refs(), GenCollectedHeap::YoungGen);
//  } // Else no need for a foreground young gc

//  assert((_gc_count_before < heap->total_collections()) ||
//         (GCLocker::is_active() /* gc may have been skipped */
//          && (_gc_count_before == heap->total_collections())),
//         "total_collections() should be monotonically increasing");
  uint _full_gc_count_before = 0;
  {
    MutexLockerEx x(FullGCCount_lock, Mutex::_no_safepoint_check_flag);
    _full_gc_count_before = total_full_collections();
  }

  CMSCollector::request_full_gc( _full_gc_count_before, gc_cause());

  wait_for_background(_full_gc_count_before);

}

#endif // SHARE_GC_CMS_CMSHEAP_INLINE_HPP

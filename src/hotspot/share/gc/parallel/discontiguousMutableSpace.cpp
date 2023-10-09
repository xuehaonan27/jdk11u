//
// Created by 华子曰 on 2023/8/28.
//

// This version requires locking. */

#include "gc/parallel/freeList.hpp"
#include "gc/parallel/freeList.inline.hpp"
#include "gc/parallel/discontiguousMutableSpace.hpp"

size_t MinChunkSize = 0;
size_t _min_chunk_size_in_bytes = 0;

HeapWord* DiscontiguousMutableSpace::allocate(size_t size) {
    assert(Heap_lock->owned_by_self() ||
           (SafepointSynchronize::is_at_safepoint() &&
            Thread::current()->is_VM_thread()),
           "not locked");

    size_t aligned_size = adjustObjectSize(size);
    FreeChunk* fc = freeList.get_first_match(aligned_size);
    size_t remainder_size = fc->size() - aligned_size;
    if(remainder_size > MinChunkSize){
        //Should split the chunk
        freeList.remove_chunk(fc);
        FreeChunk* remainder = (FreeChunk*)(((HeapWord*)fc) + aligned_size);
        remainder->set_mark((markOop)remainder_size);
        freeList.insert_chunk(remainder);
        fc->markNotFree();
        HeapWord* obj = (HeapWord*) fc;
        assert(is_object_aligned(obj), "checking alignment");
        return obj;

    }
    else{
        //Use the whole chunk
        fc->markNotFree();
        HeapWord* obj = (HeapWord*) fc;
        assert(is_object_aligned(obj), "checking alignment");
        return obj;
    }
    return NULL;
    HeapWord* obj = top();
    if (pointer_delta(end(), obj) >= size) {
        HeapWord* new_top = obj + size;
        set_top(new_top);
        assert(is_object_aligned(obj) && is_object_aligned(new_top),
               "checking alignment");
        return obj;
    } else {
        return NULL;
    }
}

void DiscontiguousMutableSpace::initialize(MemRegion mr,
                        bool clear_space,
                        bool mangle_space,
                        bool setup_pages = SetupPages){
    MutableSpace::initialize(mr, clear_space, mangle_space, setup_pages);
    freeList.initialize();
}

void DiscontiguousMutableSpace::oop_iterate(OopIterateClosure* cl) {
    HeapWord* obj_addr = bottom();
    HeapWord* t = top();
    // Could call objects iterate, but this is easier.
    while (obj_addr < t) {
        obj_addr += oop(obj_addr)->oop_iterate_size(cl);
    }
}

// Apply the given closure to each oop in the space.
void DiscontiguousMutableSpace::oop_iterate(OopIterateClosure* cl) {
    assert_lock_strong(freelistLock());
    HeapWord *cur, *limit;
    size_t curSize;
    for (cur = bottom(), limit = end(); cur < limit;
         cur += curSize) {
        curSize = block_size(cur);
        if (block_is_obj(cur)) {
            oop(cur)->oop_iterate(cl);
        }
    }
}

void DiscontiguousMutableSpace::object_iterate(ObjectClosure* cl) {
    HeapWord* p = bottom();
    while (p < top()) {
        cl->do_object(oop(p));
        p += oop(p)->size();
    }
}

void DiscontiguousMutableSpace::set_ps_values() {
    // Set CMS global values
    assert(MinChunkSize == 0, "already set");

    // MinChunkSize should be a multiple of MinObjAlignment and be large enough
    // for chunks to contain a FreeChunk.
    _min_chunk_size_in_bytes = align_up(sizeof(FreeChunk), MinObjAlignmentInBytes);
    MinChunkSize = _min_chunk_size_in_bytes / BytesPerWord;

    assert(IndexSetStart == 0 && IndexSetStride == 0, "already set");
    IndexSetStart  = MinChunkSize;
    IndexSetStride = MinObjAlignment;
}

size_t DiscontiguousMutableSpace::block_size(const HeapWord* p) const {//hua
    NOT_PRODUCT(verify_objects_initialized());
    // This must be volatile, or else there is a danger that the compiler
    // will compile the code below into a sometimes-infinite loop, by keeping
    // the value read the first time in a register.
    while (true) {
        // We must do this until we get a consistent view of the object.
        if (FreeChunk::indicatesFreeChunk(p)) {
            volatile FreeChunk* fc = (volatile FreeChunk*)p;
            size_t res = fc->size();

            // Bugfix for systems with weak memory model (PPC64/IA64). The
            // block's free bit was set and we have read the size of the
            // block. Acquire and check the free bit again. If the block is
            // still free, the read size is correct.
            OrderAccess::acquire();

            // If the object is still a free chunk, return the size, else it
            // has been allocated so try again.
            if (FreeChunk::indicatesFreeChunk(p)) {
                assert(res != 0, "Block size should not be 0");
                return res;
            }
        } else {
            // The barrier is required to prevent reordering of the free chunk check
            // and the klass read.
            OrderAccess::loadload();

            // Ensure klass read before size.
            Klass* k = oop(p)->klass_or_null_acquire();
            if (k != NULL) {
                assert(k->is_klass(), "Should really be klass oop.");
                oop o = (oop)p;
                assert(oopDesc::is_oop(o, true /* ignore mark word */), "Should be an oop.");

                size_t res = o->size_given_klass(k);
                res = adjustObjectSize(res);
                assert(res != 0, "Block size should not be 0");
                return res;
            }
        }
    }
}

void DiscontiguousMutableSpace::set_chunk_values() {
    // Set CMS global values
    assert(MinChunkSize == 0, "already set");

    // MinChunkSize should be a multiple of MinObjAlignment and be large enough
    // for chunks to contain a FreeChunk.
    _min_chunk_size_in_bytes = align_up(sizeof(FreeChunk), MinObjAlignmentInBytes);
    MinChunkSize = _min_chunk_size_in_bytes / BytesPerWord;
}
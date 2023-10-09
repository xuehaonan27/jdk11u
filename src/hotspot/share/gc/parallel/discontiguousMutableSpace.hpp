//
// Created by 华子曰 on 2023/8/28.
//

#ifndef SHARE_VM_GC_PARALLEL_DISCONTIGUOUSMUTABLESPACE
#define SHARE_VM_GC_PARALLEL_DISCONTIGUOUSMUTABLESPACE

#include "gc/parallel/mutableSpace.hpp"
#include "gc/parallel/chunkFreeList.hpp"
#include "gc/parallel/freeChunk.hpp"


class DiscontiguousMutableSpace : public MutableSpace {
    friend class VMStructs;

private:
    void set_ps_values();

public:
    virtual HeapWord* allocate(size_t word_size);

    // Iteration.
    virtual void oop_iterate(OopIterateClosure* cl);
    virtual void object_iterate(ObjectClosure* cl);

    // Adjust the chunk for the minimum size.  This version is called in
    // most cases in CompactibleFreeListSpace methods.
    inline static size_t adjustObjectSize(size_t size) {
        return align_object_size(MAX2(size, (size_t)MinChunkSize));
    }
};

#endif //SHARE_VM_GC_PARALLEL_DISCONTIGUOUSMUTABLESPACE


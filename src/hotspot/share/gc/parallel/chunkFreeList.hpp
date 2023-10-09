//
// Created by 华子曰 on 2023/8/28.
//

#ifndef SHARE_VM_GC_PARALLEL_CHUNKFREELIST
#define SHARE_VM_GC_PARALLEL_CHUNKFREELIST

#include "memory/freeList.hpp"
#include "utilities/align.hpp"

template <class Chunk>
class ChunkFreeList : public FreeList<>{
    friend class VMStructs;

public:
    ChunkFreeList();

    // Reset the head, tail, hint, and count of a free list.
    void reset(size_t hint);
    HeapWord* allocate(size_t size);
};

#endif //SHARE_VM_GC_PARALLEL_CHUNKFREELIST


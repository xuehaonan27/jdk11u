//
// Created by 华子曰 on 2023/8/28.
//

#include "share/gc/parallel/chunkFreeList.hpp"

template <class Chunk>
ChunkFreeList<Chunk>::ChunkFreeList() : FreeList<Chunk>(), _hint(0) {
    init_statistics();
}

template <class Chunk>
void ChunkFreeList<Chunk>::initialize() {
    FreeList<Chunk>::initialize();
    set_hint(0);
    init_statistics(true /* split_birth */);
}

template <class Chunk>
void ChunkFreeList<Chunk>::reset(size_t hint) {
    FreeList<Chunk>::reset();
    set_hint(hint);
}
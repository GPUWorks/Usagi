﻿#pragma once

#include <mutex>

#include "Bitmap.hpp"

namespace yuki::memory
{
/**
 * \brief A thread-safe bitmap allocator for managing remote memory.
 */
class BitmapAllocator
{
    char *const mBase = nullptr;
    const size_t mTotalSize, mBlockSize, mMaxAlignment;
    Bitmap mAllocation;
    std::mutex mBitmapLock;

public:
    /**
     * \brief 
     * \param base The starting address of the memory region. It must be
     * aligned to max_alignment, and can be nullptr to allocate based on
     * offsets.
     * \param total_size Total usable size. Must be greater than block_size.
     * If it is not a multiple of block_size, the region at the end not
     * big enough to hold a block will not be used.
     * \param block_size A positive size of minimum allocation unit. Must
     * be multiple of max_alignment if which is non-zero.
     * \param max_alignment The maximum alignment size allowed. Must be power
     * of two. It can be zero if no alignment is needed.
     */
    BitmapAllocator(void *base, size_t total_size, size_t block_size,
        size_t max_alignment);

    size_t managedSize() const { return mTotalSize; }
    size_t usableSize() const { return mBlockSize * mAllocation.blockCount(); }

    void * allocate(size_t num_bytes, size_t alignment = 0);
    void deallocate(void *pointer);
};
}

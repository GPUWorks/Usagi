#include <cassert>
#include <algorithm>

#include <Usagi/Engine/Utility/CyclicContainerAdaptor.hpp>
#include <Usagi/Engine/Utility/Iterator.h>

#include "Bitmap.hpp"

yuki::memory::Bitmap::Bitmap(size_t num_blocks)
{
    reset(num_blocks);
}

void yuki::memory::Bitmap::reset(size_t num_blocks)
{
    mBitmap = { num_blocks, Block::FREE };
}

std::vector<yuki::memory::Bitmap::Block>::iterator yuki::memory::Bitmap::
determineScanningBegin(const size_t start_block)
{
    auto start_iter = mBitmap.begin() + start_block;
    // forbit allocating from the middle of a free region
    if(start_block != 0 && *(start_iter - 1) == Block::FREE)
        start_iter = mBitmap.begin();
    return start_iter;
}

size_t yuki::memory::Bitmap::allocate(size_t num_blocks, size_t start_block)
{
    assert(start_block < mBitmap.size());

    if(num_blocks == 0)
        throw std::invalid_argument("allocation size must be positive");

    const auto scan_begin = determineScanningBegin(start_block);
    auto alloc_begin = utility::findFirstConsecutive(scan_begin, mBitmap.end(),
        Block::FREE, num_blocks);
    // space not found, wrap to begin
    if(alloc_begin == mBitmap.end())
    {
        alloc_begin = utility::findFirstConsecutive(mBitmap.begin(), scan_begin,
            Block::FREE, num_blocks);
        if(alloc_begin == scan_begin)
            throw std::bad_alloc();
    }

    const auto alloc_begin_index = alloc_begin - mBitmap.begin();
    *alloc_begin = Block::USED_BEGIN;
    std::fill(alloc_begin + 1, alloc_begin + num_blocks, Block::USED);

    return alloc_begin_index;
}

void yuki::memory::Bitmap::deallocate(const size_t start_block)
{
    assert(start_block < mBitmap.size());

    auto iter = mBitmap.begin() + start_block;
    if(*iter != Block::USED_BEGIN)
        throw std::invalid_argument(
            "argument not representing an allocation beginning");

    while(true)
    {
        *iter = Block::FREE;
        ++iter;
        if(iter == mBitmap.end() || *iter != Block::USED) break;
    }
}

size_t yuki::memory::Bitmap::blockCount() const
{
    return mBitmap.size();
}

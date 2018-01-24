﻿#pragma once

#include <Usagi/Engine/Utility/Noncopyable.hpp>

namespace yuki::graphics
{

/**
 * \brief The buffer holding resources referenced by the GPU.
 * Allocation of buffers is delegated to FrameContrller, which swap a frame
 * after command lists referencing the current frame resources are
 * submitted. Resources for the next frame are allocated from the frame
 * just swapped in.
 */
class Buffer : Noncopyable
{
public:
    Buffer() = default;
    virtual ~Buffer() = default;

    // orphan the old memory and allocate a new chuck of the same size
    virtual void reallocate() = 0;
    // get persistently mapped address, which may vary after reallocations.
    virtual void * getMappedAddress() = 0;
    // sync the content to device side.
    virtual void flush() = 0;
};

}
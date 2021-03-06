﻿#pragma once

#include <Usagi/Utility/Noncopyable.hpp>

namespace usagi
{
/**
 * \brief Describes the usages of attachments of a render job.
 */
class RenderPass : Noncopyable
{
public:
    virtual ~RenderPass() = default;
};
}

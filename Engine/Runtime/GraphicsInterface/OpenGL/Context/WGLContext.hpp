﻿#pragma once

// todo: remove windows header
#include <windows.h>

#include "OpenGLContext.hpp"

namespace yuki
{

class WGLContext : public OpenGLContext
{
    struct HGLRCWrapper
    {
        HDC dc = nullptr;
        HGLRC glrc = nullptr;

        void clear();
        void destroy();
        void setCurrent();

        HGLRCWrapper();
        HGLRCWrapper(HDC dc, HGLRC handle);
        HGLRCWrapper(const HGLRCWrapper &other) = delete;
        HGLRCWrapper(HGLRCWrapper &&other) noexcept;
        ~HGLRCWrapper();

        HGLRCWrapper & operator=(const HGLRCWrapper &other) = delete;
        HGLRCWrapper & operator=(HGLRCWrapper &&other) noexcept;
    } mContext;

public:
    WGLContext(HDC dc);

    void setCurrent() override;
    void swapBuffer() override;
};

}

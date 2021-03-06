﻿#pragma once

#include <memory>

#include <Usagi/Core/Math.hpp>

namespace usagi
{
enum class GpuBufferFormat;
class Runtime;
class Swapchain;
class Window;

class RenderWindow
{
public:
    std::shared_ptr<Window> window;
    std::shared_ptr<Swapchain> swapchain;

    void create(Runtime *runtime,
        const std::string &window_title,
        const Vector2i &window_position,
        const Vector2u32 &window_size,
        GpuBufferFormat swapchain_format);
};
}

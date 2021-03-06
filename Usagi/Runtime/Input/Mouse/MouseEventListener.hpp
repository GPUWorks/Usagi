﻿#pragma once

#include <Usagi/Core/Math.hpp>

#include "MouseButtonCode.hpp"

namespace usagi
{
struct MouseEvent
{
    class Mouse *mouse = nullptr;
    class Window *window = nullptr;
    Vector2f position;
};

struct MousePositionEvent : MouseEvent
{
    Vector2f distance;
};

struct MouseButtonEvent : MouseEvent
{
    MouseButtonCode button;
    bool pressed;
};

struct MouseWheelEvent : MouseEvent
{
    Vector2f distance;
};

class MouseEventListener
{
public:
    virtual ~MouseEventListener() = default;

    virtual void onMouseMove(const MousePositionEvent &e)
    {
    }

    virtual void onMouseButtonStateChange(const MouseButtonEvent &e)
    {
    }

    virtual void onMouseWheelScroll(const MouseWheelEvent &e)
    {
    }
};
}

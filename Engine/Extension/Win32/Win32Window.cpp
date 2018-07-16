﻿#include <Usagi/Engine/Utility/String.hpp>
#include <Usagi/Engine/Runtime/HID/Mouse/MouseEventListener.hpp>
#include <Usagi/Engine/Runtime/HID/Keyboard/KeyCode.hpp>
#include <Usagi/Engine/Runtime/HID/Keyboard/KeyEventListener.hpp>

// include dirty windows headers at last
#include "Win32Window.hpp"
#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")

const wchar_t usagi::Win32Window::WINDOW_CLASS_NAME[] =
    L"UsagiWin32WindowWrapper";
HINSTANCE usagi::Win32Window::mProcessInstanceHandle = nullptr;

void usagi::Win32Window::ensureWindowSubsystemInitialized()
{
    if(mProcessInstanceHandle)
        return;

    // get the process handle, all windows created using this class get
    // dispatched to the same place.
    mProcessInstanceHandle = GetModuleHandle(nullptr);

    WNDCLASSEX wcex { };

    wcex.cbSize = sizeof(WNDCLASSEX);
    // CS_OWNDC is required to create OpenGL context
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = &windowMessageDispatcher;
    wcex.hInstance = mProcessInstanceHandle;
    // hInstance must be null to use predefined cursors
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // we print the background on our own
    wcex.hbrBackground = nullptr;
    wcex.lpszClassName = WINDOW_CLASS_NAME;

    if(!RegisterClassEx(&wcex))
    {
        throw std::runtime_error("RegisterClassEx() failed!");
    }

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

RECT usagi::Win32Window::getClientScreenRect() const
{
    // Get the window client area.
    RECT rc;
    GetClientRect(mWindowHandle, &rc);

    POINT pt = { rc.left, rc.top };
    POINT pt2 = { rc.right, rc.bottom };
    ClientToScreen(mWindowHandle, &pt);
    ClientToScreen(mWindowHandle, &pt2);

    rc.left = pt.x;
    rc.top = pt.y;
    rc.right = pt2.x;
    rc.bottom = pt2.y;

    return rc;
}

void usagi::Win32Window::createWindowHandle(
    const std::string &title, int width, int height)
{
    auto windowTitleWide = string::toWideVector(title);
    // todo update after resizing
    mWindowSize = { width, height };

    static constexpr DWORD window_style = WS_OVERLAPPEDWINDOW;
    static constexpr DWORD window_style_ex = WS_EX_ACCEPTFILES;

    RECT window_rect { };
    window_rect.bottom = height;
    window_rect.right = width;
    AdjustWindowRectEx(&window_rect, window_style, FALSE, window_style_ex);

    mWindowHandle = CreateWindowEx(
        window_style_ex,
        WINDOW_CLASS_NAME,
        &windowTitleWide[0],
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_rect.right - window_rect.left, // width
        window_rect.bottom - window_rect.top, // height
        nullptr,
        nullptr,
        mProcessInstanceHandle,
        nullptr
    );

    if(!mWindowHandle)
    {
        throw std::runtime_error("mWindowHandle() failed");
    }

    // associate the class instance with the window so they can be identified 
    // in WindowProc
    SetWindowLongPtr(mWindowHandle, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(this));
}

void usagi::Win32Window::_registerRawInputDevices() const
{
    RAWINPUTDEVICE Rid[2];

    // adds HID mouse, RIDEV_NOLEGACY is not used because we need the system
    // to process non-client area.
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    Rid[0].dwFlags = 0;
    Rid[0].hwndTarget = 0;

    // adds HID keyboard, RIDEV_NOLEGACY is not used to allow the system
    // process hotkeys like print screen. note that alt+f4 is not handled
    // if related key messages not passed to DefWindowProc(). generally,
    // RIDEV_NOLEGACY should only be used when having a single fullscreen
    // window.
    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x06;
    // interestingly, RIDEV_NOHOTKEYS will prevent the explorer from using
    // the fancy window-choosing popup, and we still receive key events when
    // switching window, so it is not used here.
    Rid[1].dwFlags = 0;
    Rid[1].hwndTarget = 0;

    // note that this registration affects the entire application
    if(RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
    {
        //registration failed. Call GetLastError for the cause of the error
        throw std::runtime_error("RegisterRawInputDevices() failed");
    }
}

usagi::Win32Window::Win32Window(
    const std::string &title, const Vector2u32 &size)
{
    ensureWindowSubsystemInitialized();
    createWindowHandle(title, size.x(), size.y());
    _registerRawInputDevices();
    Win32Window::show(true);
    Win32Window::centerCursor();
}

HDC usagi::Win32Window::getDeviceContext() const
{
    return GetDC(mWindowHandle);
}

void usagi::Win32Window::show(bool show)
{
    ShowWindow(mWindowHandle, show ? SW_SHOWNORMAL : SW_HIDE);
}

bool usagi::Win32Window::isFocused() const
{
    return mWindowActive;
}

void usagi::Win32Window::processEvents()
{
    MSG msg;
    // hwnd should be nullptr or the loop won't end when close the window
    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

usagi::Vector2f usagi::Win32Window::size() const
{
    return mWindowSize.cast<float>();
}

bool usagi::Win32Window::isOpen() const
{
    return !mClosed;
}

void usagi::Win32Window::setTitle(const std::string &title)
{
    std::wstring wtitle { title.begin(), title.end() };
    SetWindowText(mWindowHandle, wtitle.c_str());
}

LRESULT usagi::Win32Window::windowMessageDispatcher(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    auto window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hWnd,
        GWLP_USERDATA));
    // during window creation some misc messages are sent, we just pass them
    // to the system procedure.
    if(!window)
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return window->handleWindowMessage(hWnd, message, wParam, lParam);
}

void usagi::Win32Window::sendButtonEvent(MouseButtonCode button,
    bool pressed)
{
    auto &prev_pressed = mMouseButtonDown[static_cast<std::size_t>(button)];

    // if we receive an release event without a prior press event, it means that
    // the user activated our window by clicking. this is only meaningful if we
    // are in normal mode when using GUI. if immersive mode is enabled, such
    // event may cause undesired behavior, such as firing the weapon.
    if(!pressed && !prev_pressed && isImmersiveMode())
        return;

    prev_pressed = pressed;

    MouseButtonEvent e;
    e.mouse = this;
    e.button = button;
    e.pressed = pressed;
    for(auto &&h : mMouseEventListeners)
    {
        h->onMouseButtonStateChange(e);
    }
}

usagi::KeyCode usagi::Win32Window::translate(
    const RAWKEYBOARD *keyboard)
{
    switch(keyboard->VKey)
    {
        case VK_BACK: return KeyCode::BACKSPACE;
        case VK_TAB: return KeyCode::TAB;
        case VK_PAUSE: return KeyCode::PAUSE;
        case VK_CAPITAL: return KeyCode::CAPSLOCK;
        case VK_ESCAPE: return KeyCode::ESCAPE;
        case VK_SPACE: return KeyCode::SPACE;
        case VK_PRIOR: return KeyCode::PAGE_UP;
        case VK_NEXT: return KeyCode::PAGE_DOWN;
        case VK_END: return KeyCode::END;
        case VK_HOME: return KeyCode::HOME;
        case VK_LEFT: return KeyCode::LEFT;
        case VK_UP: return KeyCode::UP;
        case VK_RIGHT: return KeyCode::RIGHT;
        case VK_DOWN: return KeyCode::DOWN;
        case VK_SNAPSHOT: return KeyCode::PRINT_SCREEN;
        case VK_INSERT: return KeyCode::INSERT;
        case VK_DELETE: return KeyCode::DELETE;
        case '0': return KeyCode::DIGIT_0;
        case '1': return KeyCode::DIGIT_1;
        case '2': return KeyCode::DIGIT_2;
        case '3': return KeyCode::DIGIT_3;
        case '4': return KeyCode::DIGIT_4;
        case '5': return KeyCode::DIGIT_5;
        case '6': return KeyCode::DIGIT_6;
        case '7': return KeyCode::DIGIT_7;
        case '8': return KeyCode::DIGIT_8;
        case '9': return KeyCode::DIGIT_9;
        case 'A': return KeyCode::A;
        case 'B': return KeyCode::B;
        case 'C': return KeyCode::C;
        case 'D': return KeyCode::D;
        case 'E': return KeyCode::E;
        case 'F': return KeyCode::F;
        case 'G': return KeyCode::G;
        case 'H': return KeyCode::H;
        case 'I': return KeyCode::I;
        case 'J': return KeyCode::J;
        case 'K': return KeyCode::K;
        case 'L': return KeyCode::L;
        case 'M': return KeyCode::M;
        case 'N': return KeyCode::N;
        case 'O': return KeyCode::O;
        case 'P': return KeyCode::P;
        case 'Q': return KeyCode::Q;
        case 'R': return KeyCode::R;
        case 'S': return KeyCode::S;
        case 'T': return KeyCode::T;
        case 'U': return KeyCode::U;
        case 'V': return KeyCode::V;
        case 'W': return KeyCode::W;
        case 'X': return KeyCode::X;
        case 'Y': return KeyCode::Y;
        case 'Z': return KeyCode::Z;
        case VK_LWIN: return KeyCode::LEFT_OS;
        case VK_RWIN: return KeyCode::RIGHT_OS;
        case VK_APPS: return KeyCode::CONTEXT_MENU;
        case VK_NUMPAD0: return KeyCode::NUM_0;
        case VK_NUMPAD1: return KeyCode::NUM_1;
        case VK_NUMPAD2: return KeyCode::NUM_2;
        case VK_NUMPAD3: return KeyCode::NUM_3;
        case VK_NUMPAD4: return KeyCode::NUM_4;
        case VK_NUMPAD5: return KeyCode::NUM_5;
        case VK_NUMPAD6: return KeyCode::NUM_6;
        case VK_NUMPAD7: return KeyCode::NUM_7;
        case VK_NUMPAD8: return KeyCode::NUM_8;
        case VK_NUMPAD9: return KeyCode::NUM_9;
        case VK_MULTIPLY: return KeyCode::NUM_MULTIPLY;
        case VK_ADD: return KeyCode::NUM_ADD;
        case VK_SUBTRACT: return KeyCode::NUM_SUBTRACT;
        case VK_DECIMAL: return KeyCode::NUM_DECIMAL;
        case VK_DIVIDE: return KeyCode::NUM_DIVIDE;
        case VK_F1: return KeyCode::F1;
        case VK_F2: return KeyCode::F2;
        case VK_F3: return KeyCode::F3;
        case VK_F4: return KeyCode::F4;
        case VK_F5: return KeyCode::F5;
        case VK_F6: return KeyCode::F6;
        case VK_F7: return KeyCode::F7;
        case VK_F8: return KeyCode::F8;
        case VK_F9: return KeyCode::F9;
        case VK_F10: return KeyCode::F10;
        case VK_F11: return KeyCode::F11;
        case VK_F12: return KeyCode::F12;
        case VK_NUMLOCK: return KeyCode::NUM_LOCK;
        case VK_SCROLL: return KeyCode::SCROLL_LOCK;
        case VK_OEM_1: return KeyCode::SEMICOLON;
        case VK_OEM_PLUS: return KeyCode::EQUAL;
        case VK_OEM_COMMA: return KeyCode::COMMA;
        case VK_OEM_MINUS: return KeyCode::MINUS;
        case VK_OEM_PERIOD: return KeyCode::PERIOD;
        case VK_OEM_2: return KeyCode::SLASH;
        case VK_OEM_3: return KeyCode::BACKQUOTE;
        case VK_OEM_4: return KeyCode::LEFT_BRACKET;
        case VK_OEM_5: return KeyCode::BACKSLASH;
        case VK_OEM_6: return KeyCode::RIGHT_BRACKET;
        case VK_OEM_7: return KeyCode::QUOTE;
        default: break;
    }
    const int e0_prefixed = (keyboard->Flags & RI_KEY_E0) != 0;
    switch(keyboard->VKey)
    {
        case VK_SHIFT:
        {
            return MapVirtualKey(keyboard->MakeCode, MAPVK_VSC_TO_VK_EX) ==
                VK_LSHIFT
                ? KeyCode::LEFT_SHIFT
                : KeyCode::RIGHT_SHIFT;
        }
        case VK_CONTROL: return e0_prefixed
                ? KeyCode::RIGHT_CONTROL
                : KeyCode::LEFT_CONTROL;
        case VK_MENU: return e0_prefixed
                ? KeyCode::RIGHT_ALT
                : KeyCode::LEFT_ALT;
        case VK_RETURN: return
                e0_prefixed ? KeyCode::NUM_ENTER : KeyCode::ENTER;
        default: return KeyCode::UNKNOWN;
    }
}

void usagi::Win32Window::sendKeyEvent(KeyCode key, bool pressed,
    bool repeated)
{
    KeyEvent e;
    e.keyboard = this;
    e.key_code = key;
    e.pressed = pressed;
    e.repeated = repeated;
    for(auto &&h : mKeyEventListeners)
    {
        h->onKeyStateChange(e);
    }
}

void usagi::Win32Window::confineCursorInClientArea() const
{
    if(!mWindowActive) return;

    RECT client_rect = getClientScreenRect();
    ClipCursor(&client_rect);
}

void usagi::Win32Window::processMouseInput(const RAWINPUT *raw)
{
    auto &mouse = raw->data.mouse;

    // only receive relative movement. note that the mouse driver typically
    // won't generate mouse input data based on absolute data.
    // see https://stackoverflow.com/questions/14113303/raw-input-device-rawmouse-usage
    if(mouse.usFlags != MOUSE_MOVE_RELATIVE) return;

    // when in GUI mode, only receive events inside the window rect
    // todo since we use raw input, we receive the mouse messages even if the
    // part of window is covered, in which case the user might perform
    // undesired actions.
    if(!isImmersiveMode())
    {
        auto cursor = getCursorPositionInWindow();
        if(cursor.x() < 0 || cursor.y() < 0 || cursor.x() >= mWindowSize.x() ||
            cursor.y() >= mWindowSize.y())
            return;
    }

    // process mouse movement
    if(mouse.lLastX || mouse.lLastY)
    {
        MousePositionEvent e;
        e.mouse = this;
        e.cursor_position_delta = { mouse.lLastX, mouse.lLastY };
        for(auto &&h : mMouseEventListeners)
        {
            h->onMouseMove(e);
        }
    }
    // proces mouse buttons & scrolling
    if(mouse.usButtonFlags)
    {
        // process mouse buttons
        // note that it is impossible to activate another window while holding
        // a mouse button pressed within the active window, so it is
        // unnecessary to clear button press states when deactive the window.
        // however this does not hold for the keyboard.
#define MOUSE_BTN_EVENT(button) \
    if(mouse.usButtonFlags & RI_MOUSE_##button##_BUTTON_DOWN) \
        sendButtonEvent(MouseButtonCode::button, true); \
    else if(mouse.usButtonFlags & RI_MOUSE_##button##_BUTTON_UP) \
        sendButtonEvent(MouseButtonCode::button, false) \
/**/
        MOUSE_BTN_EVENT(LEFT);
        MOUSE_BTN_EVENT(MIDDLE);
        MOUSE_BTN_EVENT(RIGHT);
        // ignore other buttons
#undef MOUSE_BTN_EVENT

        // process scrolling
        if(mouse.usButtonFlags & RI_MOUSE_WHEEL)
        {
            MouseWheelEvent e;
            e.mouse = this;
            e.distance = static_cast<short>(mouse.usButtonData);
            for(auto &&h : mMouseEventListeners)
            {
                h->onMouseWheelScroll(e);
            }
        }
    }
}

std::unique_ptr<BYTE[]> usagi::Win32Window::getRawInputBuffer(
    LPARAM lParam) const
{
    UINT dwSize;

    // fetch raw input data
    GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        nullptr,
        &dwSize,
        sizeof(RAWINPUTHEADER)
    );
    std::unique_ptr<BYTE[]> lpb(new BYTE[dwSize]);
    if(GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        lpb.get(),
        &dwSize,
        sizeof(RAWINPUTHEADER)
    ) != dwSize)
        throw std::runtime_error(
            "GetRawInputData does not return correct size!");

    return std::move(lpb);
}

void usagi::Win32Window::recaptureCursor()
{
    if(mMouseCursorCaptured) captureCursor();
}

void usagi::Win32Window::processKeyboardInput(RAWINPUT *raw)
{
    auto &kb = raw->data.keyboard;

    auto key = translate(&kb);
    // ignore keys other than those on 101 keyboard
    if(key == KeyCode::UNKNOWN) return;

    const bool pressed = (kb.Flags & RI_KEY_BREAK) == 0;
    bool repeated = false;

    if(pressed)
    {
        if(mKeyPressed.count(key) != 0)
            repeated = true;
        else
            mKeyPressed.insert(key);
    }
    else
    {
        mKeyPressed.erase(key);
    }

    sendKeyEvent(key, pressed, repeated);
}

void usagi::Win32Window::clearKeyPressedStates()
{
    for(auto iter = mKeyPressed.begin(); iter != mKeyPressed.end();)
    {
        sendKeyEvent(*iter, false, false);
        iter = mKeyPressed.erase(iter);
    }
}

LRESULT usagi::Win32Window::handleWindowMessage(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        // unbuffered raw input data
        case WM_INPUT:
        {
            std::unique_ptr<BYTE[]> lpb = getRawInputBuffer(lParam);
            RAWINPUT *raw = reinterpret_cast<RAWINPUT*>(lpb.get());

            switch(raw->header.dwType)
            {
                case RIM_TYPEKEYBOARD:
                {
                    processKeyboardInput(raw);
                    break;
                }
                case RIM_TYPEMOUSE:
                {
                    processMouseInput(raw);
                    break;
                }
                default: break;
            }
            break;
        }
        // window management
        case WM_ACTIVATEAPP:
        {
            // window being activated
            if((mWindowActive = (wParam == TRUE)))
            {
                recaptureCursor();
            }
                // window being deactivated
            else
            {
                clearKeyPressedStates();
                break;
            }
            break;
        }
        // todo: sent resize/move events
        case WM_SIZE:
        case WM_MOVE:
        {
            recaptureCursor();
            break;
        }
        // ignore any key events (RawInput is used instead)
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        // ignore mouse events (RawInput is used instead)
        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        {
            break;
        }
        case WM_CHAR:
        {
            break;
        }
        case WM_CLOSE:
        {
            DestroyWindow(mWindowHandle);
            break;
        }
        case WM_DESTROY:
        {
            mClosed = true;
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}

bool usagi::Win32Window::isKeyPressed(KeyCode key)
{
    return mKeyPressed.count(key) != 0;
}

usagi::Vector2f usagi::Win32Window::getCursorPositionInWindow()
{
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(mWindowHandle, &pt);
    return { pt.x, pt.y };
}

void usagi::Win32Window::captureCursor()
{
    confineCursorInClientArea();

    mMouseCursorCaptured = true;
}

void usagi::Win32Window::releaseCursor()
{
    // remove cursor restriction
    ClipCursor(nullptr);
    mMouseCursorCaptured = false;
}

bool usagi::Win32Window::isCursorCaptured()
{
    return mMouseCursorCaptured;
}

void usagi::Win32Window::centerCursor()
{
    const auto rect = getClientScreenRect();
    Vector2i cursor {
        (rect.left + rect.right) / 2,
        (rect.top + rect.bottom) / 2
    };
    SetCursorPos(cursor.x(), cursor.y());
}

void usagi::Win32Window::showCursor(bool show)
{
    if(mShowMouseCursor == show) return;

    ShowCursor(show);
    mShowMouseCursor = show;
}

bool usagi::Win32Window::isButtonPressed(MouseButtonCode button) const
{
    const auto idx = static_cast<std::size_t>(button);
    if(idx > sizeof(mMouseButtonDown) / sizeof(bool)) return false;
    return mMouseButtonDown[idx];
}

HWND usagi::Win32Window::getNativeWindowHandle() const
{
    return mWindowHandle;
}

HINSTANCE usagi::Win32Window::getProcessInstanceHandle()
{
    return mProcessInstanceHandle;
}

﻿#include <Usagi/Engine/Utility/String.hpp>

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

void usagi::Win32Window::registerRawInputDevices() const
{
    RAWINPUTDEVICE Rid[2];

    // adds HID mouse, RIDEV_NOLEGACY is not used because we need the system
    // to process non-client area.
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    // receives device add/remove messages
    Rid[0].dwFlags = RIDEV_DEVNOTIFY;
    Rid[0].hwndTarget = mWindowHandle;

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
    Rid[1].dwFlags = RIDEV_DEVNOTIFY;
    Rid[1].hwndTarget = mWindowHandle;

    // note that this registration affects the entire application
    if(RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
    {
        //registration failed. Call GetLastError for the cause of the error
        throw std::runtime_error("RegisterRawInputDevices() failed");
    }
}

usagi::Win32Window::Win32Window(
    const std::string &title, const Vector2u32 &size)
    : mMouse { this }
{
    ensureWindowSubsystemInitialized();
    createWindowHandle(title, size.x(), size.y());
    registerRawInputDevices();
    Win32Window::show(true);
}

HDC usagi::Win32Window::deviceContext() const
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

void usagi::Win32Window::close()
{
    DestroyWindow(mWindowHandle);
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
    {
        throw std::runtime_error(
            "GetRawInputData does not return correct size!"
        );
    }

    return std::move(lpb);
}

LRESULT usagi::Win32Window::handleWindowMessage(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        // unbuffered raw input data
        case WM_INPUT:
        {
            auto lpb = getRawInputBuffer(lParam);
            const auto raw = reinterpret_cast<RAWINPUT*>(lpb.get());

            switch(raw->header.dwType)
            {
                case RIM_TYPEKEYBOARD:
                {
                    mKeyboard.processKeyboardInput(raw);
                    break;
                }
                case RIM_TYPEMOUSE:
                {
                    mMouse.processMouseInput(raw);
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
                mMouse.recaptureCursor();
            }
            // window being deactivated
            else
            {
                mKeyboard.clearKeyPressedStates();
                break;
            }
            break;
        }
        // todo: sent resize/move events
        case WM_SIZE:
        case WM_MOVE:
        {
            mMouse.recaptureCursor();
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

HWND usagi::Win32Window::handle() const
{
    return mWindowHandle;
}

HINSTANCE usagi::Win32Window::processInstanceHandle()
{
    return mProcessInstanceHandle;
}

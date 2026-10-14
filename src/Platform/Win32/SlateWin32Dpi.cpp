// ============================================================================
// SlateWin32Dpi — see include/ZSlate/Platform/Win32/SlateWin32Dpi.h
// ============================================================================

#include "ZSlate/Platform/Win32/SlateWin32Dpi.h"

#if defined(_WIN32)

namespace ZSlate
{
namespace Win32
{

void DeclareDpiAwareness()
{
    const HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32 != nullptr)
    {
        using SetCtxFn = BOOL(WINAPI*)(HANDLE);
        const auto set_ctx = reinterpret_cast<SetCtxFn>(
            reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
        // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4
        if (set_ctx != nullptr &&
            set_ctx(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4))))
        {
            return;
        }
    }
    // Pre-1703 fallback: system-wide awareness. No per-monitor updates, but it
    // still stops the stretch on the primary monitor.
    SetProcessDPIAware();
}

float QueryDpiScaleForWindow(HWND hwnd)
{
    const HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32 != nullptr)
    {
        using GetDpiFn = UINT(WINAPI*)(HWND);
        const auto get_dpi = reinterpret_cast<GetDpiFn>(
            reinterpret_cast<void*>(GetProcAddress(user32, "GetDpiForWindow")));
        if (get_dpi != nullptr)
        {
            const UINT dpi = get_dpi(hwnd);
            if (dpi != 0)
                return static_cast<float>(dpi) / 96.0f;
        }
    }
    // Fallback: device caps on the window DC.
    float scale = 1.0f;
    const HDC dc = GetDC(hwnd);
    if (dc != nullptr)
    {
        const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        if (dpi > 0)
            scale = static_cast<float>(dpi) / 96.0f;
        ReleaseDC(hwnd, dc);
    }
    return scale;
}

float QuerySystemDpiScale()
{
    float scale = 1.0f;
    const HDC screen = GetDC(nullptr);
    if (screen != nullptr)
    {
        const int dpi = GetDeviceCaps(screen, LOGPIXELSX);
        if (dpi > 0)
            scale = static_cast<float>(dpi) / 96.0f;
        ReleaseDC(nullptr, screen);
    }
    return scale;
}

void ResizeWindowForDpi(HWND hwnd, int logical_w, int logical_h, unsigned dpi)
{
    if (hwnd == nullptr || logical_w <= 0 || logical_h <= 0)
        return;

    if (dpi == 0)
        dpi = 96;

    const DWORD style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));
    RECT rect {0, 0, logical_w, logical_h};

    const HMODULE user32 = GetModuleHandleA("user32.dll");
    using AdjustFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    const auto adjust_ex = reinterpret_cast<AdjustFn>(
        reinterpret_cast<void*>(GetProcAddress(user32, "AdjustWindowRectExForDpi")));

    if (adjust_ex != nullptr)
    {
        adjust_ex(&rect, style, FALSE, 0, dpi);
    }
    else
    {
        // AdjustWindowRect computes margins in the system DPI, so the result
        // has to be grown by the ratio between the window's DPI and 96.
        RECT at96 {0, 0, logical_w, logical_h};
        AdjustWindowRectEx(&at96, style, FALSE, 0);
        const long margin_w = (at96.right - at96.left) - logical_w;
        const long margin_h = (at96.bottom - at96.top) - logical_h;
        const float s = static_cast<float>(dpi) / 96.0f;
        rect.right  = logical_w + static_cast<long>(margin_w * s);
        rect.bottom = logical_h + static_cast<long>(margin_h * s);
    }

    SetWindowPos(hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

}  // namespace Win32
}  // namespace ZSlate

#endif  // _WIN32

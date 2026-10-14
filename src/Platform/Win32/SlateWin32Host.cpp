// ============================================================================
// SlateWin32Host — see include/ZSlate/Platform/Win32/SlateWin32Host.h
// ============================================================================

#include "ZSlate/Platform/Win32/SlateWin32Host.h"

#if defined(_WIN32)

#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Core/SlateClipboard.h"

#include <cstdio>
#include <cstring>
#include <filesystem>

#include <shellapi.h>  // CommandLineToArgvW

namespace ZSlate
{
namespace Win32
{

// ---- Fonts ---------------------------------------------------------------

bool LoadSystemFonts(ZSlateFontAtlas& atlas, ESystemFont preference)
{
    wchar_t windows_dir[MAX_PATH] {};
    if (GetWindowsDirectoryW(windows_dir, MAX_PATH) == 0)
        return false;

    const std::filesystem::path fonts = std::filesystem::path(windows_dir) / L"Fonts";

    // Primary candidates, best first; the loser stays as a fallback so a
    // machine missing the preferred face still renders rather than blocking.
    const wchar_t* sans[] = {L"segoeui.ttf", L"tahoma.ttf", L"arial.ttf"};
    const wchar_t* mono[] = {L"consola.ttf", L"cour.ttf"};

    const wchar_t* const* preferred = (preference == ESystemFont::Mono) ? mono : sans;
    const size_t   preferred_count  = (preference == ESystemFont::Mono)
                                          ? sizeof(mono) / sizeof(mono[0])
                                          : sizeof(sans) / sizeof(sans[0]);
    const wchar_t* const* other    = (preference == ESystemFont::Mono) ? sans : mono;
    const size_t   other_count     = (preference == ESystemFont::Mono)
                                          ? sizeof(sans) / sizeof(sans[0])
                                          : sizeof(mono) / sizeof(mono[0]);

    bool loaded = false;
    for (size_t i = 0; i < preferred_count && !loaded; ++i)
        loaded = atlas.LoadFromFile(Utf8FromWide((fonts / preferred[i]).c_str()));
    for (size_t i = 0; i < other_count && !loaded; ++i)
        loaded = atlas.LoadFromFile(Utf8FromWide((fonts / other[i]).c_str()));

    if (!loaded)
        return false;

    // CJK fallback: the primary faces above have no Han glyphs.
    const wchar_t* cjk[] = {L"msyh.ttc", L"msyhlthc.ttc", L"simsun.ttc", L"simhei.ttf"};
    for (const wchar_t* f : cjk)
    {
        if (atlas.AddFallbackFont(Utf8FromWide((fonts / f).c_str())))
            break;
    }
    return true;
}

// ---- Keys ----------------------------------------------------------------

EKey KeyFromVirtualKey(unsigned vk)
{
    switch (vk)
    {
    case VK_BACK:   return EKey::Backspace;
    case VK_DELETE: return EKey::Delete;
    case VK_RETURN: return EKey::Enter;
    case VK_ESCAPE: return EKey::Escape;
    case VK_TAB:    return EKey::Tab;
    case VK_LEFT:   return EKey::Left;
    case VK_RIGHT:  return EKey::Right;
    case VK_UP:     return EKey::Up;
    case VK_DOWN:   return EKey::Down;
    case VK_HOME:   return EKey::Home;
    case VK_END:    return EKey::End;
    case VK_SPACE:  return EKey::Space;
    default: break;
    }
    if (vk >= 'A' && vk <= 'Z')
        return static_cast<EKey>(static_cast<int>(EKey::A) + static_cast<int>(vk - 'A'));
    return EKey::Unknown;
}

// ---- Clipboard -----------------------------------------------------------

namespace
{
// The ZSlate getter returns a bare `const char*`, so the bytes have to outlive
// the call. A process-wide buffer is what every host ends up doing; one copy
// here beats one per tool.
std::string& ClipboardBuffer()
{
    static std::string buffer;
    return buffer;
}

const char* GetClipboardText(void*)
{
    std::string& out = ClipboardBuffer();
    out.clear();
    if (!OpenClipboard(nullptr))
        return out.c_str();

    if (const HANDLE data = GetClipboardData(CF_UNICODETEXT))
    {
        if (const auto* wide = static_cast<const wchar_t*>(GlobalLock(data)))
        {
            out = Utf8FromWide(wide);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return out.c_str();
}

void SetClipboardText(const char* text, void*)
{
    if (text == nullptr || !OpenClipboard(nullptr))
        return;
    EmptyClipboard();

    const std::wstring wide = WideFromUtf8(text);
    if (wide.empty())
    {
        CloseClipboard();
        return;
    }

    const SIZE_T bytes = (wide.size() + 1) * sizeof(wchar_t);
    if (const HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes))
    {
        if (auto* destination = static_cast<wchar_t*>(GlobalLock(memory)))
        {
            std::memcpy(destination, wide.c_str(), bytes);
            GlobalUnlock(memory);
            SetClipboardData(CF_UNICODETEXT, memory);
        }
        else
        {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
}
}  // namespace

void InstallClipboardBridge()
{
    SlateApplication::Get().SetClipboardCallbacks(&GetClipboardText, &SetClipboardText, nullptr);
}

// ---- Strings -------------------------------------------------------------

std::string Utf8FromWide(const wchar_t* wide)
{
    if (wide == nullptr)
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1)
        return {};
    std::string out(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &out[0], size, nullptr, nullptr);
    return out;
}

std::wstring WideFromUtf8(const char* utf8)
{
    if (utf8 == nullptr)
        return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (size <= 1)
        return {};
    std::wstring out(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &out[0], size);
    return out;
}

std::vector<std::string> CommandLineArgsUtf8()
{
    std::vector<std::string> args;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr)
        return args;
    for (int i = 0; i < argc; ++i)
        args.push_back(Utf8FromWide(argv[i]));
    LocalFree(argv);
    return args;
}

// ---- Console -------------------------------------------------------------

bool StdOutIsRedirected()
{
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == nullptr || out == INVALID_HANDLE_VALUE)
        return false;
    const DWORD type = GetFileType(out);
    return type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE;
}

void EnsureConsole()
{
    // Capturing the output already: attaching a console would steal the
    // parent's input and leave a stray window behind.
    if (StdOutIsRedirected())
        return;

    // ATTACH_PARENT_PROCESS == (DWORD)-1.
    if (!AttachConsole(static_cast<DWORD>(-1)) && !AllocConsole())
        return;

    // The C runtime captured the (invalid) handles before we had a console, so
    // stdout/stderr still point nowhere until they are reopened on CONOUT$.
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    // No buffering: without a console-attached CRT flush, a crash or an early
    // return loses the last lines -- typically the ones explaining the crash.
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
}

}  // namespace Win32
}  // namespace ZSlate

#endif  // _WIN32

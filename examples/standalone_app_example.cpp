// ============================================================================
// ZSlate Standalone Application Example
// ============================================================================
// Demonstrates building a complete UI with ZSlate using ZSlateBatchedRenderer.
//
// On Windows, opens a native Win32 window (no external library needed).
// On other platforms, adapt the window creation to GLFW / SDL / your toolkit.
//
// The renderer records draw calls into CPU-side vertex/index buffers after
// each paint.  In a real GPU app, upload those buffers and draw them.
// ============================================================================

#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Renderer/ZSlateBatchedRenderer.h"
#include "ZSlate/Widgets/Panels/SBorder.h"
#include "ZSlate/Widgets/Layout/SBoxPanel.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Input/SCheckBox.h"
#include "ZSlate/Widgets/Input/SEditableTextBox.h"
#include "ZSlate/Widgets/Input/SSlider.h"
#include "ZSlate/Widgets/Layout/SSpacer.h"
#include "ZSlate/Widgets/Text/STextBlock.h"
#include "ZSlate/Widgets/SWidget.h"

#include <cstdio>
#include <memory>
#include <string>
#include <chrono>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

// ============================================================================
// Mock Font Service
// ============================================================================

struct MockFontService : public ZSlate::ISlateFontService
{
    std::unordered_map<std::string, void*> m_FontCache;

    void* LoadFont(const std::string& path, float size) override
    {
        void* h = reinterpret_cast<void*>(path.size() + static_cast<size_t>(size));
        m_FontCache[path + std::to_string(static_cast<int>(size))] = h;
        return h;
    }
    void UnloadFont(void*) override {}
    ZSlate::Vector2 MeasureText(void*, const std::string& text) const override
    {
        return ZSlate::Vector2(static_cast<float>(text.size()) * 8.0f, 14.0f);
    }
    void* GetDefaultFont() const override { return nullptr; }
};

// ============================================================================
// Platform
// ============================================================================

class DemoPlatform : public ZSlate::ISlatePlatform
{
public:
    ZSlate::ZSlateBatchedRenderer Renderer;
    MockFontService               FontService;

    bool            m_MouseDown[3] {};
    ZSlate::Vector2 m_MousePos {0, 0};
    ZSlate::Vector2 m_WinSize {800, 600};

    ZSlate::ISlateRenderer*    GetRenderer()    override { return &Renderer; }
    ZSlate::ISlateFontService* GetFontService() override { return &FontService; }
    ZSlate::Vector2            GetMousePosition()    const override { return m_MousePos; }
    bool                       IsMouseButtonDown(int b) const override { return b < 3 ? m_MouseDown[b] : false; }
    bool                       IsKeyDown(int)        const override { return false; }
    float                      GetTimeSeconds()      const override
    {
        static auto start = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }
    ZSlate::Vector2 GetWindowSize() const override { return m_WinSize; }
};

// ============================================================================
// Demo App
// ============================================================================

class DemoApp
{
public:
    DemoPlatform m_Platform;

    void BuildUI()
    {
        auto root = std::make_shared<ZSlate::SVerticalBox>();

        // Title
        auto t = std::make_shared<ZSlate::STextBlock>();
        t->Text = "ZSlate Standalone Demo";
        t->FontSize = 24;
        t->Color = ZSlate::Colors::White;
        root->AddSlot(t);
        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 12)));

        // Button
        auto btn = std::make_shared<ZSlate::SButton>();
        auto btnLabel = std::make_shared<ZSlate::STextBlock>();
        btnLabel->Text = "Click Me";
        btnLabel->Color = ZSlate::Colors::White;
        btn->SetContent(btnLabel);
        btn->OnClicked = []() { printf("[ZSlate] Button clicked!\n"); };
        root->AddSlot(btn);

        // Checkbox
        auto cb = std::make_shared<ZSlate::SCheckBox>();
        cb->SetLabel("Enable feature");
        cb->Checked = true;
        root->AddSlot(cb);

        // Slider
        auto sl = std::make_shared<ZSlate::SSlider>();
        sl->Value = 50;
        root->AddSlot(sl);

        ZSlate::SlateApplication::Get().SetRootContent(root);
    }

    void RunFrame()
    {
        m_Platform.Renderer.BeginFrame();
        ZSlate::UIRect region(0, 0, m_Platform.GetWindowSize().x, m_Platform.GetWindowSize().y);
        ZSlate::SlateApplication::Get().PaintInto(&m_Platform.Renderer, region);
        m_Platform.Renderer.EndFrame();

        // Print stats every 60 frames
        static int frameNum = 0;
        if (++frameNum % 60 == 0)
        {
            const auto& c = m_Platform.Renderer.GetCommands();
            size_t tris = 0;
            for (const auto& cmd : c) tris += cmd.IndexCount / 3;
            printf("[Frame %d] %zu draw commands, %zu vertices, %zu triangles\n",
                   frameNum, c.size(), m_Platform.Renderer.GetVertices().size(), tris);
        }
    }
};

// ============================================================================
// Main — real window loop
// ============================================================================

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static DemoApp* app = nullptr;

    switch (msg)
    {
    case WM_CREATE:
        app = reinterpret_cast<DemoApp*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        return 0;

    case WM_SIZE:
        if (app)
        {
            app->m_Platform.m_WinSize.x = static_cast<float>(LOWORD(lp));
            app->m_Platform.m_WinSize.y = static_cast<float>(HIWORD(lp));
        }
        return 0;

    case WM_MOUSEMOVE:
        if (app)
            app->m_Platform.m_MousePos = ZSlate::Vector2(
                static_cast<float>(LOWORD(lp)), static_cast<float>(HIWORD(lp)));
        return 0;

    case WM_LBUTTONDOWN: app->m_Platform.m_MouseDown[0] = true;  return 0;
    case WM_LBUTTONUP:   app->m_Platform.m_MouseDown[0] = false; return 0;
    case WM_RBUTTONDOWN: app->m_Platform.m_MouseDown[1] = true;  return 0;
    case WM_RBUTTONUP:   app->m_Platform.m_MouseDown[1] = false; return 0;
    case WM_MBUTTONDOWN: app->m_Platform.m_MouseDown[2] = true;  return 0;
    case WM_MBUTTONUP:   app->m_Platform.m_MouseDown[2] = false; return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int main()
{
    printf("=== ZSlate Standalone (Native Win32 Window) ===\n");

    DemoApp app;
    ZSlate::SetPlatform(&app.m_Platform);

    // Tell the renderer how to measure text (default: 0.6 * fontSize per char)
    app.m_Platform.Renderer.SetTextMeasurerCallback(
        [](const std::string& text, float fontSize) -> ZSlate::Vector2 {
            return ZSlate::Vector2(static_cast<float>(text.length()) * fontSize * 0.6f, fontSize * 1.2f);
        });

    app.BuildUI();

    // ---- Create a native window ---------------------------------------------
    WNDCLASSA wc {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ZSlateDemoWindow";
    RegisterClassA(&wc);

    int w = static_cast<int>(app.m_Platform.m_WinSize.x);
    int h = static_cast<int>(app.m_Platform.m_WinSize.y);
    HWND hwnd = CreateWindowA("ZSlateDemoWindow", "ZSlate Demo",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              w, h, nullptr, nullptr, GetModuleHandle(nullptr), &app);
    ShowWindow(hwnd, SW_SHOW);

    // ---- Message loop -------------------------------------------------------
    MSG msg {};
    while (true)
    {
        // Peek + dispatch all pending messages
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Paint one ZSlate frame
        app.RunFrame();

        // Invalidate to trigger redraw (naive: every frame)
        InvalidateRect(hwnd, nullptr, FALSE);
        Sleep(1);  // yield to avoid 100% CPU
    }

    return 0;
}

#else  // non-Windows — adapt to your windowing toolkit

int main()
{
    printf("=== ZSlate Standalone (Console Mode) ===\n");
    printf("This example needs a real window. Adapt to your toolkit:\n");
    printf("  - GLFW:  glfwCreateWindow + glfwPollEvents loop\n");
    printf("  - SDL:   SDL_CreateWindow + SDL_PollEvent loop\n\n");

    DemoApp app;
    ZSlate::SetPlatform(&app.m_Platform);
    app.BuildUI();

    printf("Press Enter to exit after 3 demo frames...\n");
    for (int i = 0; i < 3; i++) app.RunFrame();
    getchar();
    return 0;
}

#endif

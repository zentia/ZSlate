// ============================================================================
// ZSlate Standalone Example — Win32 + D3D11 + Font Atlas + Input Routing
// ============================================================================
#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Renderer/ZSlateBatchedRenderer.h"
#include "ZSlate/Renderer/ZSlateFontAtlas.h"
#include "ZSlate/Platform/D3D11/ZSlateD3D11Renderer.h"
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
#include <vector>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #undef DrawText  // windows.h #defines DrawText → DrawTextA/W
#endif

// ============================================================================
struct MockFontService : public ZSlate::ISlateFontService
{
    void* LoadFont(const std::string&, float) override { return nullptr; }
    void UnloadFont(void*) override {}
    ZSlate::Vector2 MeasureText(void*, const std::string& t) const override
    { return ZSlate::Vector2((float)t.size() * 8.0f, 14.0f); }
    void* GetDefaultFont() const override { return nullptr; }
};

// ============================================================================
class DemoPlatform : public ZSlate::ISlatePlatform
{
public:
    ZSlate::ZSlateBatchedRenderer Renderer;
    MockFontService               FontService;
    bool          m_MouseDown[3] {};
    ZSlate::Vector2 m_MousePos {0, 0};
    ZSlate::Vector2 m_WinSize  {800, 600};

    ZSlate::ISlateRenderer*    GetRenderer()    override { return &Renderer; }
    ZSlate::ISlateFontService* GetFontService() override { return &FontService; }
    ZSlate::Vector2 GetMousePosition()   const override { return m_MousePos; }
    bool IsMouseButtonDown(int b) const override { return b<3 ? m_MouseDown[b] : false; }
    bool IsKeyDown(int)           const override { return false; }
    float GetTimeSeconds()        const override {
        static auto t0 = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(std::chrono::steady_clock::now()-t0).count();
    }
    ZSlate::Vector2 GetWindowSize() const override { return m_WinSize; }
};

// ============================================================================
class DemoApp
{
public:
    DemoPlatform                   m_Platform;
    ZSlate::ZSlateFontAtlas        m_FontAtlas;
    ZSlate::SlateInputRouter       m_Input;

    bool InitFonts()
    {
        const char* fonts[] = {
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\consola.ttf",
        };
        for (const char* f : fonts) { if (m_FontAtlas.LoadFromFile(f)) break; }
        if (m_FontAtlas.IsLoaded())
        {
            printf("[Font] Loaded\n");
            const char* cjk[] = {"C:\\Windows\\Fonts\\msyh.ttc","C:\\Windows\\Fonts\\simsun.ttc"};
            for (const char* f : cjk) { if (m_FontAtlas.AddFallbackFont(f)) { printf("[Font] CJK fallback OK\n"); break; } }
        }
        else printf("[Font] No TTF found — bar text\n");
        m_Platform.Renderer.SetFontAtlas(m_FontAtlas.IsLoaded() ? &m_FontAtlas : nullptr);
        return m_FontAtlas.IsLoaded();
    }

    void BuildUI()
    {
        auto root = std::make_shared<ZSlate::SVerticalBox>();

        auto t = std::make_shared<ZSlate::STextBlock>();
        t->Text = "ZSlate Standalone Demo"; t->FontSize = 24; t->Color = ZSlate::Colors::White;
        root->AddSlot(t);
        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 12)));

        auto sub = std::make_shared<ZSlate::STextBlock>();
        sub->Text = "Font atlas + input routing via SlateInputRouter";
        sub->FontSize = 14; sub->Color = ZSlate::UIColor(0.6f, 0.7f, 0.9f, 1.0f);
        root->AddSlot(sub);
        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 16)));

        auto btn = std::make_shared<ZSlate::SButton>();
        auto lbl = std::make_shared<ZSlate::STextBlock>();
        lbl->Text = "Click Me"; lbl->Color = ZSlate::Colors::White;
        btn->SetContent(lbl);
        btn->OnClicked = []{ printf("[ZSlate] Click!\n"); };
        root->AddSlot(btn);
        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 8)));

        auto cb = std::make_shared<ZSlate::SCheckBox>();
        cb->SetLabel("Enable feature"); cb->Checked = true;
        root->AddSlot(cb);
        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 8)));

        auto sl = std::make_shared<ZSlate::SSlider>();
        sl->Value = 50;
        root->AddSlot(sl);

        ZSlate::SlateApplication::Get().SetRootContent(root);
    }

    void RunFrame()
    {
        auto root = ZSlate::SlateApplication::Get().GetRootContent();
        if (root)
        {
            bool over = m_Platform.m_MousePos.x >= 0 && m_Platform.m_MousePos.y >= 0;
            m_Input.ProcessMouse(root, m_Platform.m_MousePos, over,
                m_Platform.m_MouseDown[0], 0.0f, m_Platform.m_MouseDown[1]);
        }

        m_Platform.Renderer.BeginFrame();
        ZSlate::UIRect r(0, 0, m_Platform.GetWindowSize().x, m_Platform.GetWindowSize().y);
        ZSlate::SlateApplication::Get().PaintInto(&m_Platform.Renderer, r);
        m_Platform.Renderer.EndFrame();
    }
};

// ============================================================================
#ifdef _WIN32

struct WinCtx { DemoApp* app; ZSlate::ZSlateD3D11Renderer* backend = nullptr; };

LRESULT CALLBACK WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    WinCtx* ctx = nullptr;
    if (msg == WM_CREATE) {
        ctx = reinterpret_cast<WinCtx*>(((CREATESTRUCT*)lp)->lpCreateParams);
        SetWindowLongPtr(hw, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    } else {
        ctx = reinterpret_cast<WinCtx*>(GetWindowLongPtr(hw, GWLP_USERDATA));
    }

    DemoApp* app = ctx ? ctx->app : nullptr;
    switch (msg)
    {
    case WM_SIZE:
        if (app) { app->m_Platform.m_WinSize = ZSlate::Vector2((float)LOWORD(lp),(float)HIWORD(lp));
                   if (ctx->backend) ctx->backend->Resize(LOWORD(lp), HIWORD(lp)); } return 0;
    case WM_MOUSEMOVE:   if (app) app->m_Platform.m_MousePos = ZSlate::Vector2((float)LOWORD(lp),(float)HIWORD(lp)); return 0;
    case WM_LBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[0]=true;  return 0;
    case WM_LBUTTONUP:   if (app) app->m_Platform.m_MouseDown[0]=false; return 0;
    case WM_RBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[1]=true;  return 0;
    case WM_RBUTTONUP:   if (app) app->m_Platform.m_MouseDown[1]=false; return 0;
    case WM_MBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[2]=true;  return 0;
    case WM_MBUTTONUP:   if (app) app->m_Platform.m_MouseDown[2]=false; return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hw, msg, wp, lp);
}

int main()
{
    printf("=== ZSlate Standalone (Win32 + D3D11) ===\n");
    DemoApp app;
    ZSlate::SetPlatform(&app.m_Platform);
    app.InitFonts();
    app.BuildUI();

    WNDCLASSA wc {}; wc.lpfnWndProc=WndProc; wc.hInstance=GetModuleHandle(nullptr);
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.lpszClassName="ZSlateDemo";
    RegisterClassA(&wc);

    WinCtx ctx{&app, nullptr};
    HWND hw = CreateWindowA("ZSlateDemo", "ZSlate Standalone Demo",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        (int)app.m_Platform.m_WinSize.x, (int)app.m_Platform.m_WinSize.y,
        nullptr, nullptr, GetModuleHandle(nullptr), &ctx);

    auto* backend = new ZSlate::ZSlateD3D11Renderer(hw);
    ctx.backend = backend;

    if (!backend->Init()) { printf("D3D11 init failed\n"); delete backend; return 1; }
    ShowWindow(hw, SW_SHOW);

    MSG msg {};
    while (true)
    {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        { if (msg.message == WM_QUIT) goto out; TranslateMessage(&msg); DispatchMessage(&msg); }

        app.RunFrame();
        backend->Render(app.m_Platform.Renderer, &app.m_FontAtlas);

        static int fc=0;
        if (++fc%120==0) printf("[%d] %zu cmd\n", fc, app.m_Platform.Renderer.GetCommands().size());
        Sleep(1);
    }
out:
    backend->Shutdown();
    delete backend;
    return 0;
}

#else
int main() {
    printf("This example uses D3D11 on Windows.\n");
    return 0;
}
#endif

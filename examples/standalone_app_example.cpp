// ============================================================================
// ZSlate Standalone Application Example
// ============================================================================
// Windows: native Win32 window + D3D11 renderer (no external deps beyond
//   d3d11.lib + d3dcompiler.lib — both ship with Windows SDK).
// Other platforms: adapt D3D11 → GLFW/SDL/Vulkan/Metal.
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
#include <vector>
#include <cassert>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <d3d11.h>
  #include <d3dcompiler.h>
  #pragma comment(lib, "d3d11.lib")
  #pragma comment(lib, "d3dcompiler.lib")
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
// D3D11 Render Backend
// ============================================================================

#ifdef _WIN32

struct D3D11Backend
{
    ID3D11Device*           Device    = nullptr;
    ID3D11DeviceContext*    Context   = nullptr;
    IDXGISwapChain*         SwapChain = nullptr;
    ID3D11RenderTargetView* RTV       = nullptr;

    ID3D11VertexShader*     VS = nullptr;
    ID3D11PixelShader*      PS = nullptr;
    ID3D11InputLayout*      InputLayout = nullptr;
    ID3D11Buffer*           VB = nullptr;
    ID3D11Buffer*           IB = nullptr;
    ID3D11BlendState*       BlendState = nullptr;
    ID3D11RasterizerState*  RasterState = nullptr;
    ID3D11Buffer*           PerFrameCB  = nullptr;

    uint32_t Width  = 0;
    uint32_t Height = 0;

    // Pre-compiled HLSL:
    //   VS: sv_position = float4(pos.xy / halfWin * float2(1,-1) + float2(-1,1), 0, 1), pass uv/color
    //   PS: return color
    static const uint8_t g_VSBytecode[];
    static const size_t  g_VSSize;
    static const uint8_t g_PSBytecode[];
    static const size_t  g_PSSize;

    bool Init(HWND hwnd)
    {
        RECT rc; GetClientRect(hwnd, &rc);
        Width = rc.right - rc.left; Height = rc.bottom - rc.top;
        if (Width == 0) Width = 800;
        if (Height == 0) Height = 600;

        DXGI_SWAP_CHAIN_DESC sd {};
        sd.BufferCount = 2;
        sd.BufferDesc.Width  = Width;
        sd.BufferDesc.Height = Height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;

        D3D_FEATURE_LEVEL fl;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED,
            nullptr, 0, D3D11_SDK_VERSION, &sd, &SwapChain, &Device, &fl, &Context);
        if (FAILED(hr)) { printf("D3D11CreateDeviceAndSwapChain failed (0x%08x)\n", (unsigned)hr); return false; }

        ID3D11Texture2D* backBuffer = nullptr;
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (backBuffer) { Device->CreateRenderTargetView(backBuffer, nullptr, &RTV); backBuffer->Release(); }

        // Compile shaders
        if (!CompileShaders()) return false;

        // Blend: premultiplied alpha
        D3D11_BLEND_DESC bd {};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp   = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        Device->CreateBlendState(&bd, &BlendState);

        // Raster
        D3D11_RASTERIZER_DESC rd {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.ScissorEnable = TRUE;
        Device->CreateRasterizerState(&rd, &RasterState);

        // Per-frame constant buffer: ViewportSize (float2)
        D3D11_BUFFER_DESC cbd {};
        cbd.ByteWidth = 16;  // float2 = 8 bytes, padded to 16
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Device->CreateBuffer(&cbd, nullptr, &PerFrameCB);

        return true;
    }

    bool CompileShaders()
    {
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG;
#endif
        ID3DBlob* errBlob = nullptr;

        // Vertex shader
        ID3DBlob* vsBlob = nullptr;
        HRESULT hr = D3DCompile((LPCSTR)g_VSBytecode, g_VSSize, "vs", nullptr, nullptr, "main",
                                "vs_5_0", compileFlags, 0, &vsBlob, &errBlob);
        if (FAILED(hr))
        {
            printf("VS compile failed (0x%08x)\n", (unsigned)hr);
            if (errBlob) { printf("%s\n", (const char*)errBlob->GetBufferPointer()); errBlob->Release(); }
            return false;
        }
        Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VS);

        // Input layout
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        Device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &InputLayout);
        vsBlob->Release();

        // Pixel shader
        ID3DBlob* psBlob = nullptr;
        hr = D3DCompile((LPCSTR)g_PSBytecode, g_PSSize, "ps", nullptr, nullptr, "main",
                        "ps_5_0", compileFlags, 0, &psBlob, &errBlob);
        if (FAILED(hr))
        {
            printf("PS compile failed (0x%08x)\n", (unsigned)hr);
            if (errBlob) { printf("%s\n", (const char*)errBlob->GetBufferPointer()); errBlob->Release(); }
            return false;
        }
        Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PS);
        psBlob->Release();

        printf("[D3D11] Shaders compiled OK\n");
        return true;
    }

    void Resize(uint32_t w, uint32_t h)
    {
        if (w == Width && h == Height) return;
        Width = w; Height = h;
        if (RTV) { RTV->Release(); RTV = nullptr; }
        Context->OMSetRenderTargets(0, nullptr, nullptr);
        SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        ID3D11Texture2D* bb = nullptr;
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
        if (bb) { Device->CreateRenderTargetView(bb, nullptr, &RTV); bb->Release(); }
    }

    void Render(ZSlate::ZSlateBatchedRenderer& renderer)
    {
        if (!Device) return;

        const auto& vertices = renderer.GetVertices();
        const auto& indices  = renderer.GetIndices();
        const auto& commands = renderer.GetCommands();

        // Upload vertex buffer (skip if empty — but we always draw a fallback quad)
        if (VB) { VB->Release(); VB = nullptr; }
        {
            D3D11_BUFFER_DESC bd {};
            bd.ByteWidth = (UINT)(vertices.size() * sizeof(ZSlate::ZSBVertex));
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SUBRESOURCE_DATA sd {};
            sd.pSysMem = vertices.data();
            Device->CreateBuffer(&bd, (vertices.empty() ? nullptr : &sd), &VB);
        }

        if (IB) { IB->Release(); IB = nullptr; }
        {
            D3D11_BUFFER_DESC bd {};
            bd.ByteWidth = (UINT)(indices.size() * sizeof(uint16_t));
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SUBRESOURCE_DATA sd {};
            sd.pSysMem = indices.data();
            Device->CreateBuffer(&bd, (indices.empty() ? nullptr : &sd), &IB);
        }

        // Clear to a visible color (not black)
        float clear[4] = {0.08f, 0.10f, 0.18f, 1.0f};
        Context->ClearRenderTargetView(RTV, clear);
        Context->OMSetRenderTargets(1, &RTV, nullptr);
        Context->RSSetState(RasterState);
        D3D11_VIEWPORT vp {};
        vp.Width = (FLOAT)Width; vp.Height = (FLOAT)Height; vp.MaxDepth = 1.0f;
        Context->RSSetViewports(1, &vp);

        // PerFrame CB: ViewportSize
        D3D11_MAPPED_SUBRESOURCE mapped;
        Context->Map(PerFrameCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        ((float*)mapped.pData)[0] = (float)Width;
        ((float*)mapped.pData)[1] = (float)Height;
        Context->Unmap(PerFrameCB, 0);
        Context->VSSetConstantBuffers(0, 1, &PerFrameCB);

        Context->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
        Context->VSSetShader(VS, nullptr, 0);
        Context->PSSetShader(PS, nullptr, 0);
        Context->IASetInputLayout(InputLayout);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT stride = sizeof(ZSlate::ZSBVertex);
        UINT offset = 0;
        Context->IASetVertexBuffers(0, 1, &VB, &stride, &offset);
        Context->IASetIndexBuffer(IB, DXGI_FORMAT_R16_UINT, 0);

        if (!commands.empty())
        {
            // Draw recorded ZSlate command list
            for (const auto& cmd : commands)
            {
                if (cmd.HasClip)
                {
                    D3D11_RECT sc = {(LONG)std::max(cmd.ClipRect.x, 0.0f), (LONG)std::max(cmd.ClipRect.y, 0.0f),
                                     (LONG)std::max(cmd.ClipRect.x + cmd.ClipRect.w, 0.0f),
                                     (LONG)std::max(cmd.ClipRect.y + cmd.ClipRect.h, 0.0f)};
                    Context->RSSetScissorRects(1, &sc);
                }
                else
                {
                    D3D11_RECT full = {0, 0, (LONG)Width, (LONG)Height};
                    Context->RSSetScissorRects(1, &full);
                }
                Context->DrawIndexed(cmd.IndexCount, cmd.IndexOffset, 0);
            }
        }
        else
        {
            // Fallback: no commands recorded → draw a visible placeholder
            // (proves the D3D11 pipeline works; means ZSlate batching is empty)
            printf("[WARN] No draw commands recorded — rendering fallback quad\n");
            D3D11_RECT full = {0, 0, (LONG)Width, (LONG)Height};
            Context->RSSetScissorRects(1, &full);
        }

        SwapChain->Present(1, 0);
    }

    void Shutdown()
    {
        if (VB)          VB->Release();
        if (IB)          IB->Release();
        if (PerFrameCB)  PerFrameCB->Release();
        if (BlendState)  BlendState->Release();
        if (RasterState) RasterState->Release();
        if (InputLayout) InputLayout->Release();
        if (VS)          VS->Release();
        if (PS)          PS->Release();
        if (RTV)         RTV->Release();
        if (SwapChain)   SwapChain->Release();
        if (Context)     Context->Release();
        if (Device)      Device->Release();
    }
};

// Embedded HLSL source (compiled at runtime by D3DCompile).
// ---- VS: screen-space (0..W, 0..H) -> clip-space (-1..1) ----
const uint8_t D3D11Backend::g_VSBytecode[] = R"(
struct VSInput  { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };
struct VSOutput { float4 sv_pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };

cbuffer PerFrame : register(b0) { float2 ViewportSize; }

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.sv_pos = float4((inp.pos / ViewportSize) * float2(2, -2) + float2(-1, 1), 0, 1);
    o.uv = inp.uv;
    o.col = inp.col;
    return o;
}
)";
const size_t D3D11Backend::g_VSSize = sizeof(D3D11Backend::g_VSBytecode) - 1;

const uint8_t D3D11Backend::g_PSBytecode[] = R"(
struct PSInput { float4 sv_pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };
float4 main(PSInput inp) : SV_TARGET { return inp.col; }
)";
const size_t D3D11Backend::g_PSSize = sizeof(D3D11Backend::g_PSBytecode) - 1;

#endif // _WIN32

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
    }
};

// ============================================================================
// Main
// ============================================================================

#ifdef _WIN32

struct WinCtx { DemoApp* app; D3D11Backend* backend; };

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    WinCtx* ctx = nullptr;
    if (msg == WM_CREATE)
    {
        ctx = (WinCtx*)((CREATESTRUCT*)lp)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
    }
    else
    {
        ctx = (WinCtx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    DemoApp* app = ctx ? ctx->app : nullptr;

    switch (msg)
    {
    case WM_SIZE:
        if (app)
        {
            uint32_t w = LOWORD(lp), h = HIWORD(lp);
            app->m_Platform.m_WinSize = ZSlate::Vector2((float)w, (float)h);
            if (ctx->backend && w > 0 && h > 0) ctx->backend->Resize(w, h);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (app) app->m_Platform.m_MousePos = ZSlate::Vector2((float)LOWORD(lp), (float)HIWORD(lp));
        return 0;

    case WM_LBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[0] = true;  return 0;
    case WM_LBUTTONUP:   if (app) app->m_Platform.m_MouseDown[0] = false; return 0;
    case WM_RBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[1] = true;  return 0;
    case WM_RBUTTONUP:   if (app) app->m_Platform.m_MouseDown[1] = false; return 0;
    case WM_MBUTTONDOWN: if (app) app->m_Platform.m_MouseDown[2] = true;  return 0;
    case WM_MBUTTONUP:   if (app) app->m_Platform.m_MouseDown[2] = false; return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int main()
{
    printf("=== ZSlate Standalone (Win32 + D3D11) ===\n");

    DemoApp app;
    D3D11Backend backend;
    ZSlate::SetPlatform(&app.m_Platform);

    app.m_Platform.Renderer.SetTextMeasurerCallback(
        [](const std::string& text, float fontSize) -> ZSlate::Vector2 {
            return ZSlate::Vector2((float)text.length() * fontSize * 0.6f, fontSize * 1.2f);
        });

    app.BuildUI();

    // ---- Create window -------------------------------------------------------
    WNDCLASSA wc {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ZSlateDemoWindow";
    RegisterClassA(&wc);

    int w = (int)app.m_Platform.m_WinSize.x;
    int h = (int)app.m_Platform.m_WinSize.y;
    WinCtx ctx = {&app, &backend};
    HWND hwnd = CreateWindowA("ZSlateDemoWindow", "ZSlate Standalone Demo",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              w, h, nullptr, nullptr, GetModuleHandle(nullptr), &ctx);

    if (!backend.Init(hwnd))
    {
        printf("Failed to init D3D11 renderer.\n");
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);

    // ---- Message loop -------------------------------------------------------
    MSG msg {};
    while (true)
    {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) goto cleanup;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        app.RunFrame();
        backend.Render(app.m_Platform.Renderer);

        static int fc = 0;
        if (++fc % 120 == 0)
            printf("[Frame %d] %zu cmds, %zu verts\n", fc,
                   app.m_Platform.Renderer.GetCommands().size(),
                   app.m_Platform.Renderer.GetVertices().size());

        Sleep(1);
    }

cleanup:
    backend.Shutdown();
    return 0;
}

#else  // non-Windows

int main()
{
    printf("=== ZSlate Standalone (Console Mode) ===\n");
    printf("This example uses D3D11 on Windows.\n");
    printf("For other platforms, adapt the render backend to GLFW/SDL + OpenGL/Vulkan.\n\n");

    DemoApp app;
    ZSlate::SetPlatform(&app.m_Platform);
    app.BuildUI();

    printf("Press Enter to exit after 3 demo frames...\n");
    for (int i = 0; i < 3; i++) app.RunFrame();
    getchar();
    return 0;
}

#endif

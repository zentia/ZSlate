// ============================================================================
// ZSlate Standalone Application Example
// ============================================================================
// Demonstrates building a complete UI with ZSlate using ZSlateBatchedRenderer,
// a battery-included ISlateRenderer that records draw calls into CPU-side
// vertex/index buffers.  After EndFrame(), read GetVertices()/GetIndices()/
// GetCommands() and submit to your GPU backend.
//
// To use with your own renderer: replace ZSlateBatchedRenderer with your
// ISlateRenderer implementation.  ZSlate never touches GPU APIs directly.
// ============================================================================

#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Core/SlateClipboard.h"
#include "ZSlate/Renderer/ZSlateBatchedRenderer.h"
#include "ZSlate/Widgets/Panels/SBorder.h"
#include "ZSlate/Widgets/Layout/SBoxPanel.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Input/SCheckBox.h"
#include "ZSlate/Widgets/Input/SEditableTextBox.h"
#include "ZSlate/Widgets/Panels/SOverlay.h"
#include "ZSlate/Widgets/Layout/SScrollBox.h"
#include "ZSlate/Widgets/Input/SSlider.h"
#include "ZSlate/Widgets/Layout/SSpacer.h"
#include "ZSlate/Widgets/Layout/SSplitter.h"
#include "ZSlate/Widgets/Text/STextBlock.h"
#include "ZSlate/Widgets/SWidget.h"

#include <cstdio>
#include <memory>
#include <string>

// ============================================================================
// Mock Font Service
// ============================================================================

struct MockFontService : public ZSlate::ISlateFontService
{
    std::unordered_map<std::string, void*> m_FontCache;

    void* LoadFont(const std::string& fontPath, float size) override
    {
        void* handle = reinterpret_cast<void*>(fontPath.size() + static_cast<size_t>(size));
        m_FontCache[fontPath + std::to_string(static_cast<int>(size))] = handle;
        return handle;
    }

    void UnloadFont(void* handle) override { (void)handle; }

    Vector2 MeasureText(void* fontHandle, const std::string& text) const override
    {
        (void)fontHandle;
        return Vector2(static_cast<float>(text.size()) * 8.0f, 14.0f);
    }

    void* GetDefaultFont() const override { return nullptr; }
};

// ============================================================================
// Platform (using ZSlateBatchedRenderer as the renderer backend)
// ============================================================================

class DemoPlatform : public ZSlate::ISlatePlatform
{
public:
    ZSlate::ZSlateBatchedRenderer Renderer;
    MockFontService               FontService;

    bool            m_MouseDown {false};
    ZSlate::Vector2 m_MousePos {400, 300};
    float           m_Time {0};
    ZSlate::Vector2 m_WinSize {800, 600};

    ZSlate::ISlateRenderer*    GetRenderer()    override { return &Renderer; }
    ZSlate::ISlateFontService* GetFontService() override { return &FontService; }
    ZSlate::Vector2            GetMousePosition()    const override { return m_MousePos; }
    bool                       IsMouseButtonDown(int) const override { return m_MouseDown; }
    bool                       IsKeyDown(int)        const override { return false; }
    float                      GetTimeSeconds()      const override { return m_Time; }
    ZSlate::Vector2            GetWindowSize()       const override { return m_WinSize; }
};

// ============================================================================
// Custom Widget: Color Picker
// ============================================================================

class SColorPicker : public ZSlate::SLeafWidget
{
public:
    ZSlate::UIColor Color {ZSlate::Colors::White};
    std::function<void(const ZSlate::UIColor&)> OnColorChanged;

    ZSlate::Vector2 ComputeDesiredSize() const override { return ZSlate::Vector2(80, 80); }

    void OnPaint(const ZSlate::FPaintContext& ctx, const ZSlate::FGeometry& geom) const override
    {
        if (!ctx.Renderer) return;
        ctx.Renderer->DrawQuad(geom.ToRect(), Color);
        ctx.Renderer->DrawRect(geom.ToRect(), ZSlate::Colors::Black, 1.0f);
    }

    ZSlate::FReply OnMouseButtonDown(const ZSlate::Vector2&, int button) override
    {
        if (button == 0 && OnColorChanged) OnColorChanged(Color);
        return ZSlate::FReply::Handled();
    }
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
        auto title = std::make_shared<ZSlate::STextBlock>();
        title->Text = "ZSlate Standalone Demo";
        title->FontSize = 24;
        title->Color = ZSlate::Colors::White;
        root->AddSlot(title);

        root->AddSlot(std::make_shared<ZSlate::SSpacer>(ZSlate::Vector2(0, 12)));

        // Buttons
        auto btn1 = std::make_shared<ZSlate::SButton>();
        btn1->SetText("Click Me");
        root->AddSlot(btn1);

        // Checkbox
        auto cb = std::make_shared<ZSlate::SCheckBox>();
        cb->SetLabel("Enable feature");
        cb->Checked = true;
        root->AddSlot(cb);

        // Slider
        auto slider = std::make_shared<ZSlate::SSlider>();
        slider->Value = 50;
        root->AddSlot(slider);

        // Color picker
        auto cp = std::make_shared<SColorPicker>();
        cp->Color = ZSlate::Colors::Red;
        root->AddSlot(cp);

        ZSlate::SlateApplication::Get().SetRootContent(root);
    }

    void RunFrame()
    {
        m_Platform.Renderer.BeginFrame();

        ZSlate::UIRect region(0, 0, m_Platform.GetWindowSize().x, m_Platform.GetWindowSize().y);
        ZSlate::SlateApplication::Get().PaintInto(&m_Platform.Renderer, region);

        m_Platform.Renderer.EndFrame();

        // In a real app, upload the batch to GPU and render:
        //   auto& verts   = m_Platform.Renderer.GetVertices();
        //   auto& indices = m_Platform.Renderer.GetIndices();
        //   auto& cmds    = m_Platform.Renderer.GetCommands();
        //   glBufferSubData / vkCmdUpdateBuffer ...
        //   for (auto& cmd : cmds)
        //       glBindTexture(cmd.TextureId); glScissor(cmd.ClipRect); glDrawElements(...);

        PrintBatchStats();
    }

private:
    void PrintBatchStats()
    {
        const auto& cmds = m_Platform.Renderer.GetCommands();
        size_t totalTris = 0;
        for (const auto& c : cmds) totalTris += c.IndexCount / 3;

        printf("[Frame] %zu draw commands, %zu vertices, %zu indices (%zu triangles)\n",
               cmds.size(),
               m_Platform.Renderer.GetVertices().size(),
               m_Platform.Renderer.GetIndices().size(),
               totalTris);
    }
};

// ============================================================================
// Main
// ============================================================================

int main()
{
    printf("=== ZSlate Standalone Application ===\n");
    printf("Renderer: ZSlateBatchedRenderer (CPU batch, ready for GPU upload)\n\n");

    DemoApp app;
    ZSlate::SetPlatform(&app.m_Platform);

    auto measurer = std::make_shared<ZSlate::SlateUIRendererTextMeasurer>(&app.m_Platform.Renderer);
    ZSlate::SlateApplication::Get().SetTextMeasurer(measurer.get());

    app.BuildUI();

    for (int i = 0; i < 3; i++)
    {
        printf("--- Frame %d ---\n", i + 1);
        app.RunFrame();
    }

    printf("\n=== Done ===\n");
    return 0;
}

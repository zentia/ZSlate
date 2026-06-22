// Simple example demonstrating how to use ZSlate standalone
// This is a minimal "hello world" style demo

#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Widgets/Panels/SBorder.h"
#include "ZSlate/Widgets/Layout/SBoxPanel.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Text/STextBlock.h"
#include "ZSlate/Widgets/SWidget.h"

#include <cstdio>

// ============================================================================
// Mock Renderer - Replace with your actual rendering backend
// ============================================================================

struct MockRenderer : public ZSlate::ISlateRenderer
{
    void drawQuad(const ZSlate::UIRect& rect, const ZSlate::UIColor& color) override
    {
        printf("drawQuad: (%.1f, %.1f, %.1f, %.1f) color=(%.2f, %.2f, %.2f, %.2f)\n",
            rect.x, rect.y, rect.w, rect.h,
            color.x, color.y, color.z, color.w);
    }

    void drawRect(const ZSlate::UIRect& rect, const ZSlate::UIColor& color) override
    {
        printf("drawRect: (%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void drawConvexPoly(const std::vector<ZSlate::Vector2>& points, const ZSlate::UIColor& color) override
    {
        printf("drawConvexPoly: %zu points\n", points.size());
    }

    void drawRoundedRect(const ZSlate::UIRect& rect, float radius, const ZSlate::UIColor& color) override
    {
        printf("drawRoundedRect: (%.1f, %.1f, %.1f, %.1f) radius=%.1f\n",
            rect.x, rect.y, rect.w, rect.h, radius);
    }

    void drawTexturedQuad(const ZSlate::UIRect& rect, void* texture_handle, const ZSlate::UIColor& tint = ZSlate::Colors::White) override
    {
        printf("drawTexturedQuad: (%.1f, %.1f, %.1f, %.1f) texture=%p\n",
            rect.x, rect.y, rect.w, rect.h, texture_handle);
    }

    void drawBox(const ZSlate::UIRect& rect, const ZSlate::FMargin& margin, void* texture_handle, const ZSlate::UIColor& tint) override
    {
        printf("drawBox: (%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void drawBorder(const ZSlate::UIRect& rect, const ZSlate::FMargin& margin, void* texture_handle, const ZSlate::UIColor& tint) override
    {
        printf("drawBorder: (%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void drawText(const std::string& text, const ZSlate::Vector2& pos, float font_size, const ZSlate::UIColor& color) override
    {
        printf("drawText: \"%s\" at (%.1f, %.1f) size=%.1f\n",
            text.c_str(), pos.x, pos.y, font_size);
    }

    ZSlate::Vector2 measureText(const std::string& text, float font_size) const override
    {
        // Rough estimate
        return ZSlate::Vector2(text.size() * font_size * 0.6f, font_size * 1.2f);
    }

    void pushClipRect(const ZSlate::UIRect& rect) override
    {
        printf("pushClipRect: (%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void popClipRect() override
    {
        printf("popClipRect\n");
    }

    void beginFrame() override { printf("--- beginFrame ---\n"); }
    void endFrame() override { printf("--- endFrame ---\n"); }
    void flush() override { printf("--- flush ---\n"); }
};

// ============================================================================
// Mock Font Service
// ============================================================================

struct MockFontService : public ZSlate::ISlateFontService
{
    void* loadFont(const std::string& font_path, float size) override
    {
        printf("loadFont: %s size=%.1f\n", font_path.c_str(), size);
        return nullptr;
    }

    void unloadFont(void* handle) override
    {
        printf("unloadFont: %p\n", handle);
    }

    ZSlate::Vector2 measureText(void* font_handle, const std::string& text) const override
    {
        return ZSlate::Vector2(text.size() * 10.0f, 14.0f);
    }

    void* getDefaultFont() const override { return nullptr; }
};

// ============================================================================
// Mock Platform
// ============================================================================

struct MockPlatform : public ZSlate::ISlatePlatform
{
    MockRenderer Renderer;
    MockFontService FontService;

    ZSlate::ISlateRenderer* getRenderer() override { return &Renderer; }
    ZSlate::ISlateFontService* getFontService() override { return &FontService; }
    ZSlate::Vector2 getMousePosition() const override { return ZSlate::Vector2(100.0f, 100.0f); }
    bool isMouseButtonDown(int button) const override { return false; }
    bool isKeyDown(int key) const override { return false; }
    float getTimeSeconds() const override { return 0.0f; }
    ZSlate::Vector2 getWindowSize() const override { return ZSlate::Vector2(800.0f, 600.0f); }
};

// ============================================================================
// Simple Demo Widget
// ============================================================================

class SDemoButton : public ZSlate::SLeafWidget
{
public:
    std::string Label;
    std::function<void()> OnClick;

    Vector2 ComputeDesiredSize() const override
    {
        if (ZSlate::GSlateTextMeasurer)
            return ZSlate::GSlateTextMeasurer->Measure(Label, 14.0f);
        return Vector2(100.0f, 30.0f);
    }

    void OnPaint(const ZSlate::FPaintContext& ctx, const ZSlate::FGeometry& geom) const override
    {
        if (ctx.Renderer == nullptr) return;
        
        // Draw button background
        ctx.Renderer->drawQuad(geom.ToRect(), ZSlate::Colors::Blue);
        
        // Draw text
        ctx.Renderer->drawText(Label, geom.AbsolutePosition, 14.0f, ZSlate::Colors::White);
    }

    ZSlate::ECursorType GetCursor() const override { return ZSlate::ECursorType::Hand; }

    ZSlate::FReply OnMouseButtonDown(const ZSlate::Vector2& pos, int button) override
    {
        if (button == 0) // Left mouse button
        {
            if (OnClick) OnClick();
            return ZSlate::FReply::Handled();
        }
        return ZSlate::FReply::Unhandled();
    }
};

// ============================================================================
// Main
// ============================================================================

int main()
{
    // 1. Create platform and set it
    MockPlatform platform;
    ZSlate::SetPlatform(&platform);

    // 2. Create text measurer
    auto measurer = std::make_shared<ZSlate::SlateUIRendererTextMeasurer>(&platform.Renderer);
    ZSlate::SlateApplication::Get().SetTextMeasurer(measurer.get());

    // 3. Build widget tree
    auto root = std::make_shared<ZSlate::SVerticalBox>();
    
    // Add some padding
    auto title = std::make_shared<ZSlate::STextBlock>();
    title->Text = "ZSlate Hello World!";
    title->FontSize = 24.0f;
    root->AddSlot(title);

    auto spacer = std::make_shared<ZSlate::SSpacer>();
    spacer->Size = ZSlate::Vector2(0.0f, 20.0f);
    root->AddSlot(spacer);

    auto button = std::make_shared<SDemoButton>();
    button->Label = "Click Me";
    button->OnClick = []() { printf("Button clicked!\n"); };
    root->AddSlot(button);

    // 4. Set root content
    ZSlate::SlateApplication::Get().SetRootContent(root);

    // 5. Paint a frame
    platform.Renderer.beginFrame();
    ZSlate::UIRect region(0, 0, 800, 600);
    ZSlate::SlateApplication::Get().PaintInto(&platform.Renderer, region);
    platform.Renderer.endFrame();
    platform.Renderer.flush();

    return 0;
}

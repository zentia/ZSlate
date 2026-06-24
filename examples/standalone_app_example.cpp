// ============================================================================
// ZSlate Standalone Application Example
// ============================================================================
// This example shows how to build a complete UI application using ZSlate
// with a custom rendering backend.
//
// For a real application, you would replace MockRenderer with your actual
// rendering backend (OpenGL, Vulkan, DirectX, etc.).
// ============================================================================

#include "ZSlate/Application/SlateApplication.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlatePaint.h"
#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Core/SlateClipboard.h"
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
// Mock Renderer Implementation
// Replace this with your actual rendering backend
// ============================================================================

struct MockRenderer : public ZSlate::ISlateRenderer
{
    int m_DrawCallCount {0};
    
    void DrawQuad(const ZSlate::UIRect& rect, const ZSlate::UIColor& color) override
    {
        m_DrawCallCount++;
        // In a real implementation, you would:
        // 1. Create a quad vertex buffer
        // 2. Set the color
        // 3. Submit to your render queue
        printf("[DrawQuad] rect=(%.1f, %.1f, %.1f, %.1f) color=(%.2f, %.2f, %.2f, %.2f)\n",
            rect.x, rect.y, rect.w, rect.h,
            color.x, color.y, color.z, color.w);
    }

    void DrawRect(const ZSlate::UIRect& rect, const ZSlate::UIColor& color, float thickness = 1.0f) override
    {
        m_DrawCallCount++;
        printf("[DrawRect] rect=(%.1f, %.1f, %.1f, %.1f) thickness=%.1f\n",
            rect.x, rect.y, rect.w, rect.h, thickness);
    }

    void DrawConvexPoly(const ZSlate::Vector2* points, int count, const ZSlate::UIColor& color) override
    {
        m_DrawCallCount++;
        printf("[DrawConvexPoly] %d points\n", count);
    }

    void DrawRoundedRect(const ZSlate::UIRect& rect, float radius, const ZSlate::UIColor& color) override
    {
        m_DrawCallCount++;
        printf("[DrawRoundedRect] rect=(%.1f, %.1f, %.1f, %.1f) radius=%.1f\n",
            rect.x, rect.y, rect.w, rect.h, radius);
    }

    void DrawTexturedQuad(const ZSlate::UIRect& rect, void* texture_handle, const ZSlate::UIColor& tint = ZSlate::Colors::White) override
    {
        m_DrawCallCount++;
        printf("[DrawTexturedQuad] rect=(%.1f, %.1f, %.1f, %.1f) texture=%p\n",
            rect.x, rect.y, rect.w, rect.h, texture_handle);
    }

    void DrawBox(const ZSlate::UIRect& rect, const ZSlate::FMargin& margin, void* texture_handle, const ZSlate::UIColor& tint) override
    {
        m_DrawCallCount++;
        printf("[DrawBox] rect=(%.1f, %.1f, %.1f, %.1f) margin=(%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h,
            margin.left, margin.top, margin.right, margin.bottom);
    }

    void DrawBorder(const ZSlate::UIRect& rect, const ZSlate::FMargin& margin, void* texture_handle, const ZSlate::UIColor& tint) override
    {
        m_DrawCallCount++;
        printf("[DrawBorder] rect=(%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void DrawText(const ZSlate::UIRect& rect, const std::string& text, float font_size, const ZSlate::UIColor& color,
                  ZSlate::TextAnchor alignment = ZSlate::TextAnchor::MiddleLeft,
                  ZSlate::TextWrapMode wrap = ZSlate::TextWrapMode::NoWrap,
                  void* font_handle = nullptr) override
    {
        m_DrawCallCount++;
        printf("[DrawText] \"%s\" at rect=(%.1f, %.1f, %.1f, %.1f) size=%.1f alignment=%d\n",
            text.c_str(), rect.x, rect.y, rect.w, rect.h, font_size, static_cast<int>(alignment));
    }

    void DrawText(const std::string& text, const ZSlate::Vector2& pos, float font_size, const ZSlate::UIColor& color) override
    {
        m_DrawCallCount++;
        printf("[DrawText] \"%s\" at (%.1f, %.1f) size=%.1f\n",
            text.c_str(), pos.x, pos.y, font_size);
    }

    ZSlate::Vector2 MeasureText(const std::string& text, float font_size) const override
    {
        // Rough estimate based on average character width
        // In a real implementation, query your font atlas
        float avg_char_width = font_size * 0.6f;
        return ZSlate::Vector2(text.size() * avg_char_width, font_size * 1.2f);
    }

    void PushClipRect(const ZSlate::UIRect& rect) override
    {
        printf("[PushClipRect] (%.1f, %.1f, %.1f, %.1f)\n",
            rect.x, rect.y, rect.w, rect.h);
    }

    void PopClipRect() override
    {
        printf("[PopClipRect]\n");
    }

    void BeginFrame() override 
    { 
        m_DrawCallCount = 0;
        printf("========== Frame Begin ==========\n");
    }
    
    void EndFrame() override 
    { 
        printf("========== Frame End ==========\n");
    }
    
    void Flush() override 
    { 
        printf("[Flush] Total draw calls: %d\n", m_DrawCallCount);
    }
};

// ============================================================================
// Mock Font Service
// ============================================================================

struct MockFontService : public ZSlate::ISlateFontService
{
    std::unordered_map<std::string, void*> m_FontCache;

    void* loadFont(const std::string& font_path, float size) override
    {
        printf("[loadFont] %s size=%.1f\n", font_path.c_str(), size);
        // In a real implementation, load the font from disk and create a texture atlas
        void* handle = reinterpret_cast<void*>(font_path.size() + static_cast<size_t>(size));
        m_FontCache[font_path + std::to_string(static_cast<int>(size))] = handle;
        return handle;
    }

    void unloadFont(void* handle) override
    {
        printf("[unloadFont] %p\n", handle);
        // In a real implementation, release the font resources
    }

    ZSlate::Vector2 MeasureText(void* font_handle, const std::string& text) const override
    {
        float avg_char_width = 10.0f; // Placeholder
        return ZSlate::Vector2(text.size() * avg_char_width, 14.0f);
    }

    void* getDefaultFont() const override 
    { 
        return nullptr;  // Mock: no default font
    }
};

// ============================================================================
// Mock Platform
// ============================================================================

class MockPlatform : public ZSlate::ISlatePlatform
{
public:
    MockRenderer Renderer;
    MockFontService FontService;
    
    bool m_MouseButtonDown {false};
    bool m_KeyboardDown {false};
    ZSlate::Vector2 m_MousePosition {400.0f, 300.0f};
    float m_TimeSeconds {0.0f};
    ZSlate::Vector2 m_WindowSize {800.0f, 600.0f};

    ZSlate::ISlateRenderer* getRenderer() override { return &Renderer; }
    ZSlate::ISlateFontService* getFontService() override { return &FontService; }
    ZSlate::Vector2 getMousePosition() const override { return m_MousePosition; }
    bool isMouseButtonDown(int button) const override { return m_MouseButtonDown; }
    bool isKeyDown(int key) const override { return m_KeyboardDown; }
    float getTimeSeconds() const override { return m_TimeSeconds; }
    ZSlate::Vector2 getWindowSize() const override { return m_WindowSize; }
};

// ============================================================================
// Custom Widget Example: Color Picker
// ============================================================================

class SColorPicker : public ZSlate::SLeafWidget
{
public:
    ZSlate::UIColor Color {ZSlate::Colors::White};
    std::function<void(const ZSlate::UIColor&)> OnColorChanged;

    ZSlate::Vector2 ComputeDesiredSize() const override
    {
        return ZSlate::Vector2(80.0f, 80.0f);
    }

    void OnPaint(const ZSlate::FPaintContext& ctx, const ZSlate::FGeometry& geom) const override
    {
        if (ctx.Renderer == nullptr) return;
        
        // Draw color preview
        ctx.Renderer->DrawQuad(geom.ToRect(), Color);
        
        // Draw border
        ctx.Renderer->DrawRect(geom.ToRect(), ZSlate::Colors::Black, 1.0f);
    }

    ZSlate::ECursorType GetCursor() const override 
    { 
        return ZSlate::ECursorType::Hand; 
    }

    ZSlate::FReply OnMouseButtonDown(const ZSlate::Vector2& pos, int button) override
    {
        if (button == 0) // Left mouse button
        {
            if (OnColorChanged) 
            {
                OnColorChanged(Color);
            }
            return ZSlate::FReply::Handled();
        }
        return ZSlate::FReply::Unhandled();
    }
};

// ============================================================================
// Custom Widget Example: Labeled Text Field
// ============================================================================

class SLabelledField : public ZSlate::SCompoundWidget
{
public:
    std::string Label;
    std::shared_ptr<ZSlate::SEditableTextBox> TextField;

    SLabelledField()
    {
        TextField = std::make_shared<ZSlate::SEditableTextBox>();
    }

    void Construct(const ZSlate::FArguments& args) override
    {
        SCompoundWidget::Construct(args);
        
        // Create horizontal layout
        auto hbox = std::make_shared<ZSlate::SHorizontalBox>();
        
        // Add label
        auto label = std::make_shared<ZSlate::STextBlock>();
        label->Text = Label + ": ";
        label->FontSize = 14.0f;
        hbox->AddSlot(label);
        
        // Add spacer
        auto spacer = std::make_shared<ZSlate::SSpacer>();
        spacer->Size = ZSlate::Vector2(8.0f, 1.0f);
        hbox->AddSlot(spacer);
        
        // Add text field
        hbox->AddSlot(TextField);
        
        // Set content
        SetContent(hbox);
    }
};

// ============================================================================
// Demo Application Builder
// ============================================================================

class DemoApp
{
public:
    MockPlatform m_Platform;
    
    void BuildUI()
    {
        // Create root vertical box
        auto root = std::make_shared<ZSlate::SVerticalBox>();
        
        // Title
        auto title = std::make_shared<ZSlate::STextBlock>();
        title->Text = "ZSlate Standalone Application Demo";
        title->FontSize = 24.0f;
        title->Color = ZSlate::Colors::White;
        root->AddSlot(title);
        
        // Spacer
        auto spacer = std::make_shared<ZSlate::SSpacer>();
        spacer->Size = ZSlate::Vector2(0.0f, 16.0f);
        root->AddSlot(spacer);
        
        // Buttons section
        auto buttonsTitle = std::make_shared<ZSlate::STextBlock>();
        buttonsTitle->Text = "Buttons:";
        buttonsTitle->FontSize = 16.0f;
        buttonsTitle->Color = ZSlate::Colors::Yellow;
        root->AddSlot(buttonsTitle);
        
        auto button1 = std::make_shared<ZSlate::SButton>();
        button1->SetText("Primary Button");
        button1->SetForegroundColor(ZSlate::Colors::White);
        button1->SetBackgroundColor(ZSlate::Colors::Blue);
        button1->OnClicked.AddLambda([](const ZSlate::FArguments&) {
            printf("Primary button clicked!\n");
        });
        root->AddSlot(button1);
        
        auto button2 = std::make_shared<ZSlate::SButton>();
        button2->SetText("Secondary Button");
        button2->SetForegroundColor(ZSlate::Colors::Black);
        button2->SetBackgroundColor(ZSlate::Colors::Gray);
        button2->OnClicked.AddLambda([](const ZSlate::FArguments&) {
            printf("Secondary button clicked!\n");
        });
        root->AddSlot(button2);
        
        // Checkbox section
        auto checkboxTitle = std::make_shared<ZSlate::STextBlock>();
        checkboxTitle->Text = "Checkboxes:";
        checkboxTitle->FontSize = 16.0f;
        checkboxTitle->Color = ZSlate::Colors::Yellow;
        root->AddSlot(checkboxTitle);
        
        auto checkbox1 = std::make_shared<ZSlate::SCheckBox>();
        checkbox1->SetLabel("Option A");
        checkbox1->SetIsChecked(true);
        root->AddSlot(checkbox1);
        
        auto checkbox2 = std::make_shared<ZSlate::SCheckBox>();
        checkbox2->SetLabel("Option B");
        checkbox2->SetIsChecked(false);
        root->AddSlot(checkbox2);
        
        // Slider section
        auto sliderTitle = std::make_shared<ZSlate::STextBlock>();
        sliderTitle->Text = "Sliders:";
        sliderTitle->FontSize = 16.0f;
        sliderTitle->Color = ZSlate::Colors::Yellow;
        root->AddSlot(sliderTitle);
        
        auto slider = std::make_shared<ZSlate::SSlider>();
        slider->SetMinValue(0.0f);
        slider->SetMaxValue(100.0f);
        slider->SetValue(50.0f);
        root->AddSlot(slider);
        
        // Color picker
        auto colorTitle = std::make_shared<ZSlate::STextBlock>();
        colorTitle->Text = "Color Picker:";
        colorTitle->FontSize = 16.0f;
        colorTitle->Color = ZSlate::Colors::Yellow;
        root->AddSlot(colorTitle);
        
        auto colorPicker = std::make_shared<SColorPicker>();
        colorPicker->Color = ZSlate::Colors::Red;
        colorPicker->OnColorChanged = [](const ZSlate::UIColor& color) {
            printf("Color changed: (%.2f, %.2f, %.2f, %.2f)\n",
                color.x, color.y, color.z, color.w);
        };
        root->AddSlot(colorPicker);
        
        // Scroll box with many items
        auto scrollTitle = std::make_shared<ZSlate::STextBlock>();
        scrollTitle->Text = "Scroll Box (100 items):";
        scrollTitle->FontSize = 16.0f;
        scrollTitle->Color = ZSlate::Colors::Yellow;
        root->AddSlot(scrollTitle);
        
        auto scrollBox = std::make_shared<ZSlate::SScrollBox>();
        scrollBox->SetContentSize(ZSlate::Vector2(0.0f, 500.0f));
        
        auto scrollContent = std::make_shared<ZSlate::SVerticalBox>();
        for (int i = 0; i < 100; i++)
        {
            auto item = std::make_shared<ZSlate::STextBlock>();
            item->Text = "Item " + std::to_string(i);
            item->FontSize = 12.0f;
            scrollContent->AddSlot(item);
            
            auto itemSpacer = std::make_shared<ZSlate::SSpacer>();
            itemSpacer->Size = ZSlate::Vector2(0.0f, 4.0f);
            scrollContent->AddSlot(itemSpacer);
        }
        scrollBox->SetContent(scrollContent);
        root->AddSlot(scrollBox);
        
        // Set root content
        ZSlate::SlateApplication::Get().SetRootContent(root);
    }
    
    void RunFrame()
    {
        // Begin frame
        m_Platform.Renderer.BeginFrame();
        
        // Paint UI
        ZSlate::UIRect region(0, 0, m_Platform.getWindowSize().x, m_Platform.getWindowSize().y);
        ZSlate::SlateApplication::Get().PaintInto(&m_Platform.Renderer, region);
        
        // End frame
        m_Platform.Renderer.EndFrame();
        m_Platform.Renderer.Flush();
    }
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main()
{
    printf("=== ZSlate Standalone Application ===\n\n");
    
    // 1. Create platform implementation
    DemoApp app;
    
    // 2. Set platform globally
    ZSlate::SetPlatform(&app.m_Platform);
    
    // 3. Create text measurer
    auto measurer = std::make_shared<ZSlate::SlateUIRendererTextMeasurer>(&app.m_Platform.Renderer);
    ZSlate::SlateApplication::Get().SetTextMeasurer(measurer.get());
    
    // 4. Build UI
    app.BuildUI();
    
    // 5. Run a few frames to demonstrate
    printf("\n--- Frame 1 ---\n");
    app.RunFrame();
    
    printf("\n--- Frame 2 ---\n");
    app.RunFrame();
    
    printf("\n--- Frame 3 ---\n");
    app.RunFrame();
    
    printf("\n=== Application Exited ===\n");
    
    return 0;
}

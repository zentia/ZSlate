#pragma once

// STextBlock: displays a single line of text.
// UE analogue: Widgets/Text/STextBlock.h
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Widgets/SWidget.h"

#include <string>

namespace ZSlate
{

class STextBlock : public SWidget
{
public:
    STextBlock() = default;
    virtual ~STextBlock() = default;

    // Public members accessed by UMG UTextBlock.h and SRCachedRun.
    // Alignment is TextAnchor (same type as in UE Slate), NOT EHorizontalAlignment.
    std::string Text;
    UIColor     Color {0.9f, 0.9f, 0.9f, 1.0f};
    float       FontSize {14.0f};
    TextAnchor  Alignment {TextAnchor::MiddleLeft};

    void SetText(const std::string& t) { Text = t; }
    const std::string& GetText() const { return Text; }
    void SetFont(void*) {}
    void SetColor(const UIColor& c) { Color = c; }
    void SetFontSize(float s) { FontSize = s; }

    Vector2 ComputeDesiredSize() const override
    {
        return Vector2(static_cast<float>(Text.length()) * FontSize * 0.6f, FontSize + 4.0f);
    }

    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer && !Text.empty())
        {
            ctx.Renderer->DrawText(geom.ToRect(), Text, FontSize, Color, Alignment, TextWrapMode::NoWrap);
        }
    }
};

}  // namespace ZSlate

#pragma once

// Minimal STextBlock stub — ZEditor build dependency.
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Widgets/SWidget.h"

namespace ZSlate
{

// STextBlock: displays a single line of text.
class STextBlock : public SWidget
{
public:
    STextBlock() = default;
    virtual ~STextBlock() = default;

    void SetText(const std::string& t) { Text = t; }
    const std::string& GetText() const { return Text; }
    void SetFont(void*) {}
    void SetColor(const UIColor& c) { Color = c; }
    void SetFontSize(float s) { FontSize = s; }

    // Public members accessed by SRCachedRun::OnPaint (SRichTextBlock.cpp).
    std::string Text;
    UIColor     Color {0.9f, 0.9f, 0.9f, 1.0f};
    float       FontSize {14.0f};
    EHorizontalAlignment Alignment {EHorizontalAlignment::Left};  // accessed by SMenu.cpp

    Vector2 ComputeDesiredSize() const override
    {
        // Stub: return a fixed size
        return Vector2(static_cast<float>(Text.length()) * FontSize * 0.6f, FontSize + 4.0f);
    }

    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer && !Text.empty())
        {
            ctx.Renderer->drawText(geom.ToRect(), Text, FontSize, Color,
                                   TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
        }
    }
};

}  // namespace ZSlate

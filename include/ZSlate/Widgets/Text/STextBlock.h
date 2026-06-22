#pragma once

#include "ZSlate/Widgets/SLeafWidget.h"
#include "ZSlate/Core/ZSlateTypes.h"

namespace ZSlate
{
// A text block widget (minimal implementation).
// Similar to UE's STextBlock.
class STextBlock : public SLeafWidget
{
public:
    std::string Text;
    float FontSize {16.0f};
    UIColor Color {1.0f, 1.0f, 1.0f, 1.0f};
    TextAnchor Alignment {TextAnchor::MiddleLeft};

    void SetText(const std::string& text) { Text = text; }
    void SetColor(const UIColor& color) { Color = color; }

    Vector2 ComputeDesiredSize() const override
    {
        // Approximate size - in a real implementation, this would use the text measurer
        return Vector2(Text.length() * FontSize * 0.6f, FontSize * 1.2f);
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer == nullptr || Text.empty())
            return;

        UIRect rect = geom.ToRect();
        ctx.Renderer->drawText(rect, Text, FontSize, Color,
                               Alignment,
                               TextWrapMode::NoWrap,
                               nullptr);
    }
};
}  // namespace ZSlate

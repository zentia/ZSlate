#pragma once

#include "ZSlate/Widgets/SCompoundWidget.h"

namespace ZSlate
{
// A filled background panel that wraps one content child (UE Slate SBorder
// analogue). Paints its background quad, then the content paints on top.
class SBorder : public SCompoundWidget
{
public:
    UIColor BackgroundColor {0.12f, 0.12f, 0.12f, 1.0f};
    bool DrawBackground {true};

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer == nullptr)
            return;
        if (DrawBackground)
            ctx.Renderer->drawQuad(geom.ToRect(), BackgroundColor);
    }
};
}  // namespace ZSlate

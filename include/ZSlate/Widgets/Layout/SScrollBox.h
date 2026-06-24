#pragma once

#include "ZSlate/Widgets/SCompoundWidget.h"

namespace ZSlate
{
// A scroll box widget (minimal implementation).
// Similar to UE's SScrollBox.
class SScrollBox : public SCompoundWidget
{
public:
    EOrientation Orientation {EOrientation::Vertical};
    float ScrollOffset {0.0f};
    float ScrollSpeed {40.0f};

    void AddChild(std::shared_ptr<SWidget> child)
    {
        if (child)
            m_Child = child;
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (m_Child && ctx.Renderer)
        {
            ctx.Renderer->pushClipRect(geom.ToRect());
            m_Child->Paint(ctx, geom);
            ctx.Renderer->popClipRect();
        }
    }
};
}  // namespace ZSlate

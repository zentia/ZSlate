#pragma once

#include "ZSlate/Widgets/SWidget.h"
#include "ZSlate/Core/SlateEnums.h"

namespace ZSlate
{

// Vertical or horizontal scroll bar (UE SScrollBar analogue).
//
// Reports its value as a fraction 0..1. The host (SScrollBox, SListView)
// reads m_Value each frame and translates it to a pixel offset.
//
// Thumb size is proportional to visible/total ratio; dragging the thumb
// updates m_Value continuously via CaptureMouse.
class SScrollBar : public SWidget
{
public:
    EOrientation Orientation {EOrientation::Vertical};

    // Fraction of the content that is currently visible (0..1).
    // 0.1 = 10% visible → small thumb. 1.0 = all visible → no thumb.
    float ViewFraction {1.0f};

    // Scroll offset fraction (0..1). 0 = top/left, 1 = bottom/right.
    mutable float Value {0.0f};

    // Click step on track (empty space above/below thumb). Default scrolls ~1 page.
    float TrackClickStep {0.1f};

    Vector2 ComputeDesiredSize() const override
    {
        return Orientation == EOrientation::Vertical
            ? Vector2(12.0f, 100.0f)
            : Vector2(100.0f, 12.0f);
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (!ctx.Renderer) return;
        const UIRect r = geom.ToRect();

        // Track background
        ctx.Renderer->DrawQuad(r, UIColor(0.12f, 0.12f, 0.14f, 1.0f));

        // Thumb
        float thumbFrac = std::max(0.05f, ViewFraction);
        float trackLen = (Orientation == EOrientation::Vertical) ? r.h : r.w;
        float thumbLen = std::max(12.0f, trackLen * thumbFrac);
        float travel = trackLen - thumbLen;
        float offset = (travel > 0.0f) ? Value * travel : 0.0f;

        UIRect thumbRect;
        if (Orientation == EOrientation::Vertical)
            thumbRect = UIRect(r.x, r.y + offset, r.w, thumbLen);
        else
            thumbRect = UIRect(r.x + offset, r.y, thumbLen, r.h);

        UIColor thumbCol = m_Dragging ? UIColor(0.42f, 0.45f, 0.52f, 1.0f)
                         : m_Hovered  ? UIColor(0.35f, 0.38f, 0.45f, 1.0f)
                         :              UIColor(0.28f, 0.30f, 0.36f, 1.0f);
        ctx.Renderer->DrawQuad(thumbRect, thumbCol);
    }

    // --- Input -----------------------------------------------------------

    void OnMouseEnter() override { m_Hovered = true; }
    void OnMouseLeave() override { m_Hovered = false; m_Dragging = false; }

    FReply OnMouseButtonDown(const Vector2& pos, int btn) override
    {
        if (btn != 0) return FReply::Unhandled();

        const UIRect r = GetCachedGeometry().ToRect();
        float trackLen = (Orientation == EOrientation::Vertical) ? r.h : r.w;
        float thumbLen = std::max(12.0f, trackLen * std::max(0.05f, ViewFraction));
        float travel = trackLen - thumbLen;
        if (travel <= 0.0f) return FReply::Unhandled();

        float thumbStart = Value * travel;
        float mouseCoord = (Orientation == EOrientation::Vertical)
            ? pos.y - r.y : pos.x - r.x;

        if (mouseCoord >= thumbStart && mouseCoord <= thumbStart + thumbLen)
        {
            m_Dragging = true;
            m_DragOffset = mouseCoord - thumbStart;
            return FReply::Handled().CaptureMouse(const_cast<SScrollBar*>(this));
        }

        float dir = (mouseCoord < thumbStart) ? -1.0f : 1.0f;
        Value = std::clamp(Value + dir * TrackClickStep, 0.0f, 1.0f);
        return FReply::Handled();
    }

    void OnMouseMove(const Vector2& pos) override
    {
        if (!m_Dragging) return;

        const UIRect r = GetCachedGeometry().ToRect();
        float trackLen = (Orientation == EOrientation::Vertical) ? r.h : r.w;
        float thumbLen = std::max(12.0f, trackLen * std::max(0.05f, ViewFraction));
        float travel = trackLen - thumbLen;
        if (travel <= 0.0f) return;

        float mouseCoord = (Orientation == EOrientation::Vertical)
            ? pos.y - r.y : pos.x - r.x;
        float newThumbStart = mouseCoord - m_DragOffset;
        Value = std::clamp(newThumbStart / travel, 0.0f, 1.0f);
    }

    FReply OnMouseButtonUp(const Vector2&, int btn) override
    {
        if (btn != 0 || !m_Dragging) return FReply::Unhandled();
        m_Dragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }

    void OnMouseCaptureLost() override { m_Dragging = false; }

    FReply OnMouseWheel(const Vector2&, float delta) override
    {
        float step = 1.0f / std::max(1.0f, 1.0f / std::max(0.01f, TrackClickStep));
        Value = std::clamp(Value - delta / step, 0.0f, 1.0f);
        return FReply::Handled();
    }

private:
    mutable bool m_Hovered  {false};
    mutable bool m_Dragging {false};
    float m_DragOffset {0.0f};
};

}  // namespace ZSlate

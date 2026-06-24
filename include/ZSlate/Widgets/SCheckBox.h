#pragma once

// Minimal SCheckBox stub — ZEditor build dependency.
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Widgets/SCompoundWidget.h"

namespace ZSlate
{

// SCheckBox: a checkable toggle box with a label.
class SCheckBox : public SCompoundWidget
{
public:
    SCheckBox() = default;
    virtual ~SCheckBox() = default;

    void SetChecked(bool c) { m_Checked = c; }
    bool IsChecked() const { return m_Checked; }
    void SetLabel(const std::string& l) { m_Label = l; }
    void SetOnCheckStateChanged(std::function<void(bool)> fn) { m_OnChanged = fn; }

    Vector2 ComputeDesiredSize() const override { return Vector2(200.0f, 24.0f); }
    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer)
        {
            const UIRect rect = geom.ToRect();
            ctx.Renderer->drawQuad(rect, UIColor(0.12f, 0.12f, 0.14f, 1.0f));
            // Draw check box + label
            const float box_sz = 14.0f;
            const UIRect box_rect(rect.x + 4.0f, rect.y + (rect.h - box_sz) * 0.5f, box_sz, box_sz);
            ctx.Renderer->drawQuad(box_rect, m_Checked ? UIColor(0.30f, 0.55f, 0.95f, 1.0f) : UIColor(0.20f, 0.20f, 0.22f, 1.0f));
            if (!m_Label.empty())
            {
                const UIRect label_rect(rect.x + box_sz + 10.0f, rect.y, rect.w - box_sz - 14.0f, rect.h);
                ctx.Renderer->drawText(label_rect, m_Label, 13.0f, UIColor(0.88f, 0.89f, 0.92f, 1.0f),
                                       TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
            }
        }
    }

private:
    bool m_Checked {false};
    std::string m_Label;
    std::function<void(bool)> m_OnChanged;
};

}  // namespace ZSlate

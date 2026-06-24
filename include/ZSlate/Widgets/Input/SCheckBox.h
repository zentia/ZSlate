#pragma once

// SCheckBox: a checkable toggle box with a label.
// UE analogue: Widgets/Input/SCheckBox.h
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Widgets/SCompoundWidget.h"

#include <functional>
#include <string>

namespace ZSlate
{

class SCheckBox : public SCompoundWidget
{
public:
    SCheckBox() = default;
    virtual ~SCheckBox() = default;

    // Public members accessed by UMG UCheckBox.h
    bool Checked {false};
    float BoxSize {18.0f};
    std::function<void(bool)> OnCheckStateChanged;
    std::string Label;

    void SetChecked(bool c) { Checked = c; }
    bool IsChecked() const { return Checked; }
    void SetLabel(const std::string& l) { Label = l; }

    Vector2 ComputeDesiredSize() const override { return Vector2(200.0f, 24.0f); }
    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer)
        {
            const UIRect rect = geom.ToRect();
            ctx.Renderer->DrawQuad(rect, UIColor(0.12f, 0.12f, 0.14f, 1.0f));
            const float box_sz = BoxSize;
            const UIRect box_rect(rect.x + 4.0f, rect.y + (rect.h - box_sz) * 0.5f, box_sz, box_sz);
            ctx.Renderer->DrawQuad(box_rect, Checked ? UIColor(0.30f, 0.55f, 0.95f, 1.0f) : UIColor(0.20f, 0.20f, 0.22f, 1.0f));
            if (!Label.empty())
            {
                const UIRect label_rect(rect.x + box_sz + 10.0f, rect.y, rect.w - box_sz - 14.0f, rect.h);
                ctx.Renderer->DrawText(label_rect, Label, 13.0f, UIColor(0.88f, 0.89f, 0.92f, 1.0f),
                                       TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
            }
        }
    }
};

}  // namespace ZSlate

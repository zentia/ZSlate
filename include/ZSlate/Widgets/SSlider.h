#pragma once
// Minimal SSlider stub — ZEditor build dependency.
#include "ZSlate/Widgets/SWidget.h"
#include <functional>
namespace ZSlate {
class SSlider : public SWidget {
public:
    SSlider() = default;
    float Value {0.5f};
    float MinValue {0.0f};
    float MaxValue {1.0f};
    float Height {22.0f};
    float HandleWidth {8.0f};
    float MinDesiredWidth {200.0f};
    std::function<void(float)> OnValueChanged;
    ZSlate::FMargin Padding;
    Vector2 ComputeDesiredSize() const override { return Vector2(MinDesiredWidth, Height); }
    void ArrangeChildren(const FGeometry& g, std::vector<FArrangedWidget>& o) const override { (void)g;(void)o; }
    void OnPaint(const FPaintContext& c, const FGeometry& g) const override {
        if (c.Renderer) {
            const UIRect r = g.ToRect();
            c.Renderer->drawQuad(r, UIColor(0.12f,0.12f,0.14f,1.0f));
            float t = (MaxValue > MinValue) ? (Value - MinValue) / (MaxValue - MinValue) : 0.5f;
            float knob = t * (r.w - HandleWidth) + r.x;
            c.Renderer->drawQuad(UIRect(knob, r.y + 2.0f, HandleWidth, r.h - 4.0f), UIColor(0.30f,0.55f,0.95f,1.0f));
        }
    }
};
}

#pragma once
// Minimal SColorPicker stub — ZEditor build dependency.
#include "ZSlate/Widgets/SCompoundWidget.h"
namespace ZSlate {
class SColorPicker : public SCompoundWidget {
public:
    SColorPicker() = default;
    Vector2 ComputeDesiredSize() const override { return Vector2(260.0f, 220.0f); }
    void ArrangeChildren(const FGeometry& g, std::vector<FArrangedWidget>& o) const override { (void)g;(void)o; }
    void OnPaint(const FPaintContext& c, const FGeometry& g) const override { (void)c;(void)g; }
};
}

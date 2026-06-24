#pragma once
// Minimal SExpandableArea stub — ZEditor build dependency.
#include "ZSlate/Widgets/SCompoundWidget.h"
namespace ZSlate {
class SExpandableArea : public SCompoundWidget {
public:
    SExpandableArea() = default;
    bool IsExpanded() const { return true; }
    Vector2 ComputeDesiredSize() const override { return Vector2(200.0f, 28.0f); }
    void ArrangeChildren(const FGeometry& g, std::vector<FArrangedWidget>& o) const override { (void)g;(void)o; }
    void OnPaint(const FPaintContext& c, const FGeometry& g) const override { (void)c;(void)g; }
};
}

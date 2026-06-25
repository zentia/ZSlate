#pragma once
// Minimal SImage stub — ZEditor build dependency.
#include "ZSlate/Widgets/SWidget.h"
namespace ZSlate {
class SImage : public SWidget {
public:
    SImage() = default;
    void* TextureHandle {nullptr};
    Vector2 ComputeDesiredSize() const override { return Vector2(64.0f, 64.0f); }
    void ArrangeChildren(const FGeometry& g, std::vector<FArrangedWidget>& o) const override { (void)g;(void)o; }
    void OnPaint(const FPaintContext& c, const FGeometry& g) const override;
};
}

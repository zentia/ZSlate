#pragma once

#include "ZSlate/Widgets/SLeafWidget.h"
#include "ZSlate/Core/ZSlateTypes.h"

namespace ZSlate
{
// A spacer widget (empty layout filler with fixed size).
// Similar to UE's SSpacer.
class SSpacer : public SLeafWidget
{
public:
    Vector2 Size {0.0f, 0.0f};

    SSpacer() = default;
    explicit SSpacer(const Vector2& size) : Size(size) {}

    Vector2 ComputeDesiredSize() const override
    {
        return Size;
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        // Spacer is invisible - does not paint anything
        (void)ctx;
        (void)geom;
    }
};
}  // namespace ZSlate

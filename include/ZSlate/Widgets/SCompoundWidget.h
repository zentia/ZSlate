#pragma once

// Minimal SCompoundWidget stub — ZEditor build dependency.
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Widgets/SWidget.h"

namespace ZSlate
{

// SCompoundWidget: base class for widgets that own one child.
class SCompoundWidget : public SWidget
{
public:
    std::shared_ptr<SWidget> m_Child;

    // Layout properties accessed by SMenuItem, SMenu, etc.
    FMargin   Padding;
    EHorizontalAlignment HAlign {EHorizontalAlignment::Fill};
    EVerticalAlignment VAlign {EVerticalAlignment::Center};

    void SetContent(std::shared_ptr<SWidget> w) { m_Child = w; }

    Vector2 ComputeDesiredSize() const override
    {
        if (m_Child) return m_Child->ComputeDesiredSize();
        return Vector2(0.0f, 0.0f);
    }

    void ArrangeChildren(const FGeometry& allotted, std::vector<FArrangedWidget>& out) const override
    {
        if (m_Child)
            out.push_back(FArrangedWidget{m_Child, allotted});
    }
};

}  // namespace ZSlate

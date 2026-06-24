#pragma once

// Minimal SBorder stub — ZEditor build dependency.
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Widgets/SCompoundWidget.h"

namespace ZSlate
{

class SBorder : public SCompoundWidget
{
public:
    SBorder() = default;
    virtual ~SBorder() = default;

    void SetBorder(const FMargin& m) { (void)m; }
    void SetPadding(const FMargin& p) { Padding = p; }
    void SetBorderColor(const UIColor& c) { BorderColor = c; }
    void SetBackgroundColor(const UIColor& c) { BackgroundColor = c; }

    // Members accessed by UMG UBorder.h
    UIColor   BackgroundColor {0.2f, 0.2f, 0.2f, 1.0f};
    bool      DrawBackground {true};
    FMargin   Padding;
    EHorizontalAlignment HAlign {EHorizontalAlignment::Center};
    EVerticalAlignment VAlign {EVerticalAlignment::Center};

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer && Visibility != EVisibility::Collapsed && Visibility != EVisibility::Hidden)
        {
            const UIRect rect = geom.ToRect();
            if (DrawBackground)
                ctx.Renderer->drawQuad(rect, BackgroundColor);
            if (m_Child)
                m_Child->Paint(ctx, geom);
        }
    }

private:
    UIColor   BorderColor {0.5f, 0.5f, 0.5f, 1.0f};
};

}  // namespace ZSlate

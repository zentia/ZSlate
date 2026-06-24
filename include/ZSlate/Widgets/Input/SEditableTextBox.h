#pragma once

// SEditableTextBox: a single-line text input box.
// UE analogue: Widgets/Input/SEditableTextBox.h
// TODO: replace with full implementation from ZSlate submodule.

#include "ZSlate/Widgets/SWidget.h"

#include <functional>
#include <string>

namespace ZSlate
{

class SEditableTextBox : public SWidget
{
public:
    SEditableTextBox() = default;
    virtual ~SEditableTextBox() = default;

    // Public members accessed by UMG UEditableText.h and editor windows
    std::string Text;
    std::string HintText;
    float FontSize {16.0f};
    float MinWidth {140.0f};
    FMargin Padding;
    std::function<void(const std::string&)> OnTextChanged;
    std::function<void(const std::string&)> OnTextCommitted;

    void SetText(const std::string& t) { Text = t; }
    const std::string& GetText() const { return Text; }
    void SetHintText(const std::string& h) { HintText = h; }
    bool IsFocused() const { return m_Focused; }

    Vector2 ComputeDesiredSize() const override
    {
        return Vector2(std::max(MinWidth, static_cast<float>(Text.length()) * FontSize * 0.6f + 12.0f), FontSize + 10.0f);
    }
    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer)
        {
            const UIRect rect = geom.ToRect();
            ctx.Renderer->DrawQuad(rect, UIColor(0.15f, 0.15f, 0.18f, 1.0f));
            const std::string& display = Text.empty() ? HintText : Text;
            const UIColor displayColor = Text.empty() ? UIColor(0.5f, 0.5f, 0.55f, 1.0f) : UIColor(0.9f, 0.9f, 0.9f, 1.0f);
            if (!display.empty())
            {
                ctx.Renderer->DrawText(UIRect(rect.x + 6.0f, rect.y, rect.w - 12.0f, rect.h),
                                       display, FontSize, displayColor,
                                       TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
            }
        }
    }

private:
    mutable bool m_Focused {false};
};

}  // namespace ZSlate

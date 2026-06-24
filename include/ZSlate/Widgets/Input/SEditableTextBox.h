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

    void SetText(const std::string& t) { m_Text = t; }
    const std::string& GetText() const { return m_Text; }
    void SetHintText(const std::string&) {}
    void SetOnTextChanged(std::function<void(const std::string&)> fn) { m_OnChanged = fn; }

    Vector2 ComputeDesiredSize() const override { return Vector2(200.0f, 24.0f); }
    void ArrangeChildren(const FGeometry& geom, std::vector<FArrangedWidget>& out) const override { (void)geom; (void)out; }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer)
        {
            const UIRect rect = geom.ToRect();
            ctx.Renderer->drawQuad(rect, UIColor(0.15f, 0.15f, 0.18f, 1.0f));
            ctx.Renderer->drawText(rect, m_Text, 12.0f, UIColor(0.9f, 0.9f, 0.9f, 1.0f),
                                   TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
        }
    }

private:
    std::string m_Text;
    std::function<void(const std::string&)> m_OnChanged;
};

}  // namespace ZSlate

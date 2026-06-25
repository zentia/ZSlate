#pragma once

#define ZSLATE_SEDITABLETEXTBOX_DEFINED

#include "ZSlate/Widgets/SWidget.h"

#include <functional>
#include <string>

namespace ZSlate
{

// =============================================================================
// SEditableTextBox — single-line text input (UE SEditableTextBox analogue)
// =============================================================================

class SEditableTextBox : public SWidget
{
public:
    std::string Text;
    std::string HintText;
    float FontSize {16.0f};
    float MinWidth {140.0f};
    FMargin Padding;

    std::function<void(const std::string&)> OnTextChanged;
    std::function<void(const std::string&)> OnTextCommitted;

    void SetText(const std::string& t)        { Text = t; m_CursorPos = t.size(); }
    const std::string& GetText() const        { return Text; }
    void SetHintText(const std::string& h)     { HintText = h; }
    bool IsFocused() const                     { return m_Focused; }

    Vector2 ComputeDesiredSize() const override
    {
        return Vector2(std::max(MinWidth,
            static_cast<float>(Text.length()) * FontSize * 0.6f + 12.0f),
            FontSize + 10.0f);
    }

    void ArrangeChildren(const FGeometry& geom,
                         std::vector<FArrangedWidget>& out) const override
    {
        (void)geom; (void)out;
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (!ctx.Renderer) return;
        const UIRect rect = geom.ToRect();

        // Background
        ctx.Renderer->DrawQuad(rect, m_Focused
            ? UIColor(0.18f, 0.19f, 0.24f, 1.0f)
            : UIColor(0.12f, 0.12f, 0.15f, 1.0f));

        // Border
        ctx.Renderer->DrawRect(rect, m_Focused
            ? UIColor(0.45f, 0.55f, 0.80f, 1.0f)
            : UIColor(0.30f, 0.32f, 0.38f, 1.0f), 1.0f);

        // Text or hint
        UIRect textRect(rect.x + Padding.Left + 4.0f, rect.y + Padding.Top,
                        rect.w - Padding.GetTotalHorizontal() - 8.0f,
                        rect.h - Padding.GetTotalVertical());

        if (!Text.empty())
        {
            std::string display = Text;
            // Insert cursor if focused
            if (m_Focused && (m_CursorBlinkPhase < 0.5f))
            {
                size_t pos = std::min(m_CursorPos, display.size());
                display.insert(pos, "|");
            }

            ctx.Renderer->DrawTextLabel(
                textRect, display, FontSize,
                UIColor(0.90f, 0.90f, 0.95f, 1.0f),
                TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
        }
        else if (!HintText.empty())
        {
            ctx.Renderer->DrawTextLabel(
                textRect, HintText, FontSize,
                UIColor(0.45f, 0.45f, 0.50f, 1.0f),
                TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
        }
    }

    // ---- Input ---------------------------------------------------------------

    bool SupportsKeyboardFocus() const override { return true; }
    void OnFocusReceived() override { m_Focused = true; m_CursorBlinkPhase = 0; }
    void OnFocusLost() override { m_Focused = false; }

    FReply OnMouseButtonDown(const Vector2&, int btn) override
    {
        if (btn == 0) return FReply::Handled();  // gain focus
        return FReply::Unhandled();
    }

    void OnKeyChar(unsigned int codepoint) override
    {
        if (!m_Focused) return;

        if (codepoint == '\b' || codepoint == 127)  // Backspace
        {
            if (m_CursorPos > 0)
            {
                // Remove one UTF-8 char before cursor
                size_t del = m_CursorPos - 1;
                while (del > 0 && (Text[del] & 0xC0) == 0x80) --del;
                size_t len = m_CursorPos - del;
                Text.erase(del, len);
                m_CursorPos = del;
                if (OnTextChanged) OnTextChanged(Text);
            }
        }
        else if (codepoint == 127 || codepoint == 0x7F)  // Delete
        {
            if (m_CursorPos < Text.size())
            {
                size_t end = m_CursorPos + 1;
                while (end < Text.size() && (Text[end] & 0xC0) == 0x80) ++end;
                Text.erase(m_CursorPos, end - m_CursorPos);
                if (OnTextChanged) OnTextChanged(Text);
            }
        }
        else if (codepoint == '\r' || codepoint == '\n')  // Enter
        {
            if (OnTextCommitted) OnTextCommitted(Text);
        }
        else if (codepoint >= 32)  // Printable
        {
            // Encode as UTF-8
            std::string utf8;
            if (codepoint < 0x80)
                utf8 += static_cast<char>(codepoint);
            else if (codepoint < 0x800)
            {
                utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
                utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            else if (codepoint < 0x10000)
            {
                utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
                utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            else
            {
                utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
                utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            Text.insert(m_CursorPos, utf8);
            m_CursorPos += utf8.size();
            if (OnTextChanged) OnTextChanged(Text);
        }
        m_CursorBlinkPhase = 0.0f;
    }

    void Tick(const FGeometry&, float deltaSec) override
    {
        if (m_Focused)
            m_CursorBlinkPhase = std::fmod(m_CursorBlinkPhase + deltaSec * 2.0f, 1.0f);
    }

private:
    mutable bool   m_Focused {false};
    mutable size_t m_CursorPos {0};
    mutable float  m_CursorBlinkPhase {0.0f};
};

}  // namespace ZSlate

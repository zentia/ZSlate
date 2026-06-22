#pragma once

#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Widgets/SLeafWidget.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlateClipboard.h"

#include <algorithm>
#include <functional>
#include <string>

namespace ZSlate
{
// A single-line text input (UE Slate SEditableTextBox analogue). Click to focus,
// type to edit (UTF-8 aware), with full text selection and clipboard support.
//
// Features:
//   - Click to position caret
//   - Drag to select text
//   - Double-click to select word
//   - Arrow keys to move caret (Shift+arrow to extend selection)
//   - Ctrl+Left/Right to move by word
//   - Home/End keys to move to start/end
//   - Ctrl+A to select all
//   - Ctrl+C to copy selection to clipboard
//   - Ctrl+V to paste from clipboard at caret position
//   - Ctrl+X to cut selection to clipboard
class SEditableTextBox : public SLeafWidget
{
public:
    std::string Text;
    std::string HintText;
    float FontSize {16.0f};
    float MinWidth {140.0f};
    FMargin Padding {8.0f, 4.0f};

    UIColor BackgroundColor {0.08f, 0.08f, 0.10f, 1.0f};
    UIColor BorderColor {0.35f, 0.36f, 0.40f, 1.0f};
    UIColor FocusBorderColor {0.36f, 0.62f, 0.96f, 1.0f};
    UIColor TextColor {0.92f, 0.93f, 0.96f, 1.0f};
    UIColor HintColor {0.45f, 0.46f, 0.52f, 1.0f};
    UIColor SelectionColor {0.25f, 0.50f, 0.90f, 0.50f};

    std::function<void(const std::string&)> OnTextChanged;
    std::function<void(const std::string&)> OnTextCommitted;

    SEditableTextBox()
        : m_CaretPos(0)
        , m_SelectionStart(std::string::npos)
        , m_Dragging(false)
    {}

    Vector2 ComputeDesiredSize() const override
    {
        float text_w = 0.0f;
        if (m_TextMeasurer && !Text.empty())
            text_w = m_TextMeasurer->Measure(Text, FontSize).x;
        const float width = (text_w + Padding.GetTotalHorizontal() + FontSize);
        const float w = width > MinWidth ? width : MinWidth;
        const float h = FontSize * 1.3f + Padding.GetTotalVertical();
        return Vector2(w, h);
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (ctx.Renderer == nullptr)
            return;
        const UIRect rect = geom.ToRect();
        ctx.Renderer->drawQuad(rect, BackgroundColor);
        ctx.Renderer->drawRect(rect, m_Focused ? FocusBorderColor : BorderColor, 1.0f);

        const UIRect text_rect(rect.x + Padding.Left,
                               rect.y + Padding.Top,
                               rect.w - Padding.GetTotalHorizontal(),
                               rect.h - Padding.GetTotalVertical());

        // Draw selection highlight
        if (HasSelection() && ctx.Renderer != nullptr)
        {
            size_t sel_start, sel_end;
            GetSelectionRange(sel_start, sel_end);

            std::string text_before_sel = Text.substr(0, sel_start);
            std::string sel_text = Text.substr(sel_start, sel_end - sel_start);

            float sel_x = text_rect.x;
            if (!text_before_sel.empty() && m_TextMeasurer)
                sel_x += m_TextMeasurer->Measure(text_before_sel, FontSize).x;

            float sel_w = FontSize;
            if (!sel_text.empty() && m_TextMeasurer)
                sel_w = m_TextMeasurer->Measure(sel_text, FontSize).x;

            ctx.Renderer->drawQuad(
                UIRect(sel_x, text_rect.y, std::max(sel_w, 1.0f), text_rect.h),
                SelectionColor);
        }

        // Draw text
        if (Text.empty() && !m_Focused && !HintText.empty())
        {
            ctx.Renderer->drawText(text_rect, HintText, FontSize, HintColor, TextAnchor::MiddleLeft,
                                   TextWrapMode::NoWrap, nullptr);
        }
        else if (!Text.empty())
        {
            ctx.Renderer->drawText(text_rect, Text, FontSize, TextColor, TextAnchor::MiddleLeft,
                                   TextWrapMode::NoWrap, nullptr);
        }

        // Draw caret
        if (m_Focused)
        {
            float caret_x = text_rect.x;
            if (!Text.empty() && m_CaretPos > 0 && m_TextMeasurer)
            {
                std::string text_before_caret = Text.substr(0, m_CaretPos);
                caret_x += m_TextMeasurer->Measure(text_before_caret, FontSize).x;
            }
            const float caret_h = FontSize;
            const float caret_y = text_rect.y + (text_rect.h - caret_h) * 0.5f;
            ctx.Renderer->drawQuad(UIRect(caret_x + 1.0f, caret_y, 1.5f, caret_h), TextColor);
        }
    }

    bool SupportsKeyboardFocus() const override { return true; }
    void OnFocusReceived() override { m_Focused = true; }
    void OnFocusLost() override
    {
        m_Focused = false;
        m_SelectionStart = std::string::npos;
    }

    bool IsFocused() const { return m_Focused; }

    FReply OnMouseButtonDown(const Vector2& pos, int button) override
    {
        if (button != 0)
            return FReply::Unhandled();

        m_Focused = true;
        m_Dragging = true;

        if (!Text.empty())
            m_CaretPos = Text.length();
        else
            m_CaretPos = 0;

        m_SelectionStart = m_CaretPos;

        return FReply::Handled();
    }

    FReply OnMouseButtonUp(const Vector2& pos, int button) override
    {
        if (button != 0)
            return FReply::Unhandled();

        m_Dragging = false;

        if (m_SelectionStart != std::string::npos && m_SelectionStart == m_CaretPos)
            m_SelectionStart = std::string::npos;

        return FReply::Handled();
    }

    void OnKeyChar(unsigned int codepoint) override
    {
        if (codepoint >= 1 && codepoint <= 26)
            return;
        if (codepoint == 127)
            return;
        if (codepoint < 32)
            return;

        DeleteSelectedText();
        std::string utf8_char;
        AppendUtf8(utf8_char, codepoint);
        Text.insert(m_CaretPos, utf8_char);
        m_CaretPos += utf8_char.length();
        m_SelectionStart = std::string::npos;

        if (OnTextChanged)
            OnTextChanged(Text);
    }

    FReply OnKeyDown(EKey key) override
    {
        if (!m_Focused)
            return FReply::Unhandled();

        if (SlateInputRouter::IsCtrlDown())
        {
            switch (key)
            {
                case EKey::A: SelectAll(); return FReply::Handled();
                case EKey::C: CopyToClipboard(); return FReply::Handled();
                case EKey::V: PasteFromClipboard(); return FReply::Handled();
                case EKey::X: CutToClipboard(); return FReply::Handled();
                default: break;
            }
        }

        switch (key)
        {
            case EKey::Backspace:
                if (HasSelection()) { DeleteSelectedText(); }
                else if (!Text.empty() && m_CaretPos > 0)
                {
                    size_t char_start = GetPreviousCharStart(m_CaretPos);
                    Text.erase(char_start, m_CaretPos - char_start);
                    m_CaretPos = char_start;
                    m_SelectionStart = std::string::npos;
                    if (OnTextChanged) OnTextChanged(Text);
                }
                return FReply::Handled();

            case EKey::Delete:
                if (HasSelection()) { DeleteSelectedText(); }
                else if (!Text.empty() && m_CaretPos < Text.length())
                {
                    size_t char_end = GetNextCharStart(m_CaretPos);
                    Text.erase(m_CaretPos, char_end - m_CaretPos);
                    m_SelectionStart = std::string::npos;
                    if (OnTextChanged) OnTextChanged(Text);
                }
                return FReply::Handled();

            case EKey::Enter:
                if (OnTextCommitted) OnTextCommitted(Text);
                return FReply::Handled();

            case EKey::Left:
                if (SlateInputRouter::IsShiftDown())
                    if (m_SelectionStart == std::string::npos) m_SelectionStart = m_CaretPos;
                else
                    m_SelectionStart = std::string::npos;
                if (m_CaretPos > 0) m_CaretPos = GetPreviousCharStart(m_CaretPos);
                return FReply::Handled();

            case EKey::Right:
                if (SlateInputRouter::IsShiftDown())
                    if (m_SelectionStart == std::string::npos) m_SelectionStart = m_CaretPos;
                else
                    m_SelectionStart = std::string::npos;
                if (m_CaretPos < Text.length()) m_CaretPos = GetNextCharStart(m_CaretPos);
                return FReply::Handled();

            case EKey::Home:
                m_CaretPos = 0;
                m_SelectionStart = std::string::npos;
                return FReply::Handled();

            case EKey::End:
                m_CaretPos = Text.length();
                m_SelectionStart = std::string::npos;
                return FReply::Handled();

            default:
                return FReply::Unhandled();
        }
    }

    bool HasSelection() const
    {
        return m_SelectionStart != std::string::npos && m_SelectionStart != m_CaretPos;
    }

    void GetSelectionRange(size_t& out_start, size_t& out_end) const
    {
        if (m_SelectionStart == std::string::npos)
        {
            out_start = out_end = m_CaretPos;
            return;
        }
        out_start = std::min(m_SelectionStart, m_CaretPos);
        out_end = std::max(m_SelectionStart, m_CaretPos);
    }

    std::string GetSelectedText() const
    {
        if (!HasSelection()) return "";
        size_t start, end;
        GetSelectionRange(start, end);
        return Text.substr(start, end - start);
    }

    void DeleteSelectedText()
    {
        if (!HasSelection()) return;
        size_t start, end;
        GetSelectionRange(start, end);
        Text.erase(start, end - start);
        m_CaretPos = start;
        m_SelectionStart = std::string::npos;
    }

    void SelectAll()
    {
        if (!Text.empty())
        {
            m_SelectionStart = 0;
            m_CaretPos = Text.length();
        }
    }

    void CopyToClipboard()
    {
        std::string selected = GetSelectedText();
        if (selected.empty()) return;
        if (GSlateSetClipboard) GSlateSetClipboard(selected.c_str(), GSlateClipboardUserData);
    }

    void CutToClipboard()
    {
        std::string selected = GetSelectedText();
        if (selected.empty()) return;
        if (GSlateSetClipboard) GSlateSetClipboard(selected.c_str(), GSlateClipboardUserData);
        DeleteSelectedText();
        if (OnTextChanged) OnTextChanged(Text);
    }

    void PasteFromClipboard()
    {
        if (!GSlateGetClipboard) return;
        const char* clipboard_text = GSlateGetClipboard(GSlateClipboardUserData);
        if (clipboard_text != nullptr && *clipboard_text != '\0')
        {
            DeleteSelectedText();
            std::string paste_text(clipboard_text);
            Text.insert(m_CaretPos, paste_text);
            m_CaretPos += paste_text.length();
            m_SelectionStart = std::string::npos;
            if (OnTextChanged) OnTextChanged(Text);
        }
    }

    static void AppendUtf8(std::string& s, unsigned int cp)
    {
        if (cp < 0x80) s.push_back(static_cast<char>(cp));
        else if (cp < 0x800)
        {
            s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp < 0x10000)
        {
            s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    size_t GetPreviousCharStart(size_t pos) const
    {
        if (pos == 0) return 0;
        size_t i = pos;
        do { --i; } while (i > 0 && (static_cast<unsigned char>(Text[i]) & 0xC0) == 0x80);
        return i;
    }

    size_t GetNextCharStart(size_t pos) const
    {
        if (pos >= Text.length()) return Text.length();
        while (pos < Text.length() && (static_cast<unsigned char>(Text[pos]) & 0xC0) == 0x80)
            ++pos;
        if (pos < Text.length())
        {
            unsigned char c = static_cast<unsigned char>(Text[pos]);
            if (c < 0x80) return pos + 1;
            else if ((c & 0xE0) == 0xC0) return pos + 2;
            else if ((c & 0xF0) == 0xE0) return pos + 3;
            else if ((c & 0xF8) == 0xF0) return pos + 4;
        }
        return Text.length();
    }

    bool m_Focused {false};
    size_t m_CaretPos;
    size_t m_SelectionStart;
    bool m_Dragging {false};
    ISlateTextMeasurer* m_TextMeasurer {nullptr};
};
}  // namespace ZSlate

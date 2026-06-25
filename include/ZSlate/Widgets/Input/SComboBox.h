#pragma once

#include "ZSlate/Widgets/SCompoundWidget.h"
#include "ZSlate/Widgets/Input/SButton.h"
#include "ZSlate/Widgets/Text/STextBlock.h"
#include "ZSlate/Widgets/SPanel.h"
#include "ZSlate/Widgets/Layout/SScrollBox.h"

#include <functional>
#include <string>
#include <vector>

namespace ZSlate
{

// =============================================================================
// SComboBox — dropdown selection (UE SComboBox analogue)
// =============================================================================
//
// Displays the selected item; clicking opens a dropdown list.
//
template <typename T>
class SComboBox : public SCompoundWidget
{
public:
    // Data
    std::vector<T> Options;
    T SelectedItem {};
    int32_t SelectedIndex {-1};

    // Styling
    float ItemHeight {22.0f};
    float FontSize {13.0f};
    float MaxDropdownHeight {200.0f};

    // Callbacks
    std::function<std::string(const T&)> OnGetLabel;
    std::function<void(const T&, int32_t)> OnSelectionChanged;

    Vector2 ComputeDesiredSize() const override { return Vector2(160.0f, 26.0f); }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (!ctx.Renderer) return;
        const UIRect r = geom.ToRect();

        // Closed state: just the selected value as a button-like box
        UIColor bg = m_Open ? UIColor(0.20f, 0.22f, 0.28f, 1.0f)
                   : m_Hovered ? UIColor(0.16f, 0.17f, 0.21f, 1.0f)
                   : UIColor(0.12f, 0.12f, 0.14f, 1.0f);
        ctx.Renderer->DrawQuad(r, bg);
        ctx.Renderer->DrawRect(r, UIColor(0.35f, 0.38f, 0.45f, 1.0f), 1.0f);

        // Arrow
        const char* arrow = m_Open ? "\xE2\x96\xB2" : "\xE2\x96\xBC";  // ▲ or ▼
        ctx.Renderer->DrawTextLabel(
            UIRect(r.Right() - 24.0f, r.y, 20.0f, r.h),
            arrow, FontSize, UIColor(0.7f, 0.7f, 0.7f, 1.0f),
            TextAnchor::MiddleCenter, TextWrapMode::NoWrap);

        // Selected text
        std::string label;
        if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Options.size()))
            label = OnGetLabel ? OnGetLabel(Options[SelectedIndex]) : "?";
        else
            label = "...";

        ctx.Renderer->DrawTextLabel(
            UIRect(r.x + 6.0f, r.y, r.w - 30.0f, r.h),
            label, FontSize, UIColor(0.88f, 0.89f, 0.92f, 1.0f),
            TextAnchor::MiddleLeft, TextWrapMode::NoWrap);

        // Dropdown
        if (m_Open)
            PaintDropdown(ctx, r);
    }

    // ---- Input ---------------------------------------------------------------

    void OnMouseEnter() override { m_Hovered = true; }
    void OnMouseLeave() override
    {
        m_Hovered = false;
        if (m_Open) m_Open = false;
    }

    FReply OnMouseButtonDown(const Vector2& pos, int btn) override
    {
        if (btn != 0) return FReply::Unhandled();

        const UIRect r = GetCachedGeometry().ToRect();

        if (m_Open)
        {
            // Check dropdown items
            float dropY = r.Bottom();
            int32_t numItems = static_cast<int32_t>(Options.size());
            float dropH = std::min(ItemHeight * numItems, MaxDropdownHeight);

            if (pos.x >= r.x && pos.x <= r.Right() && pos.y >= dropY && pos.y <= dropY + dropH)
            {
                int32_t idx = static_cast<int32_t>((pos.y - dropY) / ItemHeight);
                if (idx >= 0 && idx < numItems)
                {
                    SelectedIndex = idx;
                    SelectedItem = Options[idx];
                    if (OnSelectionChanged) OnSelectionChanged(SelectedItem, idx);
                }
            }
            m_Open = false;
            return FReply::Handled();
        }

        // Toggle open
        m_Open = true;
        return FReply::Handled();
    }

private:
    mutable bool m_Hovered {false};
    mutable bool m_Open {false};

    void PaintDropdown(const FPaintContext& ctx, const UIRect& mainRect) const
    {
        float dropY = mainRect.Bottom();
        int32_t numItems = static_cast<int32_t>(Options.size());
        float dropH = std::min(ItemHeight * numItems, MaxDropdownHeight);

        // Dropdown background
        ctx.Renderer->DrawQuad(
            UIRect(mainRect.x, dropY, mainRect.w, dropH),
            UIColor(0.14f, 0.15f, 0.18f, 0.97f));
        ctx.Renderer->DrawRect(
            UIRect(mainRect.x, dropY, mainRect.w, dropH),
            UIColor(0.35f, 0.38f, 0.45f, 1.0f), 1.0f);

        for (int32_t i = 0; i < numItems; ++i)
        {
            float iy = dropY + static_cast<float>(i) * ItemHeight;
            UIRect ir(mainRect.x + 1.0f, iy, mainRect.w - 2.0f, ItemHeight);

            // Highlight
            if (i == SelectedIndex)
                ctx.Renderer->DrawQuad(ir, UIColor(0.18f, 0.35f, 0.62f, 1.0f));

            std::string label = OnGetLabel ? OnGetLabel(Options[i]) : ("#" + std::to_string(i));
            ctx.Renderer->DrawTextLabel(
                UIRect(ir.x + 6.0f, ir.y, ir.w - 12.0f, ir.h),
                label, FontSize,
                i == SelectedIndex ? UIColor(1.0f, 1.0f, 1.0f, 1.0f)
                                   : UIColor(0.85f, 0.88f, 0.94f, 1.0f),
                TextAnchor::MiddleLeft, TextWrapMode::NoWrap);
        }
    }
};

}  // namespace ZSlate

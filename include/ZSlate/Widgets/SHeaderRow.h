#pragma once

#include "ZSlate/Widgets/SWidget.h"
#include "ZSlate/Widgets/SPanel.h"

#include <functional>
#include <string>
#include <vector>

namespace ZSlate
{

// =============================================================================
// SHeaderRow — table column headers (UE SHeaderRow analogue)
// =============================================================================
//
// A horizontal bar of resizable column headers. Each column has
// a label, a width, and an optional sort arrow.
//
class SHeaderRow : public SPanel
{
public:
    enum class EColumnSortMode { None, Ascending, Descending };

    struct FColumn
    {
        std::string Label;
        float Width {100.0f};
        float MinWidth {30.0f};
        EColumnSortMode SortMode {EColumnSortMode::None};
        std::function<void()> OnSort;
    };

    std::vector<FColumn> Columns;

    float RowHeight {24.0f};
    float FontSize {12.0f};

    // Track which column is being resized
    mutable int32_t ResizingColumn {-1};
    mutable float   ResizeStartX {0.0f};
    mutable float   ResizeStartWidth {0.0f};

    Vector2 ComputeDesiredSize() const override
    {
        float totalW = 0.0f;
        for (const auto& col : Columns) totalW += col.Width;
        return Vector2(totalW, RowHeight);
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (!ctx.Renderer) return;
        const UIRect r = geom.ToRect();

        float cursorX = r.x;
        for (size_t i = 0; i < Columns.size(); ++i)
        {
            const auto& col = Columns[i];
            UIRect cr(cursorX, r.y, col.Width, RowHeight);

            // Background
            ctx.Renderer->DrawQuad(cr, UIColor(0.10f, 0.10f, 0.12f, 1.0f));
            ctx.Renderer->DrawRect(cr, UIColor(0.20f, 0.22f, 0.28f, 1.0f), 1.0f);

            // Label
            ctx.Renderer->DrawTextLabel(
                UIRect(cr.x + 6.0f, cr.y, cr.w - 28.0f, cr.h),
                col.Label, FontSize,
                UIColor(0.80f, 0.82f, 0.88f, 1.0f),
                TextAnchor::MiddleLeft, TextWrapMode::NoWrap);

            // Sort arrow
            if (col.SortMode != EColumnSortMode::None)
            {
                const char* arrow = (col.SortMode == EColumnSortMode::Ascending) ? "\xE2\x96\xB2" : "\xE2\x96\xBC";
                ctx.Renderer->DrawTextLabel(
                    UIRect(cr.Right() - 24.0f, cr.y, 18.0f, cr.h),
                    arrow, FontSize - 2.0f,
                    UIColor(0.60f, 0.65f, 0.75f, 1.0f),
                    TextAnchor::MiddleCenter, TextWrapMode::NoWrap);
            }

            // Resize handle (right edge)
            if (i < Columns.size() - 1)
            {
                float handleX = cr.Right() - 2.0f;
                float handleW = (static_cast<int32_t>(i) == ResizingColumn) ? 2.0f : 4.0f;
                ctx.Renderer->DrawQuad(
                    UIRect(handleX, cr.y, handleW, cr.h),
                    (static_cast<int32_t>(i) == ResizingColumn)
                        ? UIColor(0.5f, 0.6f, 1.0f, 0.8f)
                        : UIColor(0.25f, 0.27f, 0.33f, 1.0f));
            }

            cursorX += col.Width;
        }
    }

    // ---- Input ---------------------------------------------------------------

    void OnMouseMove(const Vector2& pos) override
    {
        if (ResizingColumn < 0) return;

        const UIRect r = GetCachedGeometry().ToRect();
        float delta = pos.x - ResizeStartX;
        float newW = std::max(Columns[ResizingColumn].MinWidth, ResizeStartWidth + delta);
        Columns[ResizingColumn].Width = newW;
    }

    FReply OnMouseButtonDown(const Vector2& pos, int btn) override
    {
        if (btn != 0) return FReply::Unhandled();

        const UIRect r = GetCachedGeometry().ToRect();
        float cursorX = r.x;

        for (int32_t i = 0; i < static_cast<int32_t>(Columns.size()) - 1; ++i)
        {
            cursorX += Columns[i].Width;

            // Resize hit test (within 4px of column right edge)
            if (std::abs(pos.x - cursorX) < 4.0f)
            {
                ResizingColumn = i;
                ResizeStartX = pos.x;
                ResizeStartWidth = Columns[i].Width;
                return FReply::Handled().CaptureMouse(const_cast<SHeaderRow*>(this));
            }
        }

        // Column click for sorting
        cursorX = r.x;
        for (int32_t i = 0; i < static_cast<int32_t>(Columns.size()); ++i)
        {
            if (pos.x >= cursorX && pos.x < cursorX + Columns[i].Width)
            {
                if (Columns[i].OnSort)
                {
                    // Cycle: None → Asc → Desc → None
                    switch (Columns[i].SortMode)
                    {
                    case EColumnSortMode::None:    Columns[i].SortMode = EColumnSortMode::Ascending; break;
                    case EColumnSortMode::Ascending: Columns[i].SortMode = EColumnSortMode::Descending; break;
                    case EColumnSortMode::Descending: Columns[i].SortMode = EColumnSortMode::None; break;
                    }
                    // Clear other columns
                    for (int32_t j = 0; j < static_cast<int32_t>(Columns.size()); ++j)
                        if (j != i) Columns[j].SortMode = EColumnSortMode::None;
                    Columns[i].OnSort();
                }
                return FReply::Handled();
            }
            cursorX += Columns[i].Width;
        }
        return FReply::Unhandled();
    }

    FReply OnMouseButtonUp(const Vector2&, int btn) override
    {
        if (btn != 0 || ResizingColumn < 0) return FReply::Unhandled();
        ResizingColumn = -1;
        return FReply::Handled().ReleaseMouseCapture();
    }

    void OnMouseCaptureLost() override { ResizingColumn = -1; }
};

}  // namespace ZSlate

#pragma once

#include "ZSlate/Widgets/SPanel.h"
#include "ZSlate/Widgets/SHeaderRow.h"

#include <functional>
#include <vector>

namespace ZSlate
{

// =============================================================================
// STableViewBase — base class for table/list views (UE STableViewBase analogue)
// =============================================================================
//
// Provides column-header integration via SHeaderRow and a common
// interface for table/list views.
//
template <typename ItemType>
class STableViewBase : public SPanel
{
public:
    // Attach a header row for column-based views
    void SetHeaderRow(std::shared_ptr<SHeaderRow> header)
    {
        m_HeaderRow = std::move(header);
    }
    std::shared_ptr<SHeaderRow> GetHeaderRow() const { return m_HeaderRow; }

    // Selection tracking
    mutable int32_t SelectedIndex {-1};
    std::function<void(const ItemType&, int32_t)> OnSelectionChanged;

    // Row height (pixels)
    float RowHeight {24.0f};

    // ---- Layout --------------------------------------------------------------

    Vector2 ComputeDesiredSize() const override
    {
        float totalW = 0.0f;
        float headerH = 0.0f;

        if (m_HeaderRow)
        {
            totalW = m_HeaderRow->ComputeDesiredSize().x;
            headerH = m_HeaderRow->GetDesiredSize().y;
        }

        float rowsH = GetRowCount() * RowHeight;
        return Vector2(totalW, headerH + rowsH);
    }

    // Derived classes implement these
    virtual int32_t GetRowCount() const = 0;
    virtual const ItemType* GetItemAt(int32_t index) const = 0;

protected:
    std::shared_ptr<SHeaderRow> m_HeaderRow;

    // Paint the header and row background
    void PaintHeader(const FPaintContext& ctx, const FGeometry& geom) const
    {
        if (!m_HeaderRow) return;
        std::vector<FArrangedChild> arranged;
        m_HeaderRow->OnArrangeChildren(geom, arranged);
        m_HeaderRow->OnPaint(ctx, geom);
    }

    void PaintRowBackground(const FPaintContext& ctx,
                            const UIRect& rowRect,
                            int32_t index) const
    {
        if (!ctx.Renderer) return;

        if (index == SelectedIndex)
            ctx.Renderer->DrawQuad(rowRect, UIColor(0.18f, 0.35f, 0.62f, 1.0f));
        else if (index % 2 == 0)
            ctx.Renderer->DrawQuad(rowRect, UIColor(0.10f, 0.10f, 0.13f, 1.0f));
        else
            ctx.Renderer->DrawQuad(rowRect, UIColor(0.12f, 0.12f, 0.15f, 1.0f));

        // Bottom divider
        ctx.Renderer->DrawRect(
            UIRect(rowRect.x, rowRect.Bottom() - 1.0f, rowRect.w, 1.0f),
            UIColor(0.08f, 0.08f, 0.10f, 1.0f), 1.0f);
    }

    // Generic hit-test for row index
    int32_t HitTestRow(const Vector2& pos, const UIRect& view) const
    {
        float headerH = m_HeaderRow ? m_HeaderRow->GetDesiredSize().y : 0.0f;
        float rowY = pos.y - view.y - headerH;
        if (rowY < 0) return -1;
        int32_t idx = static_cast<int32_t>(rowY / RowHeight);
        if (idx < 0 || idx >= GetRowCount()) return -1;
        return idx;
    }
};

}  // namespace ZSlate

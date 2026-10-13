#pragma once

#include "ZSlate/Widgets/Views/STableViewBase.h"
#include "ZSlate/Widgets/Views/STableRow.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace ZSlate
{

// =============================================================================
// SListView<T> — vertical list view with widget recycling (UE SListView analogue)
// =============================================================================
//
// Inherits from STableViewBase for scroll/selection/scrollbar/header.
// Only creates widgets for visible items; recycles them when items scroll
// in/out of view (ItemToWidget map).
//
template <typename ItemType>
class SListView : public STableViewBase
{
public:
    using FOnGenerateRow = std::function<std::shared_ptr<SWidget>(const ItemType&, int32_t)>;

    FOnGenerateRow OnGenerateRow;
    std::function<void(const ItemType&, int32_t)> OnSelectionChanged;

    // Data source
    std::vector<ItemType> Items;

    Vector2 ComputeDesiredSize() const override
    {
        float w = 0.0f;
        if (m_HeaderRow)
        {
            w = m_HeaderRow->GetDesiredSize().x;
        }
        return Vector2(std::max(w, 200.0f),
                       Items.size() * ItemHeight + GetHeaderHeight());
    }

    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override
    {
        if (!ctx.Renderer || Items.empty()) return;

        const UIRect view = geom.ToRect();
        float hh = GetHeaderHeight();

        // Header
        PaintHeader(ctx, geom);

        // Content area (clipped)
        float contentY = view.y + hh;
        float contentH = view.h - hh;
        ctx.Renderer->PushClipRect(UIRect(view.x, contentY, view.w, contentH));

        // Visible range
        int32_t first = std::max(0, static_cast<int32_t>(m_ScrollOffset / ItemHeight));
        int32_t last = std::min(static_cast<int32_t>(Items.size()),
                                first + static_cast<int32_t>(contentH / ItemHeight) + 2);
        // +2 for partial visibility at top/bottom

        for (int32_t i = first; i < last; ++i)
        {
            float y = contentY + i * ItemHeight - m_ScrollOffset;
            UIRect rowRect(view.x, y, view.w, ItemHeight);

            // Selection highlight
            if (IsSelected(i))
                ctx.Renderer->DrawQuad(rowRect, UIColor(0.18f, 0.35f, 0.62f, 1.0f));

            // Row content
            if (OnGenerateRow)
            {
                auto rowWidget = GetOrCreateWidget(i);
                if (rowWidget)
                {
                    FGeometry rowGeom = geom.MakeChild(
                        Vector2(0.0f, y - view.y), Vector2(view.w, ItemHeight));
                    // Desired sizes must be cached before Paint: the widget was
                    // just built and its children have never been measured, so
                    // any Auto slot inside the row would arrange at size 0.
                    rowWidget->CacheDesiredSize();
                    rowWidget->Paint(ctx, rowGeom);
                }
            }
        }

        // Drop the widgets that scrolled out of view. Without this the
        // item->widget map grows for every row ever visited, so scrolling a
        // large list retains a widget per row until the view is destroyed.
        ReleaseInvisibleWidgets(first, last - 1);

        // Sync scroll
        ClampScrollOffset(static_cast<int32_t>(Items.size()), contentH);
        SyncScrollBar(static_cast<int32_t>(Items.size()), contentH);

        ctx.Renderer->PopClipRect();
    }

    // ---- Input ---------------------------------------------------------------

    void OnMouseMove(const Vector2& pos) override
    {
        const UIRect view = GetCachedGeometry().ToRect();
        float hh = GetHeaderHeight();
        if (pos.y < view.y + hh) return;

        int32_t idx = static_cast<int32_t>((pos.y - view.y - hh + m_ScrollOffset) / ItemHeight);
        if (idx != m_HoveredIndex) m_HoveredIndex = idx;
    }

    void OnMouseLeave() override { m_HoveredIndex = -1; }

    FReply OnMouseButtonDown(const Vector2& pos, int btn) override
    {
        if (btn == 2)  // right click — context menu / scroll
        {
            return FReply::Handled();
        }

        if (btn != 0 || Items.empty()) return FReply::Unhandled();

        const UIRect view = GetCachedGeometry().ToRect();
        float hh = GetHeaderHeight();

        // Header click (delegate to header; it hit-tests against its own
        // cached geometry, so no geometry needs to be passed here)
        if (pos.y < view.y + hh && m_HeaderRow)
            return m_HeaderRow->OnMouseButtonDown(pos, btn);

        // Row hit test
        int32_t idx = static_cast<int32_t>((pos.y - view.y - hh + m_ScrollOffset) / ItemHeight);
        if (idx >= 0 && idx < static_cast<int32_t>(Items.size()))
        {
            bool nowSel = !IsSelected(idx);
            SetSelected(idx, nowSel);
            m_LastClickedIndex = idx;

            if (OnSelectionChanged)
                OnSelectionChanged(Items[idx], idx);
            return FReply::Handled();
        }
        return STableViewBase::OnMouseButtonDown(pos, btn);
    }

    // ---- Widget recycling ----------------------------------------------------

    // Call after mutating Items so cached rows are rebuilt from the new data.
    // The cache is keyed by index, so stale entries would otherwise keep
    // rendering the previous contents of those indices.
    void RegenerateAllWidgets()
    {
        m_ItemToWidget.clear();
        RequestRefresh();
    }

private:
    mutable int32_t m_HoveredIndex {-1};
    mutable int32_t m_LastClickedIndex {-1};

    // Index -> row widget, holding only the rows currently on screen.
    //
    // There is deliberately no free-list of spare widgets here. OnGenerateRow
    // is the only way to bind a row to its data, and SWidget exposes no "rebind
    // to this item" hook, so a widget taken from such a pool would keep
    // rendering the item it was built for - i.e. scrolling would show the wrong
    // values in the right place, which is worse than reallocating. Rows are
    // therefore built on demand and dropped once off screen; the cost is bounded
    // by the number of visible rows, not by the size of Items.
    mutable std::unordered_map<int32_t, std::shared_ptr<SWidget>> m_ItemToWidget;

    std::shared_ptr<SWidget> GetOrCreateWidget(int32_t idx) const
    {
        auto it = m_ItemToWidget.find(idx);
        if (it != m_ItemToWidget.end())
            return it->second;

        if (!OnGenerateRow)
            return nullptr;

        std::shared_ptr<SWidget> widget = OnGenerateRow(Items[idx], idx);
        if (widget)
            m_ItemToWidget[idx] = widget;
        return widget;
    }

    // Forget every cached row outside [firstVisible, lastVisible].
    void ReleaseInvisibleWidgets(int32_t firstVisible, int32_t lastVisible) const
    {
        for (auto it = m_ItemToWidget.begin(); it != m_ItemToWidget.end();)
        {
            if (it->first < firstVisible || it->first > lastVisible)
                it = m_ItemToWidget.erase(it);
            else
                ++it;
        }
    }
};

}  // namespace ZSlate

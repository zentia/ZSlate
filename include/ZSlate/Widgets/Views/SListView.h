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
                    rowWidget->Paint(ctx, rowGeom);
                }
            }
        }

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

        // Header click (delegate to header)
        if (pos.y < view.y + hh && m_HeaderRow)
        {
            FGeometry hGeom = FGeometry(Vector2(view.x, view.y),
                                        Vector2(view.w, hh));
            return m_HeaderRow->OnMouseButtonDown(pos, btn);
        }

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

    // Force regeneration of all visible widgets next paint
    void RegenerateAllWidgets()
    {
        m_ItemToWidget.clear();
        m_WidgetPool.clear();
        RequestRefresh();
    }

private:
    mutable int32_t m_HoveredIndex {-1};
    mutable int32_t m_LastClickedIndex {-1};

    // Widget recycling: map item index → widget, plus a pool of unused widgets
    mutable std::unordered_map<int32_t, std::shared_ptr<SWidget>> m_ItemToWidget;
    mutable std::vector<std::shared_ptr<SWidget>> m_WidgetPool;

    std::shared_ptr<SWidget> GetOrCreateWidget(int32_t idx) const
    {
        // Return existing widget if still valid
        auto it = m_ItemToWidget.find(idx);
        if (it != m_ItemToWidget.end())
            return it->second;

        // Create new widget
        std::shared_ptr<SWidget> widget;
        if (!m_WidgetPool.empty())
        {
            widget = m_WidgetPool.back();
            m_WidgetPool.pop_back();
        }

        if (!widget && OnGenerateRow)
            widget = OnGenerateRow(Items[idx], idx);

        if (widget)
            m_ItemToWidget[idx] = widget;

        return widget;
    }

    void ReleaseInvisibleWidgets(int32_t firstVisible, int32_t lastVisible) const
    {
        // Move out-of-range widgets back to pool
        std::vector<int32_t> toRelease;
        for (const auto& pair : m_ItemToWidget)
        {
            if (pair.first < firstVisible || pair.first > lastVisible)
                toRelease.push_back(pair.first);
        }
        for (int32_t idx : toRelease)
        {
            m_WidgetPool.push_back(m_ItemToWidget[idx]);
            m_ItemToWidget.erase(idx);
        }
    }
};

}  // namespace ZSlate

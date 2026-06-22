#include "ZSlate/Widgets/Views/SListView.h"

namespace ZSlate
{

template<typename ItemType>
Vector2 SListView<ItemType>::ComputeDesiredSize() const
{
    // Desired width is the allotted width (filled by parent)
    // Desired height is the content height (may be larger than visible area)
    float Width = m_CachedGeometry.LocalSize.x > 0.0f ? m_CachedGeometry.LocalSize.x : 100.0f;
    float Height = CalculateTotalHeight();
    return Vector2(Width, Height);
}

template<typename ItemType>
void SListView<ItemType>::ArrangeChildren(const FGeometry& allotted, std::vector<FArrangedWidget>& out) const
{
    if (!DataSource || m_VisibleItems.empty())
        return;
    
    const float ViewWidth = allotted.LocalSize.x;
    const float ViewHeight = allotted.LocalSize.y;
    const float ViewTop = m_ScrollOffset;
    
    float Y = ViewTop;
    
    for (const FVisibleItem& VisibleItem : m_VisibleItems)
    {
        int32_t Index = VisibleItem.Index;
        std::shared_ptr<SWidget> Widget = VisibleItem.Widget;
        
        if (!Widget) continue;
        
        float ItemHeight = GetItemHeight(Index);
        
        // Skip if item is above visible area
        if (Y + ItemHeight <= ViewTop)
        {
            Y += ItemHeight;
            continue;
        }
        
        // Skip if item is below visible area
        if (Y >= ViewTop + ViewHeight)
            break;
        
        // Clamp to visible area
        float ItemTop = std::max(Y, ViewTop);
        float ItemBottom = std::min(Y + ItemHeight, ViewTop + ViewHeight);
        float VisibleItemHeight = ItemBottom - ItemTop;
        
        // Calculate local position within the allotted geometry
        float LocalY = ItemTop - ViewTop;
        
        // Handle variable height items
        if (Options.ItemHeight <= 0.0f)
        {
            // For variable height, use widget's desired size
            Vector2 Desired = Widget->GetDesiredSize();
            if (Desired.y > 0.0f)
            {
                // Cache the height
                if (static_cast<size_t>(Index) < m_CachedItemHeights.size())
                {
                    m_CachedItemHeights[static_cast<size_t>(Index)] = Desired.y;
                }
                else if (static_cast<size_t>(Index) == m_CachedItemHeights.size())
                {
                    m_CachedItemHeights.push_back(Desired.y);
                }
                
                ItemHeight = Desired.y;
                VisibleItemHeight = std::min(Desired.y, ViewHeight - (ItemTop - ViewTop));
            }
        }
        
        out.push_back({
            Widget,
            FGeometry(
                Vector2(allotted.AbsolutePosition.x, allotted.AbsolutePosition.y + LocalY),
                Vector2(ViewWidth, VisibleItemHeight)
            )
        });
        
        Y += ItemHeight;
    }
}

template<typename ItemType>
void SListView<ItemType>::OnPaint(const FPaintContext& ctx, const FGeometry& geom) const
{
    if (ctx.Renderer == nullptr)
        return;
    
    const float TotalHeight = CalculateTotalHeight();
    const float ViewHeight = geom.LocalSize.y;
    
    // Draw background
    if (!Options.BackgroundColor.IsZero())
    {
        ctx.Renderer->drawQuad(geom.ToRect(), Options.BackgroundColor);
    }
    
    // Draw scrollbar if content overflows
    if (TotalHeight > ViewHeight)
    {
        const UIRect Rect = geom.ToRect();
        const float ThumbHeight = ViewHeight * (ViewHeight / TotalHeight);
        const float MaxScroll = TotalHeight - ViewHeight;
        const float t = (MaxScroll > 0.0f) ? (m_ScrollOffset / MaxScroll) : 0.0f;
        const float ThumbY = Rect.y + t * (ViewHeight - ThumbHeight);
        
        ctx.Renderer->drawQuad(
            UIRect(Rect.x + Rect.w - Options.ScrollBarWidth, ThumbY, Options.ScrollBarWidth, ThumbHeight),
            Options.ScrollBarColor);
    }
    
    // Draw selection/hover highlights
    float Y = m_ScrollOffset;
    
    for (int32_t Index = 0; Index < GetNumItems(); ++Index)
    {
        float ItemHeight = GetItemHeight(Index);
        float ItemTop = Y;
        float ItemBottom = Y + ItemHeight;
        
        // Skip if item is not in visible area
        if (ItemBottom <= geom.LocalSize.y && ItemBottom < m_ScrollOffset)
        {
            Y += ItemHeight;
            continue;
        }
        if (ItemTop >= m_ScrollOffset + geom.LocalSize.y)
            break;
        
        // Check if this item should be highlighted
        bool bIsSelected = std::find(m_SelectedIndices.begin(), m_SelectedIndices.end(), Index) != m_SelectedIndices.end();
        bool bIsHovered = (m_HoveredIndex == Index);
        
        if (bIsSelected || bIsHovered)
        {
            UIColor HighlightColor = bIsSelected ? Options.SelectedBackgroundColor : Options.HoveredBackgroundColor;
            
            // Adjust to visible area
            float HighlightTop = std::max(ItemTop, m_ScrollOffset);
            float HighlightBottom = std::min(ItemBottom, m_ScrollOffset + geom.LocalSize.y);
            float HighlightHeight = HighlightBottom - HighlightTop;
            
            if (HighlightHeight > 0.0f)
            {
                ctx.Renderer->drawQuad(
                    UIRect(geom.AbsolutePosition.x, geom.AbsolutePosition.y + (HighlightTop - m_ScrollOffset),
                           geom.LocalSize.x, HighlightHeight),
                    HighlightColor);
            }
        }
        
        Y += ItemHeight;
    }
}

template<typename ItemType>
FReply SListView<ItemType>::OnMouseWheel(const Vector2& /*pos*/, float delta)
{
    const float TotalHeight = CalculateTotalHeight();
    const float ViewHeight = m_CachedGeometry.LocalSize.y;
    const float MaxScroll = (TotalHeight > ViewHeight) ? (TotalHeight - ViewHeight) : 0.0f;
    
    m_ScrollOffset -= delta * Options.ItemHeight * 3.0f;  // Scroll 3 items per wheel notch
    ClampScrollOffset();
    
    return FReply::Handled();
}

template<typename ItemType>
FReply SListView<ItemType>::OnMouseButtonDown(const Vector2& pos, int button)
{
    if (button != 0)  // Left mouse button
        return FReply::Unhandled();
    
    const float TotalHeight = CalculateTotalHeight();
    const UIRect Rect = m_CachedGeometry.ToRect();
    const float ViewHeight = Rect.h;
    
    if (TotalHeight <= ViewHeight || ViewHeight <= 0.0f)
        return FReply::Unhandled();
    
    // Check if click is in scrollbar area
    const float Band = std::max(Options.ScrollBarWidth, 12.0f);
    if (pos.x < Rect.x + Rect.w - Band)
        return FReply::Unhandled();
    
    const float ThumbHeight = ViewHeight * (ViewHeight / TotalHeight);
    const float MaxScroll = TotalHeight - ViewHeight;
    const float t = (MaxScroll > 0.0f) ? (m_ScrollOffset / MaxScroll) : 0.0f;
    const float ThumbY = Rect.y + t * (ViewHeight - ThumbHeight);
    
    if (pos.y >= ThumbY && pos.y <= ThumbY + ThumbHeight)
    {
        m_DragGrab = pos.y - ThumbY;
    }
    else
    {
        m_DragGrab = ThumbHeight * 0.5f;
        ApplyThumbDrag(pos.y, Rect, TotalHeight);
    }
    
    m_DraggingThumb = true;
    return FReply::Handled().CaptureMouse(this);
}

template<typename ItemType>
void SListView<ItemType>::OnMouseMove(const Vector2& pos)
{
    // Handle scrollbar dragging
    if (m_DraggingThumb)
    {
        ApplyThumbDrag(pos.y, m_CachedGeometry.ToRect(), CalculateTotalHeight());
        return;
    }
    
    // Update hover state
    int32_t NewHoveredIndex = HitTestIndex(pos - m_CachedGeometry.AbsolutePosition);
    if (NewHoveredIndex != m_HoveredIndex)
    {
        // Clear previous hover
        if (m_HoveredIndex >= 0 && m_HoveredIndex < static_cast<int32_t>(m_VisibleItems.size()))
        {
            // Could trigger hover leave event
        }
        
        m_HoveredIndex = NewHoveredIndex;
        
        // Trigger hover enter event
        if (m_HoveredIndex >= 0)
        {
            // Could trigger hover enter event
        }
    }
}

template<typename ItemType>
FReply SListView<ItemType>::OnMouseButtonUp(const Vector2& /*pos*/, int button)
{
    if (button != 0)
        return FReply::Unhandled();
    
    m_DraggingThumb = false;
    
    // Handle selection on click
    if (m_HoveredIndex >= 0 && m_HoveredIndex < GetNumItems())
    {
        SetSelection(m_HoveredIndex);
    }
    
    return FReply::Handled().ReleaseMouseCapture();
}

template<typename ItemType>
void SListView<ItemType>::OnMouseCaptureLost()
{
    m_DraggingThumb = false;
}

template<typename ItemType>
void SListView<ItemType>::ApplyThumbDrag(float PointerY, const UIRect& Rect, float TotalHeight)
{
    const float ViewHeight = Rect.h;
    if (TotalHeight <= ViewHeight || ViewHeight <= 0.0f)
        return;
    
    const float ThumbHeight = ViewHeight * (ViewHeight / TotalHeight);
    const float Track = ViewHeight - ThumbHeight;
    if (Track <= 0.0f)
    {
        m_ScrollOffset = 0.0f;
        return;
    }
    
    float ThumbY = PointerY - m_DragGrab - Rect.y;
    if (ThumbY < 0.0f) ThumbY = 0.0f;
    if (ThumbY > Track) ThumbY = Track;
    
    m_ScrollOffset = (ThumbY / Track) * (TotalHeight - ViewHeight);
    ClampScrollOffset();
}

// ============================================================================
// Explicit template instantiations
// ============================================================================

// Common item types used in the engine
template class SListView<int32_t>;
template class SListView<std::string>;
template class SListView<std::shared_ptr<void>>;

}  // namespace ZSlate

#include "ZSlate/Widgets/Views/SListView.h"

#include <algorithm>

namespace ZSlate
{

// ============================================================================
// SListView Implementation
// ============================================================================

Vector2 SListView<int32_t>::ComputeDesiredSize() const
{
    // Estimate based on visible items
    float Height = std::max(m_CachedGeometry.LocalSize.y, GetMaxScrollOffset() + m_CachedGeometry.LocalSize.y);
    return Vector2(300.0f, Height);
}

void SListView<int32_t>::ArrangeChildren(const FGeometry& allotted, std::vector<FArrangedWidget>& out) const
{
    UpdateVisibleItems();
    
    const UIRect Rect = allotted.ToRect();
    float X = Rect.x + Options.Padding.Left;
    float Y = Rect.y + Options.Padding.Top - m_ScrollOffset;
    
    for (const auto& VisibleItem : m_VisibleItems)
    {
        if (!VisibleItem.Widget) continue;
        
        float ItemHeight = GetItemHeight(VisibleItem.Index);
        
        FArrangedWidget Arranged;
        Arranged.Widget = VisibleItem.Widget;
        Arranged.Geometry = FGeometry(UIRect(X, Y, Rect.width - Options.Padding.Left - Options.Padding.Right, ItemHeight), allotted);
        
        out.push_back(Arranged);
        Y += ItemHeight;
    }
}

void SListView<int32_t>::OnPaint(const FPaintContext& ctx, const FGeometry& geom) const
{
    if (ctx.Renderer == nullptr)
        return;
    
    const UIRect Rect = geom.ToRect();
    
    // Draw background
    if (Options.BackgroundColor.w > 0.0f)
    {
        ctx.Renderer->drawRect(Rect, Options.BackgroundColor);
    }
    
    // Update visible items
    UpdateVisibleItems();
    
    // Draw selection and hover backgrounds
    const float ViewTop = m_ScrollOffset;
    
    for (const auto& VisibleItem : m_VisibleItems)
    {
        float ItemTop = ViewTop + GetItemTopPosition(VisibleItem.Index);
        float ItemBottom = ItemTop + GetItemHeight(VisibleItem.Index);
        
        bool bIsSelected = std::find(m_SelectedIndices.begin(), m_SelectedIndices.end(), VisibleItem.Index) != m_SelectedIndices.end();
        bool bIsHovered = VisibleItem.Index == m_HoveredIndex;
        
        if (bIsSelected)
        {
            ctx.Renderer->drawRect(UIRect(Rect.x + Options.Padding.Left, Rect.y + ItemTop,
                                           Rect.width - Options.Padding.Left - Options.Padding.Right,
                                           GetItemHeight(VisibleItem.Index)),
                                    Options.SelectedBackgroundColor);
        }
        else if (bIsHovered)
        {
            ctx.Renderer->drawRect(UIRect(Rect.x + Options.Padding.Left, Rect.y + ItemTop,
                                           Rect.width - Options.Padding.Left - Options.Padding.Right,
                                           GetItemHeight(VisibleItem.Index)),
                                    Options.HoveredBackgroundColor);
        }
    }
    
    // Draw scrollbar if content overflows
    float TotalHeight = CalculateTotalHeight();
    float ViewHeight = Rect.height;
    
    if (TotalHeight > ViewHeight)
    {
        float ScrollbarX = Rect.x + Rect.width - Options.ScrollBarWidth;
        float ScrollbarY = Rect.y + Options.Padding.Top;
        float ScrollbarH = Rect.height - Options.Padding.Top - Options.Padding.Bottom;
        
        // Track background
        ctx.Renderer->drawRect(UIRect(ScrollbarX, ScrollbarY, Options.ScrollBarWidth, ScrollbarH),
                               UIColor(0.2f, 0.2f, 0.2f, 0.5f));
        
        // Thumb
        float ThumbSize = (ViewHeight / TotalHeight) * ScrollbarH;
        float ThumbPos = (m_ScrollOffset / TotalHeight) * ScrollbarH;
        
        ctx.Renderer->drawRect(UIRect(ScrollbarX, ScrollbarY + ThumbPos, Options.ScrollBarWidth, ThumbSize),
                               Options.ScrollBarColor);
    }
}

FReply SListView<int32_t>::OnMouseWheel(const Vector2& pos, float delta)
{
    m_ScrollOffset -= delta * 24.0f;
    ClampScrollOffset();
    return FReply::Handled();
}

FReply SListView<int32_t>::OnMouseButtonDown(const Vector2& pos, int button)
{
    if (button == 0) // Left button
    {
        // Check if click is on scrollbar
        float TotalHeight = CalculateTotalHeight();
        float ViewHeight = m_CachedGeometry.LocalSize.y;
        
        if (TotalHeight > ViewHeight)
        {
            float ScrollbarX = m_CachedGeometry.LocalSize.x - Options.ScrollBarWidth;
            float ScrollbarY = Options.Padding.Top;
            float ScrollbarH = m_CachedGeometry.LocalSize.y - Options.Padding.Top - Options.Padding.Bottom;
            
            if (pos.x >= ScrollbarX && pos.y >= ScrollbarY && pos.x <= ScrollbarX + Options.ScrollBarWidth && pos.y <= ScrollbarY + ScrollbarH)
            {
                m_DraggingThumb = true;
                m_DragGrab = pos.y - (m_ScrollOffset / TotalHeight) * ScrollbarH;
                return FReply::Handled();
            }
        }
        
        // Click on item
        int32_t Index = HitTestIndex(pos - m_CachedGeometry.AbsolutePosition);
        if (Index >= 0)
        {
            SetSelection(Index);
            m_HoveredIndex = Index;
        }
        
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void SListView<int32_t>::OnMouseMove(const Vector2& pos)
{
    const Vector2 LocalPos = pos - m_CachedGeometry.AbsolutePosition;
    
    // Handle scrollbar dragging
    if (m_DraggingThumb)
    {
        float TotalHeight = CalculateTotalHeight();
        float ViewHeight = m_CachedGeometry.LocalSize.y;
        
        if (TotalHeight > ViewHeight)
        {
            float ScrollbarH = m_CachedGeometry.LocalSize.y - Options.Padding.Top - Options.Padding.Bottom;
            float NewOffset = (LocalPos.y - m_DragGrab) / ScrollbarH * TotalHeight;
            
            if (NewOffset < 0.0f) NewOffset = 0.0f;
            if (NewOffset > TotalHeight - ViewHeight) NewOffset = TotalHeight - ViewHeight;
            
            m_ScrollOffset = NewOffset;
        }
        return;
    }
    
    // Update hover
    int32_t NewHoveredIndex = HitTestIndex(LocalPos);
    if (NewHoveredIndex != m_HoveredIndex)
    {
        m_HoveredIndex = NewHoveredIndex;
    }
}

FReply SListView<int32_t>::OnMouseButtonUp(const Vector2& pos, int button)
{
    if (button == 0)
    {
        m_DraggingThumb = false;
    }
    return FReply::Handled();
}

void SListView<int32_t>::OnMouseCaptureLost()
{
    m_DraggingThumb = false;
}

template class SListView<int32_t>;

}  // namespace ZSlate

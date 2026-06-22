#pragma once

#include "ZSlate/Core/ZSlateTypes.h"
#include "ZSlate/Widgets/SWidget.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace ZSlate
{
// ============================================================================
// ListView Types
// ============================================================================

// Forward declaration
template<typename ItemType>
class SListView;

// Item data source interface - provides list data to SListView
template<typename ItemType>
struct IListViewDataSource
{
    virtual ~IListViewDataSource() = default;
    
    // Get total number of items
    virtual int32_t GetNumItems() const = 0;
    
    // Get item at index
    virtual ItemType GetItem(int32_t Index) const = 0;
};

// Simple data source using a vector
template<typename ItemType>
class TSimpleListViewDataSource : public IListViewDataSource<ItemType>
{
public:
    explicit TSimpleListViewDataSource(std::vector<ItemType> InItems)
        : Items(std::move(InItems)) {}
    
    int32_t GetNumItems() const override { return static_cast<int32_t>(Items.size()); }
    ItemType GetItem(int32_t Index) const override
    {
        if (Index >= 0 && Index < static_cast<int32_t>(Items.size()))
            return Items[static_cast<size_t>(Index)];
        return ItemType{};
    }
    
    void SetItems(std::vector<ItemType> InItems) { Items = std::move(InItems); }
    const std::vector<ItemType>& GetItems() const { return Items; }

private:
    std::vector<ItemType> Items;
};

// Item widget generator callback
template<typename ItemType>
using FOnGenerateItemWidget = std::function<std::shared_ptr<SWidget>(const ItemType& Item, int32_t Index, const std::shared_ptr<SListView<ItemType>>& ListView)>;

// Selection changed callback
template<typename ItemType>
using FOnSelectionChanged = std::function<void(const std::vector<int32_t>& SelectedIndices)>;

// ============================================================================
// SListView - Virtualized list view widget (UE SListView analogue)
// ============================================================================

template<typename ItemType>
class SListView : public SWidget
{
public:
    // Configuration options
    struct FListViewOptions
    {
        float ItemHeight {24.0f};                    // Fixed height for all items (0 = variable)
        float ScrollBarWidth {6.0f};                  // Scrollbar width
        FMargin Padding {4.0f};                       // Padding around content
        UIColor ScrollBarColor {0.40f, 0.40f, 0.46f, 0.85f};
        UIColor SelectedBackgroundColor {0.2f, 0.45f, 0.75f, 0.8f};
        UIColor HoveredBackgroundColor {0.3f, 0.3f, 0.3f, 0.5f};
        UIColor BackgroundColor {0.1f, 0.1f, 0.1f, 0.0f};
    };

    SListView() = default;
    explicit SListView(const FListViewOptions& InOptions) : Options(InOptions) {}

    // Set data source
    void SetDataSource(const std::shared_ptr<IListViewDataSource<ItemType>>& InDataSource)
    {
        DataSource = InDataSource;
        m_ScrollOffset = 0.0f;
        m_CachedItemHeights.clear();
        m_CachedTotalHeight = 0.0f;
        m_VisibleItems.clear();
        m_SelectedIndices.clear();
        m_HoveredIndex = -1;
    }

    // Set item widget generator
    void SetOnGenerateItemWidget(FOnGenerateItemWidget<ItemType> InGenerator)
    {
        OnGenerateItemWidget = std::move(InGenerator);
    }

    // Set selection changed callback
    void SetOnSelectionChanged(FOnSelectionChanged<ItemType> InCallback)
    {
        OnSelectionChanged = std::move(InCallback);
    }

    // Selection management
    void SetSelection(int32_t Index, bool bClearOtherSelections = true)
    {
        if (bClearOtherSelections)
            m_SelectedIndices.clear();
        
        if (Index >= 0 && Index < GetNumItems() && 
            std::find(m_SelectedIndices.begin(), m_SelectedIndices.end(), Index) == m_SelectedIndices.end())
        {
            m_SelectedIndices.push_back(Index);
            std::sort(m_SelectedIndices.begin(), m_SelectedIndices.end());
            
            if (OnSelectionChanged)
                OnSelectionChanged(m_SelectedIndices);
        }
    }

    void SetSelection(const std::vector<int32_t>& Indices)
    {
        m_SelectedIndices = Indices;
        std::sort(m_SelectedIndices.begin(), m_SelectedIndices.end());
        
        if (OnSelectionChanged)
            OnSelectionChanged(m_SelectedIndices);
    }

    void ClearSelection()
    {
        if (!m_SelectedIndices.empty())
        {
            m_SelectedIndices.clear();
            if (OnSelectionChanged)
                OnSelectionChanged(m_SelectedIndices);
        }
    }

    const std::vector<int32_t>& GetSelectedIndices() const { return m_SelectedIndices; }

    // Scroll to item
    void ScrollToItem(int32_t Index)
    {
        if (!DataSource) return;
        
        const int32_t NumItems = GetNumItems();
        if (Index < 0 || Index >= NumItems) return;
        
        float ItemTop = GetItemTopPosition(Index);
        float ItemBottom = ItemTop + GetItemHeight(Index);
        float ViewHeight = m_CachedGeometry.LocalSize.y;
        
        if (ItemTop < m_ScrollOffset)
            m_ScrollOffset = ItemTop;
        else if (ItemBottom > m_ScrollOffset + ViewHeight)
            m_ScrollOffset = ItemBottom - ViewHeight;
        
        ClampScrollOffset();
    }

    // Get number of items
    int32_t GetNumItems() const
    {
        return DataSource ? DataSource->GetNumItems() : 0;
    }

    // Get item height (with caching support)
    float GetItemHeight(int32_t Index) const
    {
        if (Options.ItemHeight > 0.0f)
            return Options.ItemHeight;
        
        // Cache miss - estimate from average or use default
        if (Index < 0 || Index >= static_cast<int32_t>(m_CachedItemHeights.size()))
        {
            if (!m_CachedItemHeights.empty())
                return m_AverageItemHeight;
            return 24.0f;  // Default estimate
        }
        
        return m_CachedItemHeights[static_cast<size_t>(Index)];
    }

    // Children interface
    int GetChildCount() const override
    {
        return static_cast<int>(m_VisibleItems.size());
    }

    std::shared_ptr<SWidget> GetChildAt(int Index) const override
    {
        if (Index < 0 || Index >= static_cast<int>(m_VisibleItems.size()))
            return nullptr;
        return m_VisibleItems[static_cast<size_t>(Index)].Widget;
    }

    // Layout
    Vector2 ComputeDesiredSize() const override;
    void ArrangeChildren(const FGeometry& allotted, std::vector<FArrangedWidget>& out) const override;

    // Painting
    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override;

    // Input handling
    FReply OnMouseWheel(const Vector2& pos, float delta) override;
    FReply OnMouseButtonDown(const Vector2& pos, int button) override;
    void OnMouseMove(const Vector2& pos) override;
    FReply OnMouseButtonUp(const Vector2& pos, int button) override;
    void OnMouseCaptureLost() override;

    // Clipping
    bool ClipsContent() const override { return true; }

private:
    // Calculate the Y position (from top) of item at index
    float GetItemTopPosition(int32_t Index) const
    {
        float Pos = 0.0f;
        for (int32_t i = 0; i < Index; ++i)
        {
            Pos += GetItemHeight(i);
        }
        return Pos;
    }

    // Calculate total content height
    float CalculateTotalHeight() const
    {
        const int32_t NumItems = GetNumItems();
        float Total = 0.0f;
        for (int32_t i = 0; i < NumItems; ++i)
        {
            Total += GetItemHeight(i);
        }
        return Total;
    }

    // Update average item height
    void UpdateAverageHeight()
    {
        if (m_CachedItemHeights.empty())
        {
            m_AverageItemHeight = Options.ItemHeight > 0.0f ? Options.ItemHeight : 24.0f;
            return;
        }
        
        float Sum = 0.0f;
        for (float H : m_CachedItemHeights)
            Sum += H;
        m_AverageItemHeight = Sum / static_cast<float>(m_CachedItemHeights.size());
    }

    // Create or reuse item widget
    std::shared_ptr<SWidget> CreateItemWidget(int32_t Index)
    {
        if (!DataSource || !OnGenerateItemWidget)
            return nullptr;
        
        const ItemType& Item = DataSource->GetItem(Index);
        return OnGenerateItemWidget(Item, Index, shared_from_this());
    }

    // Update visible items based on scroll offset
    void UpdateVisibleItems()
    {
        m_VisibleItems.clear();
        
        const int32_t NumItems = GetNumItems();
        if (NumItems == 0) return;
        
        float ViewTop = m_ScrollOffset;
        float ViewBottom = m_ScrollOffset + m_CachedGeometry.LocalSize.y;
        
        int32_t FirstVisible = 0;
        int32_t LastVisible = NumItems - 1;
        
        // Find first visible item using binary search for efficiency
        FirstVisible = FindFirstVisibleItem(ViewTop);
        LastVisible = FindLastVisibleItem(ViewBottom);
        
        // Clamp
        if (FirstVisible < 0) FirstVisible = 0;
        if (LastVisible >= NumItems) LastVisible = NumItems - 1;
        
        // Create widgets for visible items
        for (int32_t i = FirstVisible; i <= LastVisible; ++i)
        {
            auto Widget = CreateItemWidget(i);
            if (Widget)
            {
                m_VisibleItems.push_back(FVisibleItem{i, Widget});
            }
        }
    }

    // Binary search to find first visible item
    int32_t FindFirstVisibleItem(float ViewTop) const
    {
        const int32_t NumItems = GetNumItems();
        if (NumItems == 0) return 0;
        
        int32_t Low = 0;
        int32_t High = NumItems - 1;
        
        while (Low < High)
        {
            int32_t Mid = (Low + High) / 2;
            float ItemTop = GetItemTopPosition(Mid);
            
            if (ItemTop < ViewTop)
                Low = Mid + 1;
            else
                High = Mid;
        }
        
        // Check if we need to go back
        if (Low > 0)
        {
            float PrevTop = GetItemTopPosition(Low - 1);
            if (PrevTop + GetItemHeight(Low - 1) > ViewTop)
                Low--;
        }
        
        return Low;
    }

    // Binary search to find last visible item
    int32_t FindLastVisibleItem(float ViewBottom) const
    {
        const int32_t NumItems = GetNumItems();
        if (NumItems == 0) return 0;
        
        int32_t Low = 0;
        int32_t High = NumItems - 1;
        
        while (Low < High)
        {
            int32_t Mid = (Low + High + 1) / 2;
            float ItemTop = GetItemTopPosition(Mid);
            float ItemBottom = ItemTop + GetItemHeight(Mid);
            
            if (ItemBottom <= ViewBottom)
                Low = Mid;
            else
                High = Mid - 1;
        }
        
        return Low;
    }

    // Clamp scroll offset within valid range
    void ClampScrollOffset()
    {
        float MaxScroll = GetMaxScrollOffset();
        if (m_ScrollOffset < 0.0f)
            m_ScrollOffset = 0.0f;
        if (m_ScrollOffset > MaxScroll)
            m_ScrollOffset = MaxScroll;
    }

    float GetMaxScrollOffset() const
    {
        float Total = CalculateTotalHeight();
        float View = m_CachedGeometry.LocalSize.y;
        return (Total > View) ? (Total - View) : 0.0f;
    }

    // Hit test: find item index at position
    int32_t HitTestIndex(const Vector2& LocalPos) const
    {
        float ViewTop = m_ScrollOffset;
        float Y = ViewTop;
        
        const int32_t NumItems = GetNumItems();
        for (int32_t i = 0; i < NumItems; ++i)
        {
            float ItemHeight = GetItemHeight(i);
            if (LocalPos.y >= Y && LocalPos.y < Y + ItemHeight)
                return i;
            Y += ItemHeight;
        }
        
        return -1;
    }

    // Visible item info
    struct FVisibleItem
    {
        int32_t Index;
        std::shared_ptr<SWidget> Widget;
    };

    FListViewOptions Options;
    std::shared_ptr<IListViewDataSource<ItemType>> DataSource;
    FOnGenerateItemWidget<ItemType> OnGenerateItemWidget;
    FOnSelectionChanged<ItemType> OnSelectionChanged;
    
    mutable float m_ScrollOffset {0.0f};
    mutable std::vector<FVisibleItem> m_VisibleItems;
    mutable std::vector<int32_t> m_SelectedIndices;
    mutable int32_t m_HoveredIndex {-1};
    mutable bool m_DraggingThumb {false};
    mutable float m_DragGrab {0.0f};
    
    // Height cache for variable-height items
    mutable std::vector<float> m_CachedItemHeights;
    mutable float m_AverageItemHeight {24.0f};
    mutable float m_CachedTotalHeight {0.0f};
};

// ============================================================================
// Non-template helper functions
// ============================================================================

// Create a simple list view with vector data source
template<typename ItemType>
std::shared_ptr<SListView<ItemType>> CreateListView(
    std::vector<ItemType> Items,
    FOnGenerateItemWidget<ItemType> Generator,
    const typename SListView<ItemType>::FListViewOptions& Options = typename SListView<ItemType>::FListViewOptions{})
{
    auto ListView = std::make_shared<SListView<ItemType>>(Options);
    ListView->SetDataSource(std::make_shared<TSimpleListViewDataSource<ItemType>>(std::move(Items)));
    ListView->SetOnGenerateItemWidget(std::move(Generator));
    return ListView;
}

}  // namespace ZSlate

#pragma once

#include "ZSlate/Widgets/SWidget.h"
#include "ZSlate/Application/SlateInput.h"
#include "ZSlate/Core/SlateClipboard.h"

#include <functional>
#include <string>
#include <vector>

namespace ZSlate
{
// ============================================================================
// Text Types
// ============================================================================

// Text location (line + column offset)
struct FTextLocation
{
    int32_t LineIndex {0};     // 0-based line index
    int32_t CharIndex {0};     // 0-based character index within line
    
    FTextLocation() = default;
    FTextLocation(int32_t Line, int32_t Char) : LineIndex(Line), CharIndex(Char) {}
    
    bool operator==(const FTextLocation& Other) const
    {
        return LineIndex == Other.LineIndex && CharIndex == Other.CharIndex;
    }
    bool operator!=(const FTextLocation& Other) const { return !(*this == Other); }
};

// Text selection (start + end locations)
struct FTextSelection
{
    FTextLocation Start;
    FTextLocation End;
    
    FTextSelection() = default;
    FTextSelection(const FTextLocation& InStart, const FTextLocation& InEnd)
        : Start(InStart), End(InEnd) {}
    
    bool IsValid() const { return Start != End; }
    bool IsEmpty() const { return Start == End; }
    
    // Get normalized selection (Start < End)
    void Normalize()
    {
        if (Start.LineIndex > End.LineIndex ||
            (Start.LineIndex == End.LineIndex && Start.CharIndex > End.CharIndex))
        {
            std::swap(Start, End);
        }
    }
};

// Text line info for layout
class FTextLineInfo
{
public:
    std::string Text;
    float Height {0.0f};
    float Baseline {0.0f};  // Y position of baseline relative to line top
    
    FTextLineInfo() = default;
    explicit FTextLineInfo(std::string InText) : Text(std::move(InText)) {}
};

// ============================================================================
// SMultiLineEditableText - Multi-line text editor (UE SMultiLineEditableText analogue)
// ============================================================================

class SMultiLineEditableText : public SWidget
{
public:
    // Configuration options
    struct FTextOptions
    {
        float FontSize {14.0f};
        float LineSpacing {1.2f};
        float MinWidth {100.0f};
        float MinHeight {60.0f};
        FMargin Padding {6.0f, 6.0f};
        
        UIColor BackgroundColor {0.08f, 0.08f, 0.10f, 1.0f};
        UIColor BorderColor {0.35f, 0.36f, 0.40f, 1.0f};
        UIColor FocusBorderColor {0.36f, 0.62f, 0.96f, 1.0f};
        UIColor TextColor {0.92f, 0.93f, 0.96f, 1.0f};
        UIColor HintColor {0.45f, 0.46f, 0.52f, 1.0f};
        UIColor SelectionColor {0.25f, 0.50f, 0.90f, 0.40f};
        UIColor CursorColor {1.0f, 1.0f, 1.0f, 1.0f};
        UIColor ScrollBarColor {0.40f, 0.40f, 0.46f, 0.85f};
        float ScrollBarWidth {12.0f};
        UIColor CompositionColor {0.25f, 0.75f, 0.40f, 0.40f};
        
        bool AutoWrap {true};           // Auto-wrap long lines
        bool ReadOnly {false};          // Read-only mode
        bool IsPassword {false};        // Password mode (display *** instead of text)
        bool ShowScrollBar {true};      // Show scroll bar
    };

    SMultiLineEditableText() = default;
    explicit SMultiLineEditableText(const FTextOptions& InOptions) : Options(InOptions) {}

    // Set text content
    void SetText(const std::string& InText);
    const std::string& GetText() const { return TextContent; }
    
    // Hint text (placeholder)
    void SetHintText(const std::string& InHint) { HintText = InHint; }
    const std::string& GetHintText() const { return HintText; }
    
    // Selection management
    void SetSelection(const FTextLocation& Start, const FTextLocation& End);
    void SelectAll();
    void ClearSelection() { Selection = FTextSelection(); }
    const FTextSelection& GetSelection() const { return Selection; }
    
    // Cursor management
    void SetCursorLocation(const FTextLocation& Location);
    FTextLocation GetCursorLocation() const { return CursorLocation; }
    
    // Scroll management
    void ScrollToCursor();
    void ScrollToLine(int32_t LineIndex);
    void ClampScrollOffset();
    
    // Text modification
    void InsertTextAtCursor(const std::string& Text);
    void DeleteSelection();
    void ReplaceSelection(const std::string& Text);
    
    // Clipboard operations
    void CopySelection();
    void CutSelection();
    void PasteText();
    
    // Text change callback
    using FOnTextChanged = std::function<void(const std::string&)>;
    FOnTextChanged OnTextChanged;
    
    // Children interface
    int GetChildCount() const override { return 0; }  // Leaf widget
    std::shared_ptr<SWidget> GetChildAt(int /*index*/) const override { return nullptr; }

    // Layout
    Vector2 ComputeDesiredSize() const override;
    void ArrangeChildren(const FGeometry& /*allotted*/, std::vector<FArrangedWidget>& /*out*/) const override {}

    // Painting
    void OnPaint(const FPaintContext& ctx, const FGeometry& geom) const override;

    // Input handling
    void OnMouseEnter() override;
    void OnMouseLeave() override;
    void OnMouseMove(const Vector2& pos) override;
    FReply OnMouseButtonDown(const Vector2& pos, int button) override;
    FReply OnMouseButtonUp(const Vector2& pos, int button) override;
    void OnMouseCaptureLost() override;
    FReply OnMouseWheel(const Vector2& pos, float delta) override;
    
    bool SupportsKeyboardFocus() const override { return !Options.ReadOnly; }
    void OnFocusReceived() override;
    void OnFocusLost() override;
    void OnKeyChar(unsigned int codepoint) override;
    FReply OnKeyDown(EKey key) override;
    ECursorType GetCursor() const override;

    // Clipping
    bool ClipsContent() const override { return true; }
    
    // Configuration options (public for easy access)
    FTextOptions Options;
    
    // Text utilities (public for easy access)
    std::vector<std::string> SplitIntoLines(const std::string& Text) const;
    std::string JoinLines(const std::vector<std::string>& Lines) const;

private:
    
    // UTF-8 utilities
    size_t GetPrevCharStart(size_t pos) const;
    size_t GetNextCharStart(size_t pos) const;
    int32_t Utf8ToCharIndex(size_t BytePos, const std::string& Line) const;
    size_t CharIndexToUtf8(int32_t CharIndex, const std::string& Line) const;
    
    // Line info
    const FTextLineInfo& GetLineInfo(int32_t LineIndex) const;
    int32_t GetNumLines() const { return static_cast<int32_t>(Lines.size()); }
    
    // Layout calculations
    float GetLineHeight(int32_t LineIndex) const;
    float GetLineTopY(int32_t LineIndex) const;
    float GetLineBottomY(int32_t LineIndex) const;
    float GetContentHeight() const;
    float GetMaxScrollOffset() const;
    
    // Hit testing
    FTextLocation HitTestLocation(const Vector2& LocalPos) const;
    
    // Selection utilities
    std::string GetSelectedText() const;
    void DeleteRange(const FTextLocation& Start, const FTextLocation& End);
    
    // UTF-8 utilities
    void AppendUtf8(std::string& s, unsigned int cp) const;
    
    // Cursor utilities
    void MoveCursorHome(bool bCtrl = false);
    void MoveCursorEnd(bool bCtrl = false);
    void MoveCursorLeft(bool bShift = false, bool bCtrl = false);
    void MoveCursorRight(bool bShift = false, bool bCtrl = false);
    void MoveCursorUp(bool bShift = false);
    void MoveCursorDown(bool bShift = false);
    
    // Drawing helpers
    void DrawBackground(const FPaintContext& ctx, const FGeometry& geom) const;
    void DrawSelection(const FPaintContext& ctx, const FGeometry& geom) const;
    void DrawComposition(const FPaintContext& ctx, const FGeometry& geom) const;
    void DrawText(const FPaintContext& ctx, const FGeometry& geom) const;
    void DrawCursor(const FPaintContext& ctx, const FGeometry& geom) const;
    void DrawScrollBar(const FPaintContext& ctx, const FGeometry& geom) const;
    
    // Scrollbar
    float GetScrollBarThumbPosition() const;
    float GetScrollBarThumbSize() const;
    void ApplyThumbDrag(float PointerY, const UIRect& Rect);

private:
    std::string TextContent;
    std::string HintText;
    std::vector<FTextLineInfo> Lines;
    std::vector<std::string> LinesStrings;
    
    FTextLocation CursorLocation;
    FTextSelection Selection;
    
    float ScrollOffsetY {0.0f};
    float ScrollOffsetX {0.0f};
    
    bool m_HasFocus {false};
    bool m_DraggingThumb {false};
    float m_DragGrab {0.0f};
    
    // Composition (IME) support
    std::string CompositionText;
    int32_t CompositionCursorPos {0};
    
    // Blinking cursor
    mutable float m_CursorBlinkTime {0.0f};
    mutable bool m_CursorVisible {true};
    
    ISlateTextMeasurer* m_TextMeasurer {nullptr};
};

}  // namespace ZSlate

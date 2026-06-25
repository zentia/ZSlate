#pragma once

#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Core/SlateKeys.h"

#include <memory>
#include <vector>

namespace ZSlate
{
class SWidget;
struct FDragDropOperation;

// Routes mouse + keyboard input into a ZSlate widget tree (UE Slate
// FSlateApplication path, simplified). Drive it once per frame from the host
// panel with the live input state.
//
// CRITICAL: All stored widget references use std::weak_ptr<SWidget> (matching
// UE's TWeakPtr<SWidget> pattern in FWidgetPath). Before each use, we lock()
// and check validity. This prevents dangling-pointer crashes when widgets are
// destroyed between frames — no manual Reset() or root-change detection needed.
class SlateInputRouter
{
public:
    // Clear all hover/capture/focus state. Call when the tree is rebuilt so we
    // never dereference freed widgets.
    void Reset();

    // Drop hover-path pointers without clearing keyboard focus or mouse capture.
    // Use when only log-row widgets are destroyed (Console filter rebuild).
    void ClearHoverPath() { m_HoverPath.clear(); }

    SWidget* GetKeyboardFocusedWidget() const;

    // Set keyboard focus to a specific widget. The widget must have
    // SupportsKeyboardFocus() == true (checked by the setter).
    void SetKeyboardFocusWidget(SWidget* widget);

    // `right_down` lets widgets handle right-clicks (context menus). Optional so
    // existing call sites that only care about the left button compile unchanged.
    void ProcessMouse(const std::shared_ptr<SWidget>& root,
                      const Vector2& mouse_screen,
                      bool is_over_host,
                      bool left_down,
                      float wheel_delta,
                      bool right_down = false);

    void ProcessChar(unsigned int codepoint);
    void ProcessKey(EKey key);

    bool HasKeyboardFocus() const;

    // Modifier key state (static, set by host via SetModifierState, read by widgets)
    static bool IsCtrlDown() { return s_CtrlDown; }
    static bool IsShiftDown() { return s_ShiftDown; }
    static bool IsAltDown() { return s_AltDown; }

    // Call this from the host before ProcessKey/ProcessChar to update modifier state.
    static void SetModifierState(bool ctrl, bool shift, bool alt)
    {
        s_CtrlDown = ctrl;
        s_ShiftDown = shift;
        s_AltDown = alt;
    }

    // Programmatically move keyboard focus (e.g. onto a freshly-shown rename box)
    // or clear it. Safe to pass a widget that is part of the routed tree.
    void SetKeyboardFocus(SWidget* widget);
    void ClearKeyboardFocus();

    // True while a drag-drop gesture is in flight (a source returned an op and
    // the button is still held). Hosts can use this to draw a drag decorator.
    bool IsDragging() const { return m_DragOp != nullptr; }
    const std::shared_ptr<FDragDropOperation>& GetDragOperation() const { return m_DragOp; }

    // Deepest widget currently under the cursor (end of the hover path), or null
    // if nothing is hovered. Hosts use this to apply the widget's GetCursor().
    SWidget* GetHoveredWidget() const;

private:
    // Helper: lock a weak_ptr, return raw pointer or nullptr if expired.
    static SWidget* Lock(const std::weak_ptr<SWidget>& wp);

    // Helper: lock all non-expired weak_ptrs in a path, returning raw pointers.
    static std::vector<SWidget*> LockPath(const std::vector<std::weak_ptr<SWidget>>& path);

    void SetFocus(SWidget* widget);
    void EndDrag();

    // ---- Widget paths use weak_ptr (UE: TWeakPtr<SWidget>) ----
    // Destroyed widgets lock() to nullptr; loops safely skip expired entries.
    std::vector<std::weak_ptr<SWidget>> m_HoverPath;
    std::vector<std::weak_ptr<SWidget>> m_PressPath;

    // Captured / focused widgets also use weak_ptr for safety.
    std::weak_ptr<SWidget> m_Captured;
    std::weak_ptr<SWidget> m_Focused;
    std::weak_ptr<SWidget> m_RightCaptured;
    std::weak_ptr<SWidget> m_DragOverTarget;

    bool m_PrevLeftDown {false};
    bool m_PrevRightDown {false};

    // Drag-drop gesture state.
    Vector2 m_PressOrigin {0.0f, 0.0f};
    bool m_LeftPressed {false};
    std::shared_ptr<FDragDropOperation> m_DragOp;  // non-null while dragging

    // Static modifier key state (per-thread, set by ProcessKey)
    static thread_local bool s_CtrlDown;
    static thread_local bool s_ShiftDown;
    static thread_local bool s_AltDown;
};
}  // namespace ZSlate

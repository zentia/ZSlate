#pragma once

#include "ZSlate/Core/SlateGeometry.h"
#include "ZSlate/Core/ZSlateTypes.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ZSlate
{
// ============================================================================
// ISlateRenderer - Abstract rendering backend
// ============================================================================
// The host (engine or standalone app) provides an implementation of this
// interface. ZSlate never touches the concrete rendering API directly.

struct ISlateRenderer
{
    virtual ~ISlateRenderer() = default;

    // Drawing primitives
    virtual void drawQuad(const UIRect& rect, const UIColor& color) = 0;
    virtual void drawRect(const UIRect& rect, const UIColor& color, float thickness = 1.0f) = 0;
    virtual void drawConvexPoly(const Vector2* points, int count, const UIColor& color) = 0;
    virtual void drawRoundedRect(const UIRect& rect, float radius, const UIColor& color) = 0;
    virtual void drawTexturedQuad(const UIRect& rect, void* texture_handle, const UIColor& tint = Colors::White) = 0;
    virtual void drawBox(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint) = 0;
    virtual void drawBorder(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint) = 0;

    // Text
    // Overload 1: draw into a rect with alignment (used by most widgets)
    virtual void drawText(const UIRect& rect, const std::string& text, float font_size, const UIColor& color,
                          TextAnchor alignment = TextAnchor::MiddleLeft, TextWrapMode wrap = TextWrapMode::NoWrap,
                          void* font_handle = nullptr) = 0;
    // Overload 2: draw at position (legacy API)
    virtual void drawText(const std::string& text, const Vector2& pos, float font_size, const UIColor& color) = 0;
    virtual Vector2 measureText(const std::string& text, float font_size) const = 0;

    // Clipping
    virtual void pushClipRect(const UIRect& rect) = 0;
    virtual void popClipRect() = 0;

    // State
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void flush() = 0;
};

// ============================================================================
// FPaintContext - Everything a widget needs to emit draw calls
// ============================================================================

struct FPaintContext
{
    ISlateRenderer* Renderer {nullptr};
    // Monotonic paint depth; later used for clip-stack debugging / z-ordering.
    int LayerId {0};
};

// ============================================================================
// ISlateTextMeasurer - Layout-time text measurement
// ============================================================================
// ComputeDesiredSize() runs before painting, so it cannot reach a live
// UIRenderer frame; instead SlateApplication installs a measurer that
// forwards to ISlateRenderer::measureText (or the editor font atlas).
class ISlateTextMeasurer
{
public:
    virtual ~ISlateTextMeasurer() = default;
    virtual Vector2 Measure(const std::string& text, float font_size) const = 0;
};

// ============================================================================
// ISlateFontService - Font atlas management
// ============================================================================
// Host provides font loading and text measurement.
struct ISlateFontService
{
    virtual ~ISlateFontService() = default;

    // Load a font by name. Returns handle for text drawing.
    virtual void* loadFont(const std::string& font_path, float size) = 0;
    virtual void unloadFont(void* handle) = 0;

    // Measure text using the given font handle.
    virtual Vector2 measureText(void* font_handle, const std::string& text) const = 0;

    // Get default font handle (set by host).
    virtual void* getDefaultFont() const = 0;
};

// ============================================================================
// ISlatePlatform - All platform abstractions in one place
// ============================================================================
struct ISlatePlatform
{
    virtual ~ISlatePlatform() = default;

    // Renderer
    virtual ISlateRenderer* getRenderer() = 0;

    // Font service
    virtual ISlateFontService* getFontService() = 0;

    // Input (host pumps these each frame)
    virtual Vector2 getMousePosition() const = 0;
    virtual bool isMouseButtonDown(int button) const = 0;
    virtual bool isKeyDown(int key) const = 0;
    virtual float getTimeSeconds() const = 0;

    // Window (for DPI, etc.)
    virtual Vector2 getWindowSize() const = 0;
    virtual float getDpiScale() const { return 1.0f; }
};

// ============================================================================
// Global Platform Instance
// ============================================================================

void SetPlatform(ISlatePlatform* platform);
ISlatePlatform* GetPlatform();
}  // namespace ZSlate

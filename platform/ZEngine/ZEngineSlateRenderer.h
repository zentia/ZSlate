#pragma once

// Bridge between ZSlate's ISlateRenderer interface and ZEngine's UiGpuResources.
// This file lives OUTSIDE the ZSlate namespace and includes ZEngine headers,
// so it can only be used when linking against ZEngine.

#include "ZSlate/Core/SlatePaint.h"

// Forward declare ZEngine types to avoid including heavy headers
namespace Ui
{
class UiGpuResources;

}  // namespace Ui

namespace ZSlate
{
// ZEngine implementation of ISlateRenderer.
// Wraps UiGpuResources and translates ZSlate draw calls into ZEngine's batch system.
class ZEngineSlateRenderer : public ISlateRenderer
{
public:
    explicit ZEngineSlateRenderer(Ui::UiGpuResources* gpu_resources);
    ~ZEngineSlateRenderer() override = default;

    // ISlateRenderer implementation
    void drawQuad(const UIRect& rect, const UIColor& color) override;
    void drawRect(const UIRect& rect, const UIColor& color, float thickness = 1.0f) override;
    void drawConvexPoly(const Vector2* points, int count, const UIColor& color) override;
    void drawRoundedRect(const UIRect& rect, float radius, const UIColor& color) override;
    void drawTexturedQuad(const UIRect& rect, void* texture_handle, const UIColor& tint = Colors::White) override;
    void drawBox(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint) override;
    void drawBorder(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint) override;

    void drawText(const UIRect& rect, const std::string& text, float font_size, const UIColor& color,
                  TextAnchor alignment = TextAnchor::MiddleLeft, TextWrapMode wrap = TextWrapMode::NoWrap,
                  void* font_handle = nullptr) override;
    void drawText(const std::string& text, const Vector2& pos, float font_size, const UIColor& color) override;
    Vector2 measureText(const std::string& text, float font_size) const override;

    void pushClipRect(const UIRect& rect) override;
    void popClipRect() override;

    void beginFrame() override;
    void endFrame() override;
    void flush() override;

private:
    Ui::UiGpuResources* m_GpuResources;
};

}  // namespace ZSlate

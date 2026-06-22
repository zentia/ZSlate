#include "ZSlate/Backend/ZEngineSlateRenderer.h"

// Now we can include ZEngine headers
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/UI/Render/UiGpuResources.h"
#include "Runtime/UI/Render/UIRenderer.h"

namespace ZSlate
{
ZEngineSlateRenderer::ZEngineSlateRenderer(::UiGpuResources* gpu_resources, UIRenderer* renderer)
    : m_GpuResources(gpu_resources), m_Renderer(renderer)
{
}

void ZEngineSlateRenderer::drawQuad(const UIRect& rect, const UIColor& color)
{
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_color(color.x, color.y, color.z, color.w);
    m_Renderer->drawQuad(ui_rect, ui_color);
}

void ZEngineSlateRenderer::drawRect(const UIRect& rect, const UIColor& color, float thickness)
{
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_color(color.x, color.y, color.z, color.w);
    m_Renderer->drawRect(ui_rect, ui_color, thickness);
}

void ZEngineSlateRenderer::drawConvexPoly(const Vector2* points, int count, const UIColor& color)
{
    if (!m_Renderer || count < 3)
        return;

    // Convert ZSlate Vector2 to ZEngine Vector2
    std::vector<::Vector2> ui_points(count);
    for (int i = 0; i < count; ++i)
    {
        ui_points[i] = ::Vector2(points[i].x, points[i].y);
    }
    ::UIColor ui_color(color.x, color.y, color.z, color.w);
    m_Renderer->drawConvexPoly(ui_points.data(), count, ui_color);
}

void ZEngineSlateRenderer::drawRoundedRect(const UIRect& rect, float radius, const UIColor& color)
{
    // UIRenderer doesn't have drawRoundedRect, so we approximate with drawQuad
    // TODO: Implement proper rounded rect rendering
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_color(color.x, color.y, color.z, color.w);
    m_Renderer->drawQuad(ui_rect, ui_color);
}

void ZEngineSlateRenderer::drawTexturedQuad(const UIRect& rect, void* texture_handle, const UIColor& tint)
{
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_tint(tint.x, tint.y, tint.z, tint.w);
    m_Renderer->drawTexturedQuad(ui_rect, texture_handle, ui_tint);
}

void ZEngineSlateRenderer::drawBox(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint)
{
    // UIRenderer doesn't have drawBox, so we draw a textured quad for now
    // TODO: Implement proper 9-slice drawing
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_tint(tint.x, tint.y, tint.z, tint.w);
    m_Renderer->drawTexturedQuad(ui_rect, texture_handle, ui_tint);
}

void ZEngineSlateRenderer::drawBorder(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint)
{
    // UIRenderer doesn't have drawBorder, so we draw a rect outline for now
    // TODO: Implement proper border drawing
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_tint(tint.x, tint.y, tint.z, tint.w);
    m_Renderer->drawRect(ui_rect, ui_tint, 1.0f);
}

void ZEngineSlateRenderer::drawText(const UIRect& rect, const std::string& text, float font_size, const UIColor& color,
                                    TextAnchor alignment, TextWrapMode wrap, void* font_handle)
{
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    ::UIColor ui_color(color.x, color.y, color.z, color.w);
    ::TextAnchor ui_align = static_cast<::TextAnchor>(static_cast<int>(alignment));
    ::TextWrapMode ui_wrap = static_cast<::TextWrapMode>(static_cast<int>(wrap));

    // Convert font_handle back to ZEngine's font system handle if needed
    uint32_t font_id = font_handle ? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(font_handle)) : 0;

    m_Renderer->drawText(ui_rect, text, font_size, ui_color, ui_align, ui_wrap, nullptr);
}

void ZEngineSlateRenderer::drawText(const std::string& text, const Vector2& pos, float font_size, const UIColor& color)
{
    // Convert to rect-based call for simplicity
    drawText(UIRect(pos.x, pos.y, 0, 0), text, font_size, color, TextAnchor::MiddleLeft);
}

Vector2 ZEngineSlateRenderer::measureText(const std::string& text, float font_size) const
{
    if (!m_Renderer)
        return Vector2(0, font_size);

    ::Vector2 result = m_Renderer->measureText(text, font_size);
    return Vector2(result.x, result.y);
}

void ZEngineSlateRenderer::pushClipRect(const UIRect& rect)
{
    if (!m_Renderer)
        return;

    ::UIRect ui_rect(rect.x, rect.y, rect.w, rect.h);
    m_Renderer->pushClipRect(ui_rect);
}

void ZEngineSlateRenderer::popClipRect()
{
    if (!m_Renderer)
        return;

    m_Renderer->popClipRect();
}

void ZEngineSlateRenderer::beginFrame()
{
    if (m_Renderer)
        m_Renderer->beginFrame();
}

void ZEngineSlateRenderer::endFrame()
{
    if (m_Renderer)
        m_Renderer->endFrame();
}

void ZEngineSlateRenderer::flush()
{
    // UIRenderer doesn't have a flush method, so this is a no-op
    // If needed, we could call m_Renderer->endFrame() here, but that's not quite right
}

}  // namespace ZSlate

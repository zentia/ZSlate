#include "ZEngineSlateRenderer.h"

// Now we can include ZEngine headers
#include "Runtime/UI/Render/UiGpuResources.h"
#include "Runtime/UI/Render/UIRenderer.h"

namespace ZSlate
{
ZEngineSlateRenderer::ZEngineSlateRenderer(Ui::UiGpuResources* gpu_resources)
    : m_GpuResources(gpu_resources)
{
}

void ZEngineSlateRenderer::drawQuad(const UIRect& rect, const UIColor& color)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    m_GpuResources->m_Renderer->drawQuad(ui_rect, color);
}

void ZEngineSlateRenderer::drawRect(const UIRect& rect, const UIColor& color, float thickness)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    m_GpuResources->m_Renderer->drawRect(ui_rect, color, thickness);
}

void ZEngineSlateRenderer::drawConvexPoly(const Vector2* points, int count, const UIColor& color)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer || count < 3)
        return;

    // Convert ZSlate Vector2 to ZEngine Vector2
    std::vector<Ui::Vector2> ui_points(count);
    for (int i = 0; i < count; ++i)
    {
        ui_points[i] = Ui::Vector2(points[i].x, points[i].y);
    }
    m_GpuResources->m_Renderer->drawConvexPoly(ui_points, color);
}

void ZEngineSlateRenderer::drawRoundedRect(const UIRect& rect, float radius, const UIColor& color)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    m_GpuResources->m_Renderer->drawRoundedRect(ui_rect, radius, color);
}

void ZEngineSlateRenderer::drawTexturedQuad(const UIRect& rect, void* texture_handle, const UIColor& tint)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    m_GpuResources->m_Renderer->drawTexturedQuad(ui_rect, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(texture_handle)), tint);
}

void ZEngineSlateRenderer::drawBox(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    Ui::FMargin ui_margin(margin.Left, margin.Top, margin.Right, margin.Bottom);
    m_GpuResources->m_Renderer->drawBox(ui_rect, ui_margin, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(texture_handle)), tint);
}

void ZEngineSlateRenderer::drawBorder(const UIRect& rect, const FMargin& margin, void* texture_handle, const UIColor& tint)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    Ui::FMargin ui_margin(margin.Left, margin.Top, margin.Right, margin.Bottom);
    m_GpuResources->m_Renderer->drawBorder(ui_rect, ui_margin, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(texture_handle)), tint);
}

void ZEngineSlateRenderer::drawText(const UIRect& rect, const std::string& text, float font_size, const UIColor& color,
                                    TextAnchor alignment, TextWrapMode wrap, void* font_handle)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    Ui::TextAnchor ui_align = static_cast<Ui::TextAnchor>(static_cast<int>(alignment));
    Ui::TextWrapMode ui_wrap = static_cast<Ui::TextWrapMode>(static_cast<int>(wrap));

    // Convert font_handle back to ZEngine's font system handle if needed
    uint32_t font_id = font_handle ? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(font_handle)) : 0;

    m_GpuResources->m_Renderer->drawText(ui_rect, text, font_size, color, ui_align, ui_wrap, font_id);
}

void ZEngineSlateRenderer::drawText(const std::string& text, const Vector2& pos, float font_size, const UIColor& color)
{
    // Convert to rect-based call for simplicity
    drawText(UIRect(pos.x, pos.y, 0, 0), text, font_size, color, TextAnchor::MiddleLeft);
}

Vector2 ZEngineSlateRenderer::measureText(const std::string& text, float font_size) const
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return Vector2(0, font_size);

    return m_GpuResources->m_Renderer->measureText(text, font_size);
}

void ZEngineSlateRenderer::pushClipRect(const UIRect& rect)
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    Ui::UiRect ui_rect(rect.x, rect.y, rect.width, rect.height);
    m_GpuResources->m_Renderer->pushClipRect(ui_rect);
}

void ZEngineSlateRenderer::popClipRect()
{
    if (!m_GpuResources || !m_GpuResources->m_Renderer)
        return;

    m_GpuResources->m_Renderer->popClipRect();
}

void ZEngineSlateRenderer::beginFrame()
{
    if (m_GpuResources)
        m_GpuResources->beginFrame();
}

void ZEngineSlateRenderer::endFrame()
{
    if (m_GpuResources)
        m_GpuResources->endFrame();
}

void ZEngineSlateRenderer::flush()
{
    if (m_GpuResources)
        m_GpuResources->flush();
}

}  // namespace ZSlate

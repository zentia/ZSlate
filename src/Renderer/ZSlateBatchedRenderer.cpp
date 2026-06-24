#include "ZSlate/Renderer/ZSlateBatchedRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ZSlate
{

// ---- Internal helpers -------------------------------------------------------

namespace
{
    bool RectsEqual(const UIRect& a, const UIRect& b)
    {
        return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
    }

    UIRect IntersectRects(const UIRect& a, const UIRect& b)
    {
        float x0 = std::max(a.x, b.x);
        float y0 = std::max(a.y, b.y);
        float x1 = std::min(a.Right(), b.Right());
        float y1 = std::min(a.Bottom(), b.Bottom());
        if (x1 <= x0 || y1 <= y0)
            return UIRect{0, 0, 0, 0};
        return UIRect{x0, y0, x1 - x0, y1 - y0};
    }

    void ToColorFloats(const UIColor& c, float out[4])
    {
        out[0] = std::clamp(c.x, 0.0f, 1.0f);
        out[1] = std::clamp(c.y, 0.0f, 1.0f);
        out[2] = std::clamp(c.z, 0.0f, 1.0f);
        out[3] = std::clamp(c.w, 0.0f, 1.0f);
    }
}  // namespace

// ---- Constructor ------------------------------------------------------------

ZSlateBatchedRenderer::ZSlateBatchedRenderer()
{
    // Enough for ~1024 quads per frame; grows if needed
    m_Vertices.reserve(4096);
    m_Indices.reserve(6144);
    m_Commands.reserve(128);
    m_ClipStack.reserve(16);
    m_Outlines.reserve(32);
}

// ---- Frame lifecycle --------------------------------------------------------

void ZSlateBatchedRenderer::BeginFrame()
{
    m_Vertices.clear();
    m_Indices.clear();
    m_Commands.clear();
    m_ClipStack.clear();
    m_Outlines.clear();
    m_CurrentTexture = nullptr;
    m_HasClip = false;
    m_ActiveClip = UIRect{0, 0, 0, 0};
    m_Active = true;
}

void ZSlateBatchedRenderer::EndFrame()
{
    m_Active = false;
    FlushOutlineCommands();
}

// ---- Clipping ---------------------------------------------------------------

void ZSlateBatchedRenderer::PushClipRect(const UIRect& rect)
{
    if (!m_Active) return;
    UIRect effective = m_HasClip ? IntersectRects(m_ActiveClip, rect) : rect;
    m_ClipStack.push_back(m_ActiveClip);
    m_ActiveClip = effective;
    m_HasClip = true;
    ForceNewCommand(m_CurrentTexture);
}

void ZSlateBatchedRenderer::PopClipRect()
{
    if (!m_Active) return;
    if (!m_ClipStack.empty())
    {
        m_ActiveClip = m_ClipStack.back();
        m_ClipStack.pop_back();
        m_HasClip = !m_ClipStack.empty() || (m_ActiveClip.w > 0 && m_ActiveClip.h > 0);
    }
    else
    {
        m_HasClip = false;
    }
    ForceNewCommand(m_CurrentTexture);
}

// ---- Command management -----------------------------------------------------

void ZSlateBatchedRenderer::BeginCommand(void* textureId)
{
    if (!m_Active) return;
    if (m_CurrentTexture != textureId)
    {
        m_CurrentTexture = textureId;
        ForceNewCommand(textureId);
    }
}

void ZSlateBatchedRenderer::ForceNewCommand(void* textureId)
{
    if (!m_Active) return;
    ZSBDrawCmd cmd;
    cmd.TextureId = textureId;
    cmd.IndexOffset = static_cast<uint32_t>(m_Indices.size());
    cmd.IndexCount = 0;
    cmd.ClipRect = m_ActiveClip;
    cmd.HasClip = m_HasClip;
    m_Commands.push_back(cmd);
}

// ---- Quad generation --------------------------------------------------------

void ZSlateBatchedRenderer::AppendTexturedQuad(
    const UIRect& rect, const UIColor& color, void* textureId,
    float u0, float v0, float u1, float v1)
{
    if (!m_Active) return;

    BeginCommand(textureId);

    // Clamp to active clip rect
    UIRect clipped = rect;
    if (m_HasClip)
    {
        UIRect inter = IntersectRects(rect, m_ActiveClip);
        if (inter.w <= 0 || inter.h <= 0) return;
        float scaleU = (u1 - u0) / rect.w;
        float scaleV = (v1 - v0) / rect.h;
        u0 += (inter.x - rect.x) * scaleU;
        v0 += (inter.y - rect.y) * scaleV;
        u1 -= (rect.Right() - inter.Right()) * scaleU;
        v1 -= (rect.Bottom() - inter.Bottom()) * scaleV;
        clipped = inter;
    }

    float x0 = clipped.x;
    float y0 = clipped.y;
    float x1 = clipped.Right();
    float y1 = clipped.Bottom();

    float c[4];
    ToColorFloats(color, c);

    uint16_t base = static_cast<uint16_t>(m_Vertices.size());

    ZSBVertex v;
    std::memcpy(v.Color, c, sizeof(c));

    // TL
    v.Pos[0] = x0; v.Pos[1] = y0; v.UV[0] = u0; v.UV[1] = v0;
    m_Vertices.push_back(v);
    // TR
    v.Pos[0] = x1; v.Pos[1] = y0; v.UV[0] = u1; v.UV[1] = v0;
    m_Vertices.push_back(v);
    // BR
    v.Pos[0] = x1; v.Pos[1] = y1; v.UV[0] = u1; v.UV[1] = v1;
    m_Vertices.push_back(v);
    // BL
    v.Pos[0] = x0; v.Pos[1] = y1; v.UV[0] = u0; v.UV[1] = v1;
    m_Vertices.push_back(v);

    m_Indices.push_back(base);
    m_Indices.push_back(base + 1);
    m_Indices.push_back(base + 2);
    m_Indices.push_back(base);
    m_Indices.push_back(base + 2);
    m_Indices.push_back(base + 3);

    if (!m_Commands.empty())
        m_Commands.back().IndexCount = static_cast<uint32_t>(m_Indices.size()) - m_Commands.back().IndexOffset;
}

// ---- Outline accumulation (flushed in EndFrame) -----------------------------

void ZSlateBatchedRenderer::FlushOutlineCommands()
{
    if (m_Outlines.empty()) return;

    for (const auto& o : m_Outlines)
    {
        if (o.IdxBase + 23 >= m_Indices.size()) continue;

        uint16_t base = o.IdxBase;
        const ZSBVertex& v0 = m_Vertices[base];
        uint16_t idx = static_cast<uint16_t>(m_Vertices.size());
        float t = o.Thickness;
        float coeff = t * 1.4142f;  // sqrt(2) for corner bevelling

        // Top edge
        m_Vertices.push_back({ {v0.Pos[0] + t, v0.Pos[1]}, {0, 0}, {v0.Color[0], v0.Color[1], v0.Color[2], v0.Color[3]} });
        m_Indices.push_back(base);
        m_Indices.push_back(base + 1);
        m_Indices.push_back(idx);
        base++;
        idx++;

        // Right edge
        m_Vertices.push_back({ {v0.Pos[0] + coeff, v0.Pos[1] + coeff}, {0, 0}, {v0.Color[0], v0.Color[1], v0.Color[2], v0.Color[3]} });
        m_Indices.push_back(base);
        m_Indices.push_back(base + 1);
        m_Indices.push_back(idx);
        base = idx;
        idx++;

        // Bottom edge — simplified: just add the outline as a filled quad for now
        // For a complete implementation, see ZEngine's UIRenderBatch::appendOutline
        m_Vertices.push_back({ {v0.Pos[0] + coeff, v0.Pos[1] + coeff}, {0, 0}, {v0.Color[0], v0.Color[1], v0.Color[2], v0.Color[3]} });
        m_Indices.push_back(base);
        m_Indices.push_back(base + 1);
        m_Indices.push_back(idx);
    }
    m_Outlines.clear();
}

// ---- ISlateRenderer implementation ------------------------------------------

void ZSlateBatchedRenderer::DrawQuad(const UIRect& rect, const UIColor& color)
{
    if (!m_Active) return;
    AppendTexturedQuad(rect, color, nullptr);  // nullptr = solid white 1x1 dummy
}

void ZSlateBatchedRenderer::DrawRect(const UIRect& rect, const UIColor& color, float thickness)
{
    if (!m_Active || thickness <= 0) return;

    // Record as a filled quad first (the outline expands from it in EndFrame)
    AppendTexturedQuad(rect, color, nullptr);

    if (!m_Commands.empty())
    {
        uint16_t idxBase = static_cast<uint16_t>(m_Indices.size()) - 6;  // 6 indices per quad
        m_Outlines.push_back({ idxBase, thickness });
    }
}

void ZSlateBatchedRenderer::DrawConvexPoly(const Vector2* points, int count, const UIColor& color)
{
    if (!m_Active || count < 3) return;
    // Simplified: fill bounding rect
    float minX = points[0].x, maxX = points[0].x;
    float minY = points[0].y, maxY = points[0].y;
    for (int i = 1; i < count; ++i)
    {
        minX = std::min(minX, points[i].x); maxX = std::max(maxX, points[i].x);
        minY = std::min(minY, points[i].y); maxY = std::max(maxY, points[i].y);
    }
    AppendTexturedQuad(UIRect{minX, minY, maxX - minX, maxY - minY}, color, nullptr);
}

void ZSlateBatchedRenderer::DrawRoundedRect(const UIRect& rect, float radius, const UIColor& color)
{
    // Simplified: draw as filled quad; full rounded-rect tessellation omitted for brevity
    (void)radius;
    DrawQuad(rect, color);
}

void ZSlateBatchedRenderer::DrawTexturedQuad(const UIRect& rect, void* textureHandle, const UIColor& tint)
{
    AppendTexturedQuad(rect, tint, textureHandle);
}

void ZSlateBatchedRenderer::DrawBox(const UIRect& rect, const FMargin& margin, void* textureHandle, const UIColor& tint)
{
    // 9-slice box — simplified: draw the whole rect as a single textured quad
    (void)margin;
    AppendTexturedQuad(rect, tint, textureHandle);
}

void ZSlateBatchedRenderer::DrawBorder(const UIRect& rect, const FMargin& margin, void* textureHandle, const UIColor& tint)
{
    // Simplified border: draw rect with outline thickness = margin.Left
    (void)textureHandle;
    DrawRect(rect, tint, margin.Left);
}

void ZSlateBatchedRenderer::DrawText(const UIRect& rect, const std::string& text, float fontSize,
                                  const UIColor& color, TextAnchor alignment,
                                  TextWrapMode wrap, void* fontHandle)
{
    // Text is rendered as NDC-mapped quads in a full GPU text pipeline.
    // For the batch, approximate text as a filled quad for layout visualization.
    // Real text rendering requires a font atlas and glyph cache.
    (void)alignment; (void)wrap; (void)fontHandle;

    if (text.empty() || !m_Active) return;

    Vector2 measure = MeasureText(text, fontSize);
    float offsetX = rect.x;
    float offsetY = rect.y + (rect.h - measure.y) * 0.5f;  // vertically center

    switch (alignment)
    {
    case TextAnchor::UpperRight: case TextAnchor::MiddleRight: case TextAnchor::LowerRight:
        offsetX = rect.Right() - measure.x; break;
    case TextAnchor::UpperCenter: case TextAnchor::MiddleCenter: case TextAnchor::LowerCenter:
        offsetX = rect.x + (rect.w - measure.x) * 0.5f; break;
    default: break;
    }

    // Draw a placeholder rectangle approximating the text bounds
    float placeholderH = fontSize * 0.333f;  // ~1/3 line height for visibility
    UIRect placeholder{offsetX, offsetY + fontSize * 0.333f, measure.x, placeholderH};
    AppendTexturedQuad(placeholder, color, nullptr);
}

void ZSlateBatchedRenderer::DrawText(const std::string& text, const Vector2& pos, float fontSize, const UIColor& color)
{
    DrawText(UIRect{pos.x, pos.y, 300, fontSize + 4}, text, fontSize, color,
             TextAnchor::MiddleLeft, TextWrapMode::NoWrap, nullptr);
}

Vector2 ZSlateBatchedRenderer::MeasureText(const std::string& text, float fontSize) const
{
    if (m_TextMeasurer)
        return m_TextMeasurer(text, fontSize);

    // Default: rough ASCII estimate (0.6 * fontSize per char)
    float avgCharWidth = fontSize * 0.6f;
    return Vector2(static_cast<float>(text.length()) * avgCharWidth, fontSize * 1.2f);
}

}  // namespace ZSlate

#include "ScreenRenderer.h"

#include "TextRenderer.h"

#ifndef N3DS
namespace {

uint8_t colorComponent(uint32_t color, int shift) {
    return static_cast<uint8_t>((color >> shift) & 0xFFu);
}

} // namespace

ScreenRenderer::ScreenRenderer(SDL_Renderer* renderer, int xOffset)
    : m_renderer(renderer), m_xOffset(xOffset) {
}
#endif

void ScreenRenderer::fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    C2D_DrawRectSolid(static_cast<float>(x),
                      static_cast<float>(y),
                      0.5f,
                      static_cast<float>(w),
                      static_cast<float>(h),
                      C2D_Color32(r, g, b, a));
#else
    if (!m_renderer) {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    SDL_Rect rect{m_xOffset + x, y, w, h};
    SDL_RenderFillRect(m_renderer, &rect);
#endif
}

void ScreenRenderer::fillRect(int x, int y, int w, int h, uint32_t color) {
#ifdef N3DS
    C2D_DrawRectSolid(static_cast<float>(x),
                      static_cast<float>(y),
                      0.5f,
                      static_cast<float>(w),
                      static_cast<float>(h),
                      color);
#else
    fillRect(x, y, w, h, colorComponent(color, 24), colorComponent(color, 16), colorComponent(color, 8), colorComponent(color, 0));
#endif
}

void ScreenRenderer::drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    const uint32_t color = C2D_Color32(r, g, b, a);
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    fillRect(x, y, 1, h, color);
    fillRect(x + w - 1, y, 1, h, color);
#else
    if (!m_renderer) {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    SDL_Rect rect{m_xOffset + x, y, w, h};
    SDL_RenderDrawRect(m_renderer, &rect);
#endif
}

void ScreenRenderer::drawRect(int x, int y, int w, int h, uint32_t color) {
#ifdef N3DS
    const uint8_t r = static_cast<uint8_t>(color >> 24);
    const uint8_t g = static_cast<uint8_t>(color >> 16);
    const uint8_t b = static_cast<uint8_t>(color >> 8);
    const uint8_t a = static_cast<uint8_t>(color);
    drawRect(x, y, w, h, r, g, b, a);
#else
    drawRect(x, y, w, h, colorComponent(color, 24), colorComponent(color, 16), colorComponent(color, 8), colorComponent(color, 0));
#endif
}

void ScreenRenderer::drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    TextRenderer::drawText(text, x, y, scale, r, g, b, a);
#else
    TextRenderer::setRenderer(m_renderer);
    TextRenderer::drawText(text, x + static_cast<float>(m_xOffset), y, scale, r, g, b, a);
#endif
}

void ScreenRenderer::drawText(const std::string& text, float x, float y, float scaleX, float scaleY, uint32_t color) {
#ifdef N3DS
    TextRenderer::drawText(text, x, y, scaleX, scaleY, color);
#else
    TextRenderer::setRenderer(m_renderer);
    TextRenderer::drawText(text,
                           x + static_cast<float>(m_xOffset),
                           y,
                           scaleX,
                           colorComponent(color, 24),
                           colorComponent(color, 16),
                           colorComponent(color, 8),
                           colorComponent(color, 0));
#endif
}

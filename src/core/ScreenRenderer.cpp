#include "ScreenRenderer.h"

#include "TextRenderer.h"

#ifndef N3DS
#include <SDL.h>

ScreenRenderer::ScreenRenderer(SDL_Renderer* renderer, int xOffset)
    : m_renderer(renderer), m_xOffset(xOffset) {
    TextRenderer::setRenderer(renderer);
}
#endif

void ScreenRenderer::fillRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    C2D_DrawRectSolid(x,
                      y,
                      0.5f,
                      w,
                      h,
                      C2D_Color32(r, g, b, a));
#else
    if (!m_renderer) {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    SDL_FRect rect{x + static_cast<float>(m_xOffset), y, w, h};
    SDL_RenderFillRectF(m_renderer, &rect);
#endif
}

void ScreenRenderer::drawRectOutline(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    fillRect(x, y, w, 1.0f, r, g, b, a);
    fillRect(x, y + h - 1.0f, w, 1.0f, r, g, b, a);
    fillRect(x, y, 1.0f, h, r, g, b, a);
    fillRect(x + w - 1.0f, y, 1.0f, h, r, g, b, a);
#else
    if (!m_renderer) {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    SDL_FRect rect{x + static_cast<float>(m_xOffset), y, w, h};
    SDL_RenderDrawRectF(m_renderer, &rect);
#endif
}

void ScreenRenderer::drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef N3DS
    TextRenderer::drawText(text, x, y, scale, r, g, b, a);
#else
    TextRenderer::drawText(text, x + static_cast<float>(m_xOffset), y, scale, r, g, b, a);
#endif
}

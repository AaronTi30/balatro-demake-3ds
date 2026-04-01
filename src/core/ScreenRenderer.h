#pragma once

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
struct SDL_Renderer;
#endif

#include <cstdint>
#include <string>

class ScreenRenderer {
public:
#ifdef N3DS
    ScreenRenderer() = default;
#else
    ScreenRenderer(SDL_Renderer* renderer, int xOffset);
#endif

    void fillRect(float x, float y, float w, float h,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void drawRectOutline(float x, float y, float w, float h,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void drawText(const std::string& text, float x, float y, float scale,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

private:
#ifndef N3DS
    SDL_Renderer* m_renderer;
    int m_xOffset;
#endif
};

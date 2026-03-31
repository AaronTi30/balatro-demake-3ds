#pragma once

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
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

    void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void fillRect(int x, int y, int w, int h, uint32_t color);

    void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void drawRect(int x, int y, int w, int h, uint32_t color);

    void drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void drawText(const std::string& text, float x, float y, float scaleX, float scaleY, uint32_t color);

#ifndef N3DS
    SDL_Renderer* renderer() const { return m_renderer; }
    int xOffset() const { return m_xOffset; }
#endif

private:
#ifndef N3DS
    SDL_Renderer* m_renderer;
    int m_xOffset;
#endif
};

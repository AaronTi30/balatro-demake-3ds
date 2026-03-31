#pragma once

// Cross-platform text rendering helper.
// PC: uses SDL_ttf
// 3DS: uses citro2d's built-in C2D_Text

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

#include <cstdint>
#include <string>

class TextRenderer {
public:
    static bool init();
    static void shutdown();

    // Draw text at (x, y) with the given color (r, g, b)
    // size: 0 = small (card pip), 1 = medium (UI), 2 = large (title)
#ifdef N3DS
    static void drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static void drawText(const std::string& text, float x, float y, float scaleX, float scaleY, u32 color);
#else
    // Desktop shared drawText() uses the renderer set here.
    static void setRenderer(SDL_Renderer* renderer);
    static void drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static int fontSizeForScaleForTests(float scale);
    static int lastResolvedFontSizeForTests();
    static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b);
#endif

private:
#ifdef N3DS
    static C2D_TextBuf s_textBuf;
#else
    static SDL_Renderer* s_renderer;
    static int s_lastResolvedFontSizeForTests;
    static TTF_Font* s_fontSmall;
    static TTF_Font* s_fontMedium;
    static TTF_Font* s_fontLarge;
    static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
#endif
    static bool s_initialized;
};

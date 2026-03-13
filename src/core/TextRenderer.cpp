#include "TextRenderer.h"
#include <iostream>

bool TextRenderer::s_initialized = false;

#ifdef N3DS
C2D_TextBuf TextRenderer::s_textBuf = nullptr;
#else
TTF_Font* TextRenderer::s_fontSmall = nullptr;
TTF_Font* TextRenderer::s_fontMedium = nullptr;
TTF_Font* TextRenderer::s_fontLarge = nullptr;
#endif

bool TextRenderer::init() {
    if (s_initialized) return true;

#ifdef N3DS
    s_textBuf = C2D_TextBufNew(4096);
    s_initialized = (s_textBuf != nullptr);
    return s_initialized;
#else
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    // Try several font paths (macOS system fonts)
    const char* fontPaths[] = {
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Geneva.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux fallback
        nullptr
    };

    const char* chosenFont = nullptr;
    for (int i = 0; fontPaths[i] != nullptr; ++i) {
        TTF_Font* test = TTF_OpenFont(fontPaths[i], 12);
        if (test) {
            TTF_CloseFont(test);
            chosenFont = fontPaths[i];
            break;
        }
    }

    if (!chosenFont) {
        std::cerr << "TextRenderer: Could not find any system font!" << std::endl;
        return false;
    }

    s_fontSmall  = TTF_OpenFont(chosenFont, 10);
    s_fontMedium = TTF_OpenFont(chosenFont, 16);
    s_fontLarge  = TTF_OpenFont(chosenFont, 24);

    if (!s_fontSmall || !s_fontMedium || !s_fontLarge) {
        std::cerr << "TextRenderer: Failed to load font at sizes: " << TTF_GetError() << std::endl;
        return false;
    }

    s_initialized = true;
    std::cout << "TextRenderer: Using font '" << chosenFont << "'" << std::endl;
    return true;
#endif
}

void TextRenderer::shutdown() {
#ifdef N3DS
    if (s_textBuf) {
        C2D_TextBufDelete(s_textBuf);
        s_textBuf = nullptr;
    }
#else
    if (s_fontSmall)  { TTF_CloseFont(s_fontSmall);  s_fontSmall = nullptr; }
    if (s_fontMedium) { TTF_CloseFont(s_fontMedium); s_fontMedium = nullptr; }
    if (s_fontLarge)  { TTF_CloseFont(s_fontLarge);  s_fontLarge = nullptr; }
    TTF_Quit();
#endif
    s_initialized = false;
}

#ifdef N3DS
void TextRenderer::drawText(const std::string& text, float x, float y, float scaleX, float scaleY, u32 color) {
    if (!s_initialized) return;
    C2D_TextBufClear(s_textBuf);
    C2D_Text c2dText;
    C2D_TextParse(&c2dText, s_textBuf, text.c_str());
    C2D_TextOptimize(&c2dText);
    C2D_DrawText(&c2dText, C2D_WithColor, x, y, 0.5f, scaleX, scaleY, color);
}
#else
void TextRenderer::drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b) {
    if (!s_initialized || text.empty()) return;

    TTF_Font* font = nullptr;
    switch (size) {
        case 0: font = s_fontSmall; break;
        case 1: font = s_fontMedium; break;
        case 2: font = s_fontLarge; break;
        default: font = s_fontSmall; break;
    }

    SDL_Color color = { r, g, b, 255 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}
#endif

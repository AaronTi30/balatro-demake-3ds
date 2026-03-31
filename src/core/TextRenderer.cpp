#include "TextRenderer.h"

#include "AssetPath.h"

#include <filesystem>
#include <iostream>
#include <string>

bool TextRenderer::s_initialized = false;

#ifdef N3DS
C2D_TextBuf TextRenderer::s_textBuf = nullptr;
#else
SDL_Renderer* TextRenderer::s_renderer = nullptr;
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

    const std::filesystem::path currentDir = std::filesystem::current_path();
    char* rawBasePath = SDL_GetBasePath();
    const std::filesystem::path executableDir =
        rawBasePath ? std::filesystem::path(rawBasePath) : currentDir;
    if (rawBasePath) {
        SDL_free(rawBasePath);
    }

    const std::filesystem::path bundledFont = resolveAssetPath(
        "assets/fonts/m6x11plus.ttf",
        currentDir,
        executableDir
    );

    auto canOpenFont = [](const std::string& path) {
        TTF_Font* test = TTF_OpenFont(path.c_str(), 12);
        if (!test) {
            return false;
        }

        TTF_CloseFont(test);
        return true;
    };

    std::string chosenFont;
    if (!bundledFont.empty() && std::filesystem::exists(bundledFont)) {
        const std::string bundledFontString = bundledFont.string();
        if (canOpenFont(bundledFontString)) {
            chosenFont = bundledFontString;
        }
    }

    // Fall back to several system font paths when the bundled font is unavailable.
    const char* fontPaths[] = {
        "/System/Library/Fonts/Courier.ttc",
        "/System/Library/Fonts/Geneva.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux fallback
        nullptr
    };

    if (chosenFont.empty()) {
        for (int i = 0; fontPaths[i] != nullptr; ++i) {
            if (canOpenFont(fontPaths[i])) {
                chosenFont = fontPaths[i];
                break;
            }
        }
    }

    if (chosenFont.empty()) {
        std::cerr << "TextRenderer: Could not find a bundled or system font!" << std::endl;
        return false;
    }

    s_fontSmall  = TTF_OpenFont(chosenFont.c_str(), 10);
    s_fontMedium = TTF_OpenFont(chosenFont.c_str(), 16);
    s_fontLarge  = TTF_OpenFont(chosenFont.c_str(), 24);

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
    s_renderer = nullptr;
    if (s_fontSmall)  { TTF_CloseFont(s_fontSmall);  s_fontSmall = nullptr; }
    if (s_fontMedium) { TTF_CloseFont(s_fontMedium); s_fontMedium = nullptr; }
    if (s_fontLarge)  { TTF_CloseFont(s_fontLarge);  s_fontLarge = nullptr; }
    TTF_Quit();
#endif
    s_initialized = false;
}

#ifdef N3DS
void TextRenderer::drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    drawText(text, x, y, scale, scale, C2D_Color32(r, g, b, a));
}

void TextRenderer::drawText(const std::string& text, float x, float y, float scaleX, float scaleY, u32 color) {
    if (!s_initialized) return;
    C2D_TextBufClear(s_textBuf);
    C2D_Text c2dText;
    C2D_TextParse(&c2dText, s_textBuf, text.c_str());
    C2D_TextOptimize(&c2dText);
    C2D_DrawText(&c2dText, C2D_WithColor, x, y, 0.5f, scaleX, scaleY, color);
}
#else
void TextRenderer::setRenderer(SDL_Renderer* renderer) {
    s_renderer = renderer;
}

int TextRenderer::fontSizeForScaleForTests(float scale) {
    if (scale < 0.45f) {
        return 0;
    }
    if (scale < 0.63f) {
        return 1;
    }
    return 2;
}

void TextRenderer::drawText(const std::string& text, float x, float y, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    drawText(s_renderer, text, static_cast<int>(x), static_cast<int>(y), fontSizeForScaleForTests(scale), r, g, b, a);
}

void TextRenderer::drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!s_initialized || text.empty() || !renderer) return;

    TTF_Font* font = nullptr;
    switch (size) {
        case 0: font = s_fontSmall; break;
        case 1: font = s_fontMedium; break;
        case 2: font = s_fontLarge; break;
        default: font = s_fontSmall; break;
    }

    SDL_Color color = { r, g, b, a };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(texture, a);

    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void TextRenderer::drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, Uint8 r, Uint8 g, Uint8 b) {
    drawText(renderer, text, x, y, size, r, g, b, 255);
}
#endif

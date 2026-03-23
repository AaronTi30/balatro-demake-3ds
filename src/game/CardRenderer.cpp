#include "CardRenderer.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../core/stb_image.h"
#endif

#include <math.h>

#ifndef N3DS
SDL_Texture* CardRenderer::t_base = nullptr;
SDL_Texture* CardRenderer::t_spade = nullptr;
SDL_Texture* CardRenderer::t_heart = nullptr;
SDL_Texture* CardRenderer::t_club = nullptr;
SDL_Texture* CardRenderer::t_diamond = nullptr;

static SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    int w, h, channels;
    unsigned char* pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) return nullptr;
    
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 32, 4 * w, SDL_PIXELFORMAT_RGBA32);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(pixels);
    return tex;
}
#endif

void CardRenderer::init(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
    if (!t_base) {
        t_base = loadTexture(renderer, "assets/textures/card_base.png");
        t_spade = loadTexture(renderer, "assets/textures/spade.png");
        t_heart = loadTexture(renderer, "assets/textures/heart.png");
        t_club = loadTexture(renderer, "assets/textures/club.png");
        t_diamond = loadTexture(renderer, "assets/textures/diamond.png");
    }
#endif
}

void CardRenderer::drawCard(Application* app, const Card& card, int x, int y) {
    init(app); // Lazy init textures
    
    // Shift card up if selected
    int drawY = card.selected ? y - SELECT_OFFSET : y;
    bool isRed = suitIsRed(card.suit);
    const char* rankStr = rankToString(card.rank);

    // Apply some slight wobble if selected (juice physics)
    float angle = 0.0f;
    if (card.selected) {
#ifndef N3DS
        angle = sin(SDL_GetTicks() / 150.0f) * 4.0f; // Wobble ±4 degrees
#endif
    }

#ifdef N3DS
    // ── Card body (white rectangle) ──
    C2D_DrawRectSolid(x, drawY, 0.5f, CARD_W, CARD_H, C2D_Color32(240, 240, 235, 255));

    // ── Card border ──
    u32 borderColor = C2D_Color32(160, 160, 160, 255);
    C2D_DrawRectSolid(x, drawY, 0.5f, CARD_W, 1, borderColor);
    C2D_DrawRectSolid(x, drawY + CARD_H - 1, 0.5f, CARD_W, 1, borderColor);
    C2D_DrawRectSolid(x, drawY, 0.5f, 1, CARD_H, borderColor);
    C2D_DrawRectSolid(x + CARD_W - 1, drawY, 0.5f, 1, CARD_H, borderColor);

    // ── Rank text (top-left) ──
    u32 textColor = isRed ? C2D_Color32(220, 50, 50, 255) : C2D_Color32(30, 30, 30, 255);
    TextRenderer::drawText(rankStr, x + 3, drawY + 2, 0.4f, 0.4f, textColor);

    // ── Center suit indicator ──
    const char* suitStr = suitToString(card.suit);
    TextRenderer::drawText(suitStr, x + CARD_W/2 - 4, drawY + CARD_H/2 - 6, 0.5f, 0.5f, textColor);
#else
    SDL_Renderer* renderer = app->getRenderer();
    SDL_Rect dstRect = { x, drawY, CARD_W, CARD_H };

    // ── Base Card ──
    if (t_base) {
        SDL_RenderCopyEx(renderer, t_base, NULL, &dstRect, angle, NULL, SDL_FLIP_NONE);
    } else {
        SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
        SDL_RenderFillRect(renderer, &dstRect);
        SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
        SDL_RenderDrawRect(renderer, &dstRect);
    }

    // ── Text colors ──
    Uint8 tr = isRed ? 220 : 30;
    Uint8 tg = isRed ? 50 : 30;
    Uint8 tb = isRed ? 50 : 30;

    // ── Rank text ──
    // Note: Text rotation isn't easily supported by our TextRenderer right now, 
    // so it will just wobble Y based on angle if we really wanted, but staying static is fine.
    int textOfsY = (card.selected) ? (int)(sin(SDL_GetTicks() / 150.0f) * 2.0f) : 0;
    TextRenderer::drawText(renderer, rankStr, x + 4, drawY + 2 + textOfsY, 0, tr, tg, tb);

    // ── Suit Icon ──
    SDL_Texture* t_suit = nullptr;
    switch(card.suit) {
        case Suit::Spades: t_suit = t_spade; break;
        case Suit::Hearts: t_suit = t_heart; break;
        case Suit::Clubs: t_suit = t_club; break;
        case Suit::Diamonds: t_suit = t_diamond; break;
    }

    if (t_suit) {
        int suitW = (card.suit == Suit::Diamonds) ? 21 : 20;
        int suitH = (card.suit == Suit::Diamonds) ? 21 : 20;
        SDL_Rect suitRect = { x + CARD_W/2 - suitW/2, drawY + CARD_H/2 - suitH/2, suitW, suitH };
        SDL_RenderCopyEx(renderer, t_suit, NULL, &suitRect, angle, NULL, SDL_FLIP_NONE);
        
        // Small suit under rank
        SDL_Rect thinRect = { x + 6, drawY + 16 + textOfsY, 8, 8 };
        if (card.rank != Rank::Ten) {
            SDL_RenderCopyEx(renderer, t_suit, NULL, &thinRect, angle, NULL, SDL_FLIP_NONE);
        }
    } else {
        const char* suitStr = suitToString(card.suit);
        TextRenderer::drawText(renderer, suitStr, x + CARD_W/2 - 5, drawY + CARD_H/2 - 8, 1, tr, tg, tb);
    }
#endif
}

void CardRenderer::drawHand(Application* app, const Hand& hand, int centerX, int y, int cursorIndex) {
    if (hand.empty()) return;

    int totalWidth = (hand.size() - 1) * CARD_SPACING + CARD_W;
    int startX = centerX - totalWidth / 2;

    for (int i = 0; i < hand.size(); ++i) {
        int cardX = startX + i * CARD_SPACING;
        drawCard(app, hand.at(i), cardX, y);

        // Draw cursor indicator (yellow rectangle below the card)
        if (i == cursorIndex) {
            int drawY = hand.at(i).selected ? y - SELECT_OFFSET : y;
#ifdef N3DS
            C2D_DrawRectSolid(cardX + CARD_W / 2 - 4, drawY + CARD_H + 4, 0.5f, 8, 4, 
                              C2D_Color32(255, 255, 0, 255));
#else
            SDL_Renderer* renderer = app->getRenderer();
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect cursor = { cardX + CARD_W / 2 - 4, drawY + CARD_H + 4, 8, 4 };
            SDL_RenderFillRect(renderer, &cursor);
#endif
        }
    }
}

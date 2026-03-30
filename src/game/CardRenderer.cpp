#include "CardRenderer.h"
#include "../core/Application.h"
#include "../core/AssetPath.h"
#include "../core/TextRenderer.h"

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <math.h>

#ifndef N3DS
SDL_Texture* CardRenderer::t_cards = nullptr;

namespace {

int spriteSheetRow(Suit suit) {
    switch (suit) {
        case Suit::Hearts: return 0;
        case Suit::Clubs: return 1;
        case Suit::Diamonds: return 2;
        case Suit::Spades: return 3;
        default: return 0;
    }
}

int spriteSheetColumn(Rank rank) {
    switch (rank) {
        case Rank::Two: return 0;
        case Rank::Three: return 1;
        case Rank::Four: return 2;
        case Rank::Five: return 3;
        case Rank::Six: return 4;
        case Rank::Seven: return 5;
        case Rank::Eight: return 6;
        case Rank::Nine: return 7;
        case Rank::Ten: return 8;
        case Rank::Jack: return 9;
        case Rank::Queen: return 10;
        case Rank::King: return 11;
        case Rank::Ace: return 12;
        default: return 0;
    }
}

} // namespace
#endif

void CardRenderer::init(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
    if (!t_cards) {
        t_cards = sdlLoadTexture(renderer, "assets/textures/8BitDeck.png");
    }
#endif
}

#ifndef N3DS
CardRenderer::CardSpriteSourceRect CardRenderer::spriteSheetSourceRect(const Card& card) {
    return {
        spriteSheetColumn(card.rank) * SPRITE_SHEET_CELL_W,
        spriteSheetRow(card.suit) * SPRITE_SHEET_CELL_H,
        SPRITE_SHEET_CELL_W,
        SPRITE_SHEET_CELL_H
    };
}
#endif

void CardRenderer::drawCard(Application* app, const Card& card, int x, int y, bool selected) {
    init(app); // Lazy init textures
    
    // Shift card up if selected
    int drawY = selected ? y - SELECT_OFFSET : y;
    bool isRed = suitIsRed(card.suit);
    const char* rankStr = rankToString(card.rank);

    // Apply some slight wobble if selected (juice physics)
    float angle = 0.0f;
    if (selected) {
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

    if (t_cards) {
        const CardSpriteSourceRect source = spriteSheetSourceRect(card);
        SDL_Rect srcRect = { source.x, source.y, source.w, source.h };
        SDL_RenderCopyEx(renderer, t_cards, &srcRect, &dstRect, angle, NULL, SDL_FLIP_NONE);
    } else {
        SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
        SDL_RenderFillRect(renderer, &dstRect);
        SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
        SDL_RenderDrawRect(renderer, &dstRect);
        const char* suitStr = suitToString(card.suit);
        Uint8 tr = isRed ? 220 : 30;
        Uint8 tg = isRed ? 50 : 30;
        Uint8 tb = isRed ? 50 : 30;
        int textOfsY = selected ? static_cast<int>(sin(SDL_GetTicks() / 150.0f) * 2.0f) : 0;

        TextRenderer::drawText(renderer, rankStr, x + 4, drawY + 2 + textOfsY, 0, tr, tg, tb);
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
        const bool selected = hand.isSelected(i);
        drawCard(app, hand.at(i), cardX, y, selected);

        // Draw cursor indicator (yellow rectangle below the card)
        if (i == cursorIndex) {
            int drawY = selected ? y - SELECT_OFFSET : y;
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

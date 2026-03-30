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
SDL_Texture* CardRenderer::t_base = nullptr;
SDL_Texture* CardRenderer::t_cards = nullptr;

namespace {

CardRenderer::CardSpriteSourceRect defaultBaseSourceRect() {
    return {
        CardRenderer::SPRITE_SHEET_CELL_W,
        0,
        CardRenderer::SPRITE_SHEET_CELL_W,
        CardRenderer::SPRITE_SHEET_CELL_H
    };
}

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

CardRenderer::HandLayoutMetrics CardRenderer::defaultHandLayout() {
    return { CARD_W, CARD_H, CARD_SPACING, SELECT_OFFSET, 8, 4, 4 };
}

CardRenderer::HandLayoutMetrics CardRenderer::gameplayHandLayout() {
    return { 52, 70, 28, 14, 10, 5, 5 };
}

int CardRenderer::handWidthForCount(int cardCount, const HandLayoutMetrics& layout) {
    if (cardCount <= 0) return 0;
    return (cardCount - 1) * layout.cardSpacing + layout.cardW;
}

int CardRenderer::handStartX(int centerX, int cardCount, const HandLayoutMetrics& layout) {
    return centerX - handWidthForCount(cardCount, layout) / 2;
}

int CardRenderer::handCardX(int centerX, int cardCount, int index, const HandLayoutMetrics& layout) {
    return handStartX(centerX, cardCount, layout) + index * layout.cardSpacing;
}

int CardRenderer::handIndexAtX(int mouseX, int centerX, int cardCount, const HandLayoutMetrics& layout) {
    if (cardCount <= 0) return -1;
    int startX = handStartX(centerX, cardCount, layout);
    // Left of first card
    if (mouseX < startX) return -1;
    // Right of the fully visible last card
    int lastCardRightEdge = startX + (cardCount - 1) * layout.cardSpacing + layout.cardW;
    if (mouseX > lastCardRightEdge) return -1;
    // For all but the last card, use spacing buckets
    int idx = (mouseX - startX) / layout.cardSpacing;
    if (idx >= cardCount) idx = cardCount - 1;
    return idx;
}

void CardRenderer::init(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
    if (!t_base) {
        t_base = sdlLoadTexture(renderer, "assets/textures/Enhancers.png");
    }
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

CardRenderer::DesktopCardRenderPlan CardRenderer::desktopRenderPlan(const Card& card) {
    return {
        true,
        defaultBaseSourceRect(),
        spriteSheetSourceRect(card)
    };
}
#endif

void CardRenderer::drawCard(Application* app, const Card& card, int x, int y, bool selected, const HandLayoutMetrics& layout) {
    init(app); // Lazy init textures

    // Shift card up if selected
    int drawY = selected ? y - layout.selectOffset : y;
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
    C2D_DrawRectSolid(x, drawY, 0.5f, layout.cardW, layout.cardH, C2D_Color32(240, 240, 235, 255));

    // ── Card border ──
    u32 borderColor = C2D_Color32(160, 160, 160, 255);
    C2D_DrawRectSolid(x, drawY, 0.5f, layout.cardW, 1, borderColor);
    C2D_DrawRectSolid(x, drawY + layout.cardH - 1, 0.5f, layout.cardW, 1, borderColor);
    C2D_DrawRectSolid(x, drawY, 0.5f, 1, layout.cardH, borderColor);
    C2D_DrawRectSolid(x + layout.cardW - 1, drawY, 0.5f, 1, layout.cardH, borderColor);

    // ── Rank text (top-left) ──
    u32 textColor = isRed ? C2D_Color32(220, 50, 50, 255) : C2D_Color32(30, 30, 30, 255);
    TextRenderer::drawText(rankStr, x + 3, drawY + 2, 0.4f, 0.4f, textColor);

    // ── Center suit indicator ──
    const char* suitStr = suitToString(card.suit);
    TextRenderer::drawText(suitStr, x + layout.cardW/2 - 4, drawY + layout.cardH/2 - 6, 0.5f, 0.5f, textColor);
#else
    SDL_Renderer* renderer = app->getRenderer();
    SDL_Rect dstRect = { x, drawY, layout.cardW, layout.cardH };
    const DesktopCardRenderPlan plan = desktopRenderPlan(card);

    if (plan.drawBaseTexture) {
        if (t_base) {
            SDL_Rect baseSrcRect = {
                plan.baseSource.x,
                plan.baseSource.y,
                plan.baseSource.w,
                plan.baseSource.h
            };
            SDL_RenderCopyEx(renderer, t_base, &baseSrcRect, &dstRect, angle, NULL, SDL_FLIP_NONE);
        } else {
            SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
            SDL_RenderFillRect(renderer, &dstRect);
            SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
            SDL_RenderDrawRect(renderer, &dstRect);
        }
    }

    if (t_cards) {
        SDL_Rect srcRect = {
            plan.overlaySource.x,
            plan.overlaySource.y,
            plan.overlaySource.w,
            plan.overlaySource.h
        };
        SDL_RenderCopyEx(renderer, t_cards, &srcRect, &dstRect, angle, NULL, SDL_FLIP_NONE);
    } else {
        const char* suitStr = suitToString(card.suit);
        Uint8 tr = isRed ? 220 : 30;
        Uint8 tg = isRed ? 50 : 30;
        Uint8 tb = isRed ? 50 : 30;
        int textOfsY = selected ? static_cast<int>(sin(SDL_GetTicks() / 150.0f) * 2.0f) : 0;

        TextRenderer::drawText(renderer, rankStr, x + 4, drawY + 2 + textOfsY, 0, tr, tg, tb);
        TextRenderer::drawText(renderer, suitStr, x + layout.cardW/2 - 5, drawY + layout.cardH/2 - 8, 1, tr, tg, tb);
    }
#endif
}

void CardRenderer::drawHand(Application* app, const Hand& hand, int centerX, int y, int cursorIndex, const HandLayoutMetrics& layout) {
    if (hand.empty()) return;

    for (int i = 0; i < (int)hand.size(); ++i) {
        int cardX = handCardX(centerX, (int)hand.size(), i, layout);
        const bool selected = hand.isSelected(i);
        drawCard(app, hand.at(i), cardX, y, selected, layout);

        // Draw cursor indicator (yellow rectangle below the card)
        if (i == cursorIndex) {
            int drawY = selected ? y - layout.selectOffset : y;
            int cursorX = cardX + layout.cardW / 2 - layout.cursorW / 2;
            int cursorY = drawY + layout.cardH + layout.cursorGap;
#ifdef N3DS
            C2D_DrawRectSolid(cursorX, cursorY, 0.5f, layout.cursorW, layout.cursorH,
                              C2D_Color32(255, 255, 0, 255));
#else
            SDL_Renderer* renderer = app->getRenderer();
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect cursor = { cursorX, cursorY, layout.cursorW, layout.cursorH };
            SDL_RenderFillRect(renderer, &cursor);
#endif
        }
    }
}

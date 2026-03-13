#include "CardRenderer.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#endif

void CardRenderer::drawCard(Application* app, const Card& card, int x, int y) {
    // Shift card up if selected
    int drawY = card.selected ? y - SELECT_OFFSET : y;

    // Determine suit color
    bool isRed = suitIsRed(card.suit);

    // Get rank and suit strings
    const char* rankStr = rankToString(card.rank);
    const char* suitStr = suitToString(card.suit);

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

    // ── Suit letter (below rank) ──
    TextRenderer::drawText(suitStr, x + 3, drawY + 14, 0.35f, 0.35f, textColor);

    // ── Center suit indicator ──
    TextRenderer::drawText(suitStr, x + CARD_W/2 - 4, drawY + CARD_H/2 - 6, 0.5f, 0.5f, textColor);

#else
    SDL_Renderer* renderer = app->getRenderer();

    // ── Card body (off-white) ──
    SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
    SDL_Rect body = { x, drawY, CARD_W, CARD_H };
    SDL_RenderFillRect(renderer, &body);

    // ── Border ──
    SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
    SDL_RenderDrawRect(renderer, &body);

    // ── Text colors ──
    Uint8 tr = isRed ? 220 : 30;
    Uint8 tg = isRed ? 50 : 30;
    Uint8 tb = isRed ? 50 : 30;

    // ── Rank text (top-left corner) ──
    TextRenderer::drawText(renderer, rankStr, x + 3, drawY + 2, 0, tr, tg, tb);

    // ── Suit letter (below rank) ──
    TextRenderer::drawText(renderer, suitStr, x + 3, drawY + 14, 0, tr, tg, tb);

    // ── Center suit (larger) ──
    TextRenderer::drawText(renderer, suitStr, x + CARD_W/2 - 5, drawY + CARD_H/2 - 8, 1, tr, tg, tb);
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

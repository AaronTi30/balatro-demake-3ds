#pragma once

#include "Card.h"
#include "Hand.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#endif

class Application;
struct SDL_Texture;

class CardRenderer {
public:
    struct CardSpriteSourceRect {
        int x;
        int y;
        int w;
        int h;
    };

    struct DesktopCardRenderPlan {
        bool drawBaseTexture;
        CardSpriteSourceRect baseSource;
        CardSpriteSourceRect overlaySource;
    };

    struct HandLayoutMetrics {
        int cardW;
        int cardH;
        int cardSpacing;
        int selectOffset;
        int cursorW;
        int cursorH;
        int cursorGap;
    };

    // Card dimensions (3DS-friendly sizing)
    static constexpr int CARD_W = 40;
    static constexpr int CARD_H = 56;
    static constexpr int CARD_SPACING = 36; // Overlap amount
    static constexpr int SELECT_OFFSET = 12; // Y shift when selected
    static constexpr int SPRITE_SHEET_CELL_W = 71;
    static constexpr int SPRITE_SHEET_CELL_H = 95;

    // Hand layout presets
    static HandLayoutMetrics defaultHandLayout();
    static HandLayoutMetrics gameplayHandLayout();

    // Hand geometry helpers
    static int handWidthForCount(int cardCount, const HandLayoutMetrics& layout);
    static int handStartX(int centerX, int cardCount, const HandLayoutMetrics& layout);
    static int handCardX(int centerX, int cardCount, int index, const HandLayoutMetrics& layout);
    static int handIndexAtX(int mouseX, int centerX, int cardCount, const HandLayoutMetrics& layout);

    // Init texture assets
    static void init(Application* app);

    // Draw a single card at x,y
    static void drawCard(Application* app,
                         const Card& card,
                         int x,
                         int y,
                         bool selected = false,
                         const HandLayoutMetrics& layout = defaultHandLayout());

    // Draw a hand of cards fanned horizontally, centered around centerX
    static void drawHand(Application* app,
                         const Hand& hand,
                         int centerX,
                         int y,
                         int cursorIndex = -1,
                         const HandLayoutMetrics& layout = defaultHandLayout());

#ifndef N3DS
    static CardSpriteSourceRect spriteSheetSourceRect(const Card& card);
    static DesktopCardRenderPlan desktopRenderPlan(const Card& card);
    static SDL_Texture* getCardsTexture() { return t_cards; }
    static SDL_Texture* getCardBaseTexture() { return t_base; }
#endif

private:
#ifdef N3DS
    static C3D_Tex s_cardTex;
    static Tex3DS_Texture s_cardT3x;
    static bool s_cardTexLoaded;
#else
    static SDL_Texture* t_base;
    static SDL_Texture* t_cards;
#endif
};

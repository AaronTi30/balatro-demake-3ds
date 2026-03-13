#pragma once

#include "Card.h"
#include "Hand.h"

class Application;

class CardRenderer {
public:
    // Card dimensions (3DS-friendly sizing)
    static constexpr int CARD_W = 40;
    static constexpr int CARD_H = 56;
    static constexpr int CARD_SPACING = 36; // Overlap amount
    static constexpr int SELECT_OFFSET = 12; // Y shift when selected

    // Draw a single card at x,y
    static void drawCard(Application* app, const Card& card, int x, int y);
    
    // Draw a hand of cards fanned horizontally, centered around centerX
    static void drawHand(Application* app, const Hand& hand, int centerX, int y, int cursorIndex = -1);
};

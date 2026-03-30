#include "game/CardRenderer.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void expectRect(const CardRenderer::CardSpriteSourceRect& actual,
                int expectedX,
                int expectedY,
                const std::string& label) {
    expectEqual(actual.x, expectedX, label + " x");
    expectEqual(actual.y, expectedY, label + " y");
    expectEqual(actual.w, CardRenderer::SPRITE_SHEET_CELL_W, label + " w");
    expectEqual(actual.h, CardRenderer::SPRITE_SHEET_CELL_H, label + " h");
}

void testSpriteSheetMappingUsesExplicitSuitAndRankOrder() {
    const CardRenderer::DesktopCardRenderPlan heartsTwoPlan =
        CardRenderer::desktopRenderPlan({ Suit::Hearts, Rank::Two });
    expect(heartsTwoPlan.drawBaseTexture,
           "desktop renderer should keep drawing the opaque base texture under sprite art");
    expectRect(heartsTwoPlan.baseSource,
               CardRenderer::SPRITE_SHEET_CELL_W,
               0,
               "desktop renderer should use the Balatro default base card cell from Enhancers.png");
    expectRect(heartsTwoPlan.overlaySource,
               0,
               0,
               "hearts two should map to the top-left cell");

    expectRect(CardRenderer::desktopRenderPlan({ Suit::Clubs, Rank::Two }).overlaySource,
               0,
               CardRenderer::SPRITE_SHEET_CELL_H,
               "clubs should use the second row even though its enum ordinal differs");

    expectRect(CardRenderer::desktopRenderPlan({ Suit::Diamonds, Rank::Ace }).overlaySource,
               12 * CardRenderer::SPRITE_SHEET_CELL_W,
               2 * CardRenderer::SPRITE_SHEET_CELL_H,
               "diamonds ace should use the explicit row and final column");

    expectRect(CardRenderer::desktopRenderPlan({ Suit::Spades, Rank::King }).overlaySource,
               11 * CardRenderer::SPRITE_SHEET_CELL_W,
               3 * CardRenderer::SPRITE_SHEET_CELL_H,
               "spades king should use the fourth row and king column");
}

void testDefaultHandLayoutPreset() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::defaultHandLayout();
    expectEqual(layout.cardW, 40, "default layout cardW");
    expectEqual(layout.cardH, 56, "default layout cardH");
    expectEqual(layout.cardSpacing, 36, "default layout cardSpacing");
    expectEqual(layout.selectOffset, 12, "default layout selectOffset");
    expectEqual(layout.cursorW, 8, "default layout cursorW");
    expectEqual(layout.cursorH, 4, "default layout cursorH");
    expectEqual(layout.cursorGap, 4, "default layout cursorGap");

    const int totalWidth = CardRenderer::handWidthForCount(8, layout);
    expectEqual(totalWidth, 7 * 36 + 40, "default layout total width for 8 cards");
}

void testGameplayHandLayoutPreset() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    expectEqual(layout.cardW, 52, "gameplay layout cardW");
    expectEqual(layout.cardH, 70, "gameplay layout cardH");
    expectEqual(layout.cardSpacing, 28, "gameplay layout cardSpacing");
    expectEqual(layout.selectOffset, 14, "gameplay layout selectOffset");
    expectEqual(layout.cursorW, 10, "gameplay layout cursorW");
    expectEqual(layout.cursorH, 5, "gameplay layout cursorH");
    expectEqual(layout.cursorGap, 5, "gameplay layout cursorGap");

    const int totalWidth = CardRenderer::handWidthForCount(8, layout);
    expectEqual(totalWidth, 7 * 28 + 52, "gameplay layout total width for 8 cards");

    // handStartX(200, 8) = 200 - 248/2 = 200 - 124 = 76
    const int startX = CardRenderer::handStartX(200, 8, layout);
    expect(startX >= 0, "gameplay hand start X should be non-negative for centerX=200");

    // last card right edge = startX + 7 * spacing + cardW
    const int lastCardRightEdge = startX + (8 - 1) * layout.cardSpacing + layout.cardW;
    expect(lastCardRightEdge <= 400, "gameplay last card right edge should be within 400px top screen width");
}

void testHitTestOneCardHand() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // One card centered at 200: startX = 200 - 52/2 = 174, right edge = 174 + 52 = 226
    const int centerX = 200;
    const int cardCount = 1;
    const int startX = CardRenderer::handStartX(centerX, cardCount, layout);
    // Click in the center of the card
    const int hitIdx = CardRenderer::handIndexAtX(startX + layout.cardW / 2, centerX, cardCount, layout);
    expectEqual(hitIdx, 0, "one-card hand: center click should hit index 0");
}

void testHitTestEightCardHandFirstBucket() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 200 - (7*28+52)/2 = 200 - 124 = 76
    const int centerX = 200;
    const int cardCount = 8;
    // x=80 → bucket (80-76)/28 = 0 → card 0
    const int hitIdx = CardRenderer::handIndexAtX(80, centerX, cardCount, layout);
    expectEqual(hitIdx, 0, "8-card hand: click at x=80 should hit first card (index 0)");
}

void testHitTestEightCardHandMiddleBucket() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 76
    // x=110 → bucket (110-76)/28 = 1 → card 1
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(110, centerX, cardCount, layout);
    expectEqual(hitIdx, 1, "8-card hand: click at x=110 should hit index 1 (middle overlap bucket)");
}

void testHitTestEightCardHandLastCard() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 76; card 7 at 76+7*28=268; right edge=268+52=320
    // x=300 → bucket (300-76)/28 = 8, clamped to 7 → last card
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(300, centerX, cardCount, layout);
    expectEqual(hitIdx, 7, "8-card hand: click at x=300 in far-right exposed region should hit last card (index 7)");
}

void testHitTestOutsideHandLeftReturnsMinusOne() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 76; click at x=75 is left of startX
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(75, centerX, cardCount, layout);
    expectEqual(hitIdx, -1, "8-card hand: click left of hand (x=75) should return -1");
}

void testHitTestOutsideHandRightReturnsMinusOne() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // lastCardRightEdge = 76 + 7*28 + 52 = 76 + 196 + 52 = 324
    // click at x=325 is right of right edge
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(325, centerX, cardCount, layout);
    expectEqual(hitIdx, -1, "8-card hand: click right of hand (x=325) should return -1");
}

// Compile-level API stability check: verify that drawCard and drawHand accept an optional
// HandLayoutMetrics parameter. This test never runs (the lambda is never called), but it will
// fail to compile if the signatures are missing the layout overload.
void testDrawSignaturesAcceptLayoutParameter() {
    // Verify that drawCard signature accepts layout as an optional trailing parameter.
    // We only check that the call is well-formed; we pass nullptr for app since this is
    // a compile-time check and will never execute at runtime.
    auto checkDrawCard = [](const CardRenderer::HandLayoutMetrics& layout) {
        Card c{Suit::Hearts, Rank::Ace};
        // Must compile: drawCard with explicit layout
        CardRenderer::drawCard(nullptr, c, 0, 0, false, layout);
        // Must compile: drawCard with default layout (omitting layout param)
        CardRenderer::drawCard(nullptr, c, 0, 0);
    };
    (void)checkDrawCard;

    auto checkDrawHand = [](const CardRenderer::HandLayoutMetrics& layout) {
        Hand h;
        // Must compile: drawHand with explicit layout
        CardRenderer::drawHand(nullptr, h, 200, 85, -1, layout);
        // Must compile: drawHand with default layout (omitting layout param)
        CardRenderer::drawHand(nullptr, h, 200, 85);
    };
    (void)checkDrawHand;
}

} // namespace

int main() {
    testSpriteSheetMappingUsesExplicitSuitAndRankOrder();
    testDefaultHandLayoutPreset();
    testGameplayHandLayoutPreset();
    testHitTestOneCardHand();
    testHitTestEightCardHandFirstBucket();
    testHitTestEightCardHandMiddleBucket();
    testHitTestEightCardHandLastCard();
    testHitTestOutsideHandLeftReturnsMinusOne();
    testHitTestOutsideHandRightReturnsMinusOne();
    testDrawSignaturesAcceptLayoutParameter();
    std::cout << "CardRenderer sprite-sheet tests passed\n";
    return 0;
}

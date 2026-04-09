#include "game/CardRenderer.h"
#include "states/GameplayState.h"

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

void expectEqual(const std::string& actual, const std::string& expected, const std::string& label) {
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
    expect(!heartsTwoPlan.drawBaseTexture,
           "desktop renderer should treat the sprite sheet as a complete card image");
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
    expectEqual(layout.cardW, 40, "gameplay layout cardW");
    expectEqual(layout.cardH, 56, "gameplay layout cardH");
    expectEqual(layout.cardSpacing, 36, "gameplay layout cardSpacing");
    expectEqual(layout.selectOffset, 12, "gameplay layout selectOffset");
    expectEqual(layout.cursorW, 8, "gameplay layout cursorW");
    expectEqual(layout.cursorH, 4, "gameplay layout cursorH");
    expectEqual(layout.cursorGap, 4, "gameplay layout cursorGap");

    const int totalWidth = CardRenderer::handWidthForCount(8, layout);
    expectEqual(totalWidth, 7 * 36 + 40, "gameplay layout total width for 8 cards");

    // handStartX(200, 8) = 200 - 292/2 = 54
    const int startX = CardRenderer::handStartX(200, 8, layout);
    expect(startX >= 0, "gameplay hand start X should be non-negative for centerX=200");

    // last card right edge = startX + 7 * spacing + cardW
    const int lastCardRightEdge = startX + (8 - 1) * layout.cardSpacing + layout.cardW;
    expect(lastCardRightEdge <= 400, "gameplay last card right edge should be within 400px top screen width");
}

void testHitTestOneCardHand() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // One card centered at 200: startX = 200 - 40/2 = 180, right edge = 220
    const int centerX = 200;
    const int cardCount = 1;
    const int startX = CardRenderer::handStartX(centerX, cardCount, layout);
    // Click in the center of the card
    const int hitIdx = CardRenderer::handIndexAtX(startX + layout.cardW / 2, centerX, cardCount, layout);
    expectEqual(hitIdx, 0, "one-card hand: center click should hit index 0");
}

void testHitTestEightCardHandFirstBucket() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 200 - (7*36+40)/2 = 54
    const int centerX = 200;
    const int cardCount = 8;
    // x=80 → bucket (80-54)/36 = 0 → card 0
    const int hitIdx = CardRenderer::handIndexAtX(80, centerX, cardCount, layout);
    expectEqual(hitIdx, 0, "8-card hand: click at x=80 should hit first card (index 0)");
}

void testHitTestEightCardHandMiddleBucket() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 54
    // x=110 → bucket (110-54)/36 = 1 → card 1
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(110, centerX, cardCount, layout);
    expectEqual(hitIdx, 1, "8-card hand: click at x=110 should hit index 1 (middle overlap bucket)");
}

void testHitTestEightCardHandLastCard() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 54; card 7 at 54+7*36=306; right edge=306+40=346
    // x=330 → bucket (330-54)/36 = 7 → last card
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(330, centerX, cardCount, layout);
    expectEqual(hitIdx, 7, "8-card hand: click at x=330 in far-right exposed region should hit last card (index 7)");
}

void testHitTestOutsideHandLeftReturnsMinusOne() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // handStartX(200, 8) = 54; click at x=53 is left of startX
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(53, centerX, cardCount, layout);
    expectEqual(hitIdx, -1, "8-card hand: click left of hand (x=53) should return -1");
}

void testHitTestOutsideHandRightReturnsMinusOne() {
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();
    // The rightmost visible pixel is x=345. x=346 is already outside the card rect.
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(346, centerX, cardCount, layout);
    expectEqual(hitIdx, -1, "8-card hand: click right of hand (x=346) should return -1");
}

void testGameplayHandPlacementAssumptions() {
    constexpr int kMaxCards = 8;

    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();

    // 8-card gameplay layout fits within the 400px top screen
    const int startX = CardRenderer::handStartX(topLayout.handCenterX, kMaxCards, layout);
    expect(startX >= 0, "gameplay placement: hand startX should be >= 0 for 8 cards at centerX=200");
    const int endX = startX + (kMaxCards - 1) * layout.cardSpacing + layout.cardW;
    expect(endX <= 400, "gameplay placement: hand right edge should fit within 400px top screen");

    // Selected lift of 14 still leaves room above (no negative Y)
    const int selectedTopY = topLayout.handY - layout.selectOffset;
    expect(selectedTopY >= 0, "gameplay placement: selected card top Y should be non-negative");

    const int jokerStripBottomY = topLayout.jokerStripY + topLayout.jokerBoxH;
    expect(selectedTopY > jokerStripBottomY,
           "gameplay placement: lifted cards should clear the compact joker/header strip");

    // Bottom bound fits within 240px screen height
    const int bottomBound = topLayout.handY + layout.cardH + layout.cursorGap + layout.cursorH;
    expect(bottomBound <= 240, "gameplay placement: bottom hit-test bound should fit within 240px screen height");
}

void testGameplayTopScreenCompactLayoutContract() {
    const auto layout = gameplay_state_helpers::compactTopScreenLayout();

    expectEqual(layout.anteX, 10, "top layout anteX");
    expectEqual(layout.anteY, 6, "top layout anteY");
    expectEqual(layout.blindX, 10, "top layout blindX");
    expectEqual(layout.blindY, 22, "top layout blindY");
    expectEqual(layout.moneyX, 346, "top layout moneyX");
    expectEqual(layout.moneyY, 6, "top layout moneyY");
    expectEqual(layout.jokerStripY, 34, "top layout jokerStripY");
    expectEqual(layout.jokerBoxW, 24, "top layout jokerBoxW");
    expectEqual(layout.jokerBoxH, 36, "top layout jokerBoxH");
    expectEqual(layout.jokerSpacing, 28, "top layout jokerSpacing");
    expectEqual(layout.handCenterX, 200, "top layout handCenterX");
    expectEqual(layout.handY, 170, "top layout handY");
    expectEqual(layout.bossLabelY, 74, "top layout bossLabelY");
    expectEqual(layout.idleStagePanelRect.x, 60, "top layout idleStagePanelRect.x");
    expectEqual(layout.idleStagePanelRect.y, 92, "top layout idleStagePanelRect.y");
    expectEqual(layout.idleStagePanelRect.w, 280, "top layout idleStagePanelRect.w");
    expectEqual(layout.idleStagePanelRect.h, 68, "top layout idleStagePanelRect.h");
    expectEqual(layout.idleStagePromptX, 148, "top layout idleStagePromptX");
    expectEqual(layout.idleStagePromptY, 116, "top layout idleStagePromptY");

    expectEqual(gameplay_state_helpers::compactJokerLabel("Blueprint"), std::string("Bluepr"),
                "compact joker label should truncate to 6 chars");
    expectEqual(gameplay_state_helpers::compactJokerLabel("N"), std::string("N"),
                "compact joker label should keep short names intact");

    const int resultBannerBottomY = layout.resultBannerY + layout.resultBannerH;
    expect(resultBannerBottomY <= layout.jokerStripY,
           "result banner should stay above the compact joker strip");

    const int centeredStripStartX = gameplay_state_helpers::jokerStripStartX(3, layout);
    expectEqual(centeredStripStartX, layout.handCenterX - gameplay_state_helpers::jokerStripWidth(3, layout) / 2,
                "joker strip centering should use the shared hand center");
}

void testGameplayInputHelpersUseSharedLayoutContract() {
    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const auto handLayout = CardRenderer::gameplayHandLayout();

    expectEqual(gameplay_state_helpers::gameplayHandHitTop(topLayout, handLayout),
                topLayout.handY - handLayout.selectOffset,
                "top hand hit bound should derive from shared hand Y");
    expectEqual(gameplay_state_helpers::gameplayHandHitBottom(topLayout, handLayout),
                topLayout.handY + handLayout.cardH + handLayout.cursorGap + handLayout.cursorH,
                "bottom hand hit bound should derive from shared hand Y");

    const int cardIndex = gameplay_state_helpers::gameplayHandIndexAtPoint(
        topLayout.handCenterX, topLayout.handY, 8, topLayout, handLayout);
    expectEqual(cardIndex, 4,
                "hand point hit-test should use the shared top layout center for card selection");
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
    testGameplayHandPlacementAssumptions();
    testGameplayTopScreenCompactLayoutContract();
    testGameplayInputHelpersUseSharedLayoutContract();
    testDrawSignaturesAcceptLayoutParameter();
    std::cout << "CardRenderer sprite-sheet tests passed\n";
    return 0;
}

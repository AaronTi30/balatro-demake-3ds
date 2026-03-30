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
    // The rightmost visible pixel is x=323. x=324 is already outside the card rect.
    const int centerX = 200;
    const int cardCount = 8;
    const int hitIdx = CardRenderer::handIndexAtX(324, centerX, cardCount, layout);
    expectEqual(hitIdx, -1, "8-card hand: click right of hand (x=324) should return -1");
}

void testGameplayHandPlacementAssumptions() {
    // These encode the intended gameplay layout constants:
    //   kGameplayHandCenterX = 200, kGameplayHandY = 96
    constexpr int kGameplayHandCenterX = 200;
    constexpr int kGameplayHandY = 96;
    constexpr int kMaxCards = 8;

    const CardRenderer::HandLayoutMetrics layout = CardRenderer::gameplayHandLayout();

    // 8-card gameplay layout fits within the 400px top screen
    const int startX = CardRenderer::handStartX(kGameplayHandCenterX, kMaxCards, layout);
    expect(startX >= 0, "gameplay placement: hand startX should be >= 0 for 8 cards at centerX=200");
    const int endX = startX + (kMaxCards - 1) * layout.cardSpacing + layout.cardW;
    expect(endX <= 400, "gameplay placement: hand right edge should fit within 400px top screen");

    // Selected lift of 14 still leaves room above (no negative Y)
    const int selectedTopY = kGameplayHandY - layout.selectOffset;
    expect(selectedTopY >= 0, "gameplay placement: selected card top Y should be non-negative");

    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const int jokerStripBottomY = topLayout.jokerStripY + topLayout.jokerBoxH;
    expect(selectedTopY > jokerStripBottomY,
           "gameplay placement: lifted cards should clear the compact joker/header strip");

    // Bottom bound fits within 240px screen height
    const int bottomBound = kGameplayHandY + layout.cardH + layout.cursorGap + layout.cursorH;
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
    expectEqual(layout.handY, 96, "top layout handY");
    expectEqual(layout.bossLabelY, 74, "top layout bossLabelY");

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

void testGameplayBottomScreenUtilityLayoutContract() {
    const auto layout = gameplay_state_helpers::compactBottomScreenLayout();

    expectEqual(layout.scoreHeaderX, 20, "bottom layout scoreHeaderX");
    expectEqual(layout.scoreHeaderY, 10, "bottom layout scoreHeaderY");
    expectEqual(layout.scoreValueX, 80, "bottom layout scoreValueX");
    expectEqual(layout.scoreValueY, 8, "bottom layout scoreValueY");
    expectEqual(layout.scoreTargetX, 140, "bottom layout scoreTargetX");
    expectEqual(layout.scoreTargetY, 10, "bottom layout scoreTargetY");
    expectEqual(layout.progressBarX, 20, "bottom layout progressBarX");
    expectEqual(layout.progressBarY, 35, "bottom layout progressBarY");
    expectEqual(layout.progressBarW, 280, "bottom layout progressBarW");
    expectEqual(layout.progressBarH, 20, "bottom layout progressBarH");
    expectEqual(layout.statusRowY, 62, "bottom layout statusRowY");
    expectEqual(layout.previewLabelY, 84, "bottom layout previewLabelY");
    expectEqual(layout.previewTypeY, 100, "bottom layout previewTypeY");
    expectEqual(layout.previewScoreY, 118, "bottom layout previewScoreY");
    expectEqual(layout.bossDescriptionY, 136, "bottom layout bossDescriptionY");
    expectEqual(layout.buttonY, 160, "bottom layout buttonY");
    expectEqual(layout.buttonW, 120, "bottom layout buttonW");
    expectEqual(layout.buttonH, 50, "bottom layout buttonH");
    expectEqual(layout.buttonGap, 20, "bottom layout buttonGap");

    expectEqual(gameplay_state_helpers::compactStatusLine(4, 3, 11), std::string("Hands 4   Discards 3   Deck 11"),
                "bottom status line should reflect current round counts");
}

void testGameplayInputHelpersUseSharedLayoutContract() {
    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const auto bottomLayout = gameplay_state_helpers::compactBottomScreenLayout();
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

    const auto play3DS = gameplay_state_helpers::bottomPlayButtonRect(bottomLayout);
    expectEqual(play3DS.x, bottomLayout.buttonX, "3DS play button rect x");
    expectEqual(play3DS.y, bottomLayout.buttonY, "3DS play button rect y");
    expectEqual(play3DS.w, bottomLayout.buttonW, "3DS play button rect w");
    expectEqual(play3DS.h, bottomLayout.buttonH, "3DS play button rect h");

    const auto discardDesktop = gameplay_state_helpers::bottomDiscardButtonRect(bottomLayout, 400);
    expectEqual(discardDesktop.x, 400 + bottomLayout.buttonX + bottomLayout.buttonW + bottomLayout.buttonGap,
                "desktop discard button rect x should include desktop base offset");
    expectEqual(discardDesktop.y, bottomLayout.buttonY, "desktop discard button rect y");
    expectEqual(discardDesktop.w, bottomLayout.buttonW, "desktop discard button rect w");
    expectEqual(discardDesktop.h, bottomLayout.buttonH, "desktop discard button rect h");
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
    testGameplayBottomScreenUtilityLayoutContract();
    testGameplayInputHelpersUseSharedLayoutContract();
    testDrawSignaturesAcceptLayoutParameter();
    std::cout << "CardRenderer sprite-sheet tests passed\n";
    return 0;
}

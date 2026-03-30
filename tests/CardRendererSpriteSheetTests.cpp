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

} // namespace

int main() {
    testSpriteSheetMappingUsesExplicitSuitAndRankOrder();
    std::cout << "CardRenderer sprite-sheet tests passed\n";
    return 0;
}

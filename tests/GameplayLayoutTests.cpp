#include "states/GameplayLayout.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expectRectEqual(const GameplayRect& actual, const GameplayRect& expected, const std::string& label) {
    if (actual.x != expected.x || actual.y != expected.y ||
        actual.w != expected.w || actual.h != expected.h) {
        std::ostringstream oss;
        oss << label << ": expected {" << expected.x << ", " << expected.y
            << ", " << expected.w << ", " << expected.h << "}, got {"
            << actual.x << ", " << actual.y << ", " << actual.w << ", " << actual.h << "}";
        fail(oss.str());
    }
}

void expectActionEqual(GameplayHudAction actual, GameplayHudAction expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << static_cast<int>(expected)
            << ", got " << static_cast<int>(actual);
        fail(oss.str());
    }
}

void testPrimaryButtonRects() {
    expectRectEqual(gameplayPlayButtonRect(),
                    GameplayRect{12, 176, 118, 36},
                    "play button rect");
    expectRectEqual(gameplayDiscardButtonRect(),
                    GameplayRect{136, 176, 118, 36},
                    "discard button rect");
    expectRectEqual(gameplaySortButtonRect(),
                    GameplayRect{260, 176, 54, 36},
                    "sort button rect");
}

void testHudActionHitResolution() {
    expectActionEqual(resolveGameplayHudAction(20, 180),
                      GameplayHudAction::Play,
                      "play hit");
    expectActionEqual(resolveGameplayHudAction(150, 180),
                      GameplayHudAction::Discard,
                      "discard hit");
    expectActionEqual(resolveGameplayHudAction(265, 180),
                      GameplayHudAction::Sort,
                      "sort hit");
    expectActionEqual(resolveGameplayHudAction(5, 5),
                      GameplayHudAction::None,
                      "outside HUD buttons should miss");
}

void testSmallActionButtonsStillHaveExclusiveEdges() {
    expectActionEqual(resolveGameplayHudAction(260, 176),
                      GameplayHudAction::Sort,
                      "sort button top-left edge should hit");
    expectActionEqual(resolveGameplayHudAction(314, 176),
                      GameplayHudAction::None,
                      "sort button right edge should remain exclusive");
}

void testRoundProgressFillWidth() {
    expectEqual(roundProgressFillWidth(0, 300, 96), 0,
                "zero score yields zero fill");
    expectEqual(roundProgressFillWidth(150, 300, 96), 48,
                "half progress yields half fill");
    expectEqual(roundProgressFillWidth(999, 300, 96), 96,
                "fill width clamps to track width");
}

} // namespace

int main() {
    testPrimaryButtonRects();
    testHudActionHitResolution();
    testSmallActionButtonsStillHaveExclusiveEdges();
    testRoundProgressFillWidth();
    return 0;
}

#include "states/GameplayHudStyle.h"

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

void expectColorEqual(const GameplayHudColor& actual,
                      const GameplayHudColor& expected,
                      const std::string& label) {
    if (actual.r != expected.r || actual.g != expected.g || actual.b != expected.b) {
        std::ostringstream oss;
        oss << label << ": expected {" << expected.r << ", " << expected.g << ", " << expected.b
            << "}, got {" << actual.r << ", " << actual.g << ", " << actual.b << "}";
        fail(oss.str());
    }
}

void testBalatroPaletteMatchesReference() {
    expectColorEqual(gameplayHudNeutralShellColor(), GameplayHudColor{0x37, 0x42, 0x44}, "neutral shell");
    expectColorEqual(gameplayHudNeutralInsetColor(), GameplayHudColor{0x4F, 0x63, 0x67}, "neutral inset");
    expectColorEqual(gameplayHudOutlineColor(), GameplayHudColor{0xBF, 0xC7, 0xD5}, "outline");
    expectColorEqual(gameplayHudChipsColor(), GameplayHudColor{0x00, 0x9D, 0xFF}, "chips");
    expectColorEqual(gameplayHudMultColor(), GameplayHudColor{0xFE, 0x5F, 0x55}, "mult");
    expectColorEqual(gameplayHudMoneyColor(), GameplayHudColor{0xF3, 0xB9, 0x58}, "money");
    expectColorEqual(gameplayHudSortAccentColor(), GameplayHudColor{0xFD, 0xA2, 0x00}, "sort accent");
}

void testPrimaryActionStylesFollowBalatroColors() {
    const GameplayHudActionStyle play = gameplayPrimaryActionStyle(GameplayHudAction::Play, true);
    const GameplayHudActionStyle discard = gameplayPrimaryActionStyle(GameplayHudAction::Discard, true);
    const GameplayHudActionStyle sort = gameplayPrimaryActionStyle(GameplayHudAction::Sort, true);

    expectColorEqual(play.fill, gameplayHudChipsColor(), "enabled play fill");
    expectColorEqual(discard.fill, gameplayHudMultColor(), "enabled discard fill");
    expectColorEqual(sort.fill, gameplayHudSortAccentColor(), "enabled sort fill");

    expectColorEqual(play.text, GameplayHudColor{0xFF, 0xFF, 0xFF}, "enabled play text");
    expectColorEqual(discard.text, GameplayHudColor{0xFF, 0xFF, 0xFF}, "enabled discard text");
    expectColorEqual(sort.text, GameplayHudColor{0xFF, 0xFF, 0xFF}, "enabled sort text");
}

void testDisabledActionStyleMutesButtons() {
    const GameplayHudColor mutedFill{0x5F, 0x73, 0x77};
    const GameplayHudColor mutedShadow{0x37, 0x42, 0x44};
    const GameplayHudColor mutedText{0xBF, 0xC7, 0xD5};

    const GameplayHudActionStyle play = gameplayPrimaryActionStyle(GameplayHudAction::Play, false);
    const GameplayHudActionStyle discard = gameplayPrimaryActionStyle(GameplayHudAction::Discard, false);
    const GameplayHudActionStyle sort = gameplayPrimaryActionStyle(GameplayHudAction::Sort, false);

    expectColorEqual(play.fill, mutedFill, "disabled play fill");
    expectColorEqual(discard.fill, mutedFill, "disabled discard fill");
    expectColorEqual(sort.fill, mutedFill, "disabled sort fill");

    expectColorEqual(play.shadow, mutedShadow, "disabled play shadow");
    expectColorEqual(discard.shadow, mutedShadow, "disabled discard shadow");
    expectColorEqual(sort.shadow, mutedShadow, "disabled sort shadow");

    expectColorEqual(play.text, mutedText, "disabled play text");
    expectColorEqual(discard.text, mutedText, "disabled discard text");
    expectColorEqual(sort.text, mutedText, "disabled sort text");

    expect(play.fill.r < gameplayHudSortAccentColor().r, "disabled fill should mute the accent family");
    expect(play.text.r > 120, "disabled text should remain readable");
}

void testDisabledSortStillUsesBalatroUtilityFamily() {
    const GameplayHudActionStyle sort = gameplayPrimaryActionStyle(GameplayHudAction::Sort, false);
    expect(sort.fill.r >= 60 && sort.fill.r <= 120,
           "disabled sort fill stays in muted slate range");
}

void testChipsAndMultRetainOriginalBalatroAccentColors() {
    expectColorEqual(gameplayHudChipsColor(), GameplayHudColor{0x00, 0x9D, 0xFF}, "chips accent");
    expectColorEqual(gameplayHudMultColor(),  GameplayHudColor{0xFE, 0x5F, 0x55}, "mult accent");
}

} // namespace

int main() {
    testBalatroPaletteMatchesReference();
    testPrimaryActionStylesFollowBalatroColors();
    testDisabledActionStyleMutesButtons();
    testDisabledSortStillUsesBalatroUtilityFamily();
    testChipsAndMultRetainOriginalBalatroAccentColors();
    return 0;
}

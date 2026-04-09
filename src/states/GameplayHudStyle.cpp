#include "states/GameplayHudStyle.h"

namespace {

constexpr GameplayHudColor kNeutralShell{0x37, 0x42, 0x44};
constexpr GameplayHudColor kNeutralInset{0x4F, 0x63, 0x67};
constexpr GameplayHudColor kNeutralGrey{0x5F, 0x73, 0x77};
constexpr GameplayHudColor kOutline{0xBF, 0xC7, 0xD5};
constexpr GameplayHudColor kChips{0x00, 0x9D, 0xFF};
constexpr GameplayHudColor kMult{0xFE, 0x5F, 0x55};
constexpr GameplayHudColor kMoney{0xF3, 0xB9, 0x58};
constexpr GameplayHudColor kSortAccent{0xFD, 0xA2, 0x00};
constexpr GameplayHudColor kActionText{0xFF, 0xFF, 0xFF};

GameplayHudColor darken(GameplayHudColor color) {
    return GameplayHudColor{
        color.r * 3 / 4,
        color.g * 3 / 4,
        color.b * 3 / 4,
    };
}

GameplayHudColor actionFillFor(GameplayHudAction action) {
    switch (action) {
        case GameplayHudAction::Play:
            return kChips;
        case GameplayHudAction::Discard:
            return kMult;
        case GameplayHudAction::Sort:
            return kSortAccent;
        case GameplayHudAction::None:
            return kNeutralInset;
    }

    return kNeutralInset;
}

} // namespace

GameplayHudColor gameplayHudNeutralShellColor() {
    return kNeutralShell;
}

GameplayHudColor gameplayHudNeutralInsetColor() {
    return kNeutralInset;
}

GameplayHudColor gameplayHudOutlineColor() {
    return kOutline;
}

GameplayHudColor gameplayHudChipsColor() {
    return kChips;
}

GameplayHudColor gameplayHudMultColor() {
    return kMult;
}

GameplayHudColor gameplayHudMoneyColor() {
    return kMoney;
}

GameplayHudColor gameplayHudSortAccentColor() {
    return kSortAccent;
}

GameplayHudActionStyle gameplayPrimaryActionStyle(GameplayHudAction action, bool enabled) {
    if (!enabled || action == GameplayHudAction::None) {
        return GameplayHudActionStyle{
            kNeutralGrey,
            kNeutralShell,
            kOutline,
        };
    }

    const GameplayHudColor fill = actionFillFor(action);
    return GameplayHudActionStyle{
        fill,
        darken(fill),
        kActionText,
    };
}

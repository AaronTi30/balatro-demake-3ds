#pragma once

#include "states/GameplayLayout.h"

struct GameplayHudColor {
    int r;
    int g;
    int b;
};

struct GameplayHudActionStyle {
    GameplayHudColor fill;
    GameplayHudColor shadow;
    GameplayHudColor text;
};

GameplayHudColor gameplayHudNeutralShellColor();
GameplayHudColor gameplayHudNeutralInsetColor();
GameplayHudColor gameplayHudOutlineColor();
GameplayHudColor gameplayHudChipsColor();
GameplayHudColor gameplayHudMultColor();
GameplayHudColor gameplayHudMoneyColor();
GameplayHudColor gameplayHudSortAccentColor();

GameplayHudActionStyle gameplayPrimaryActionStyle(GameplayHudAction action, bool enabled);

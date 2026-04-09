#pragma once

struct GameplayRect { int x, y, w, h; };

enum class GameplayHudAction { None, Play, Discard, Sort };

GameplayRect gameplayPlayButtonRect();
GameplayRect gameplayDiscardButtonRect();
GameplayRect gameplaySortButtonRect();

GameplayHudAction resolveGameplayHudAction(int px, int py);
int roundProgressFillWidth(int roundScore, int roundTarget, int trackWidth);

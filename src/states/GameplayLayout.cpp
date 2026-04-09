#include "states/GameplayLayout.h"

namespace {

bool pointInRect(const GameplayRect& rect, int px, int py) {
    return px >= rect.x && px < rect.x + rect.w &&
           py >= rect.y && py < rect.y + rect.h;
}

} // namespace

GameplayRect gameplayPlayButtonRect() {
    return {12, 176, 118, 36};
}

GameplayRect gameplayDiscardButtonRect() {
    return {136, 176, 118, 36};
}

GameplayRect gameplaySortButtonRect() {
    return {260, 176, 54, 36};
}

GameplayHudAction resolveGameplayHudAction(int px, int py) {
    if (pointInRect(gameplayPlayButtonRect(), px, py)) {
        return GameplayHudAction::Play;
    }
    if (pointInRect(gameplayDiscardButtonRect(), px, py)) {
        return GameplayHudAction::Discard;
    }
    if (pointInRect(gameplaySortButtonRect(), px, py)) {
        return GameplayHudAction::Sort;
    }
    return GameplayHudAction::None;
}

int roundProgressFillWidth(int roundScore, int roundTarget, int trackWidth) {
    if (roundScore <= 0 || roundTarget <= 0 || trackWidth <= 0) {
        return 0;
    }
    if (roundScore >= roundTarget) {
        return trackWidth;
    }
    return (roundScore * trackWidth) / roundTarget;
}

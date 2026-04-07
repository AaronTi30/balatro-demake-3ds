#pragma once

#include "../core/State.h"
#include "../game/CardRenderer.h"
#include "../game/Hand.h"
#include "../game/HandType.h"
#include "../game/HandEvaluator.h"
#include "../game/RunState.h"
#include <memory>
#include <string>

namespace gameplay_state_helpers {

inline constexpr int kIdleStagePanelColorR = 60;
inline constexpr int kIdleStagePanelColorG = 60;
inline constexpr int kIdleStagePanelColorB = 80;

struct ScreenRect {
    int x;
    int y;
    int w;
    int h;
};

struct CompactTopScreenLayout {
    int anteX;
    int anteY;
    int blindX;
    int blindY;
    int moneyX;
    int moneyY;
    int jokerStripY;
    int jokerBoxW;
    int jokerBoxH;
    int jokerSpacing;
    int handCenterX;
    int handY;
    int bossLabelY;
    ScreenRect idleStagePanelRect;
    int idleStagePromptX;
    int idleStagePromptY;
    int resultBannerX;
    int resultBannerY;
    int resultBannerW;
    int resultBannerH;
};

struct CompactBottomScreenLayout {
    int scoreHeaderX;
    int scoreHeaderY;
    int scoreValueX;
    int scoreValueY;
    int scoreTargetX;
    int scoreTargetY;
    int progressBarX;
    int progressBarY;
    int progressBarW;
    int progressBarH;
    int statusRowY;
    int previewLabelY;
    int previewTypeY;
    int previewScoreY;
    int bossDescriptionY;
    int buttonX;
    int buttonY;
    int buttonW;
    int buttonH;
    int buttonGap;
};

inline CompactTopScreenLayout compactTopScreenLayout() {
    return {
        10, 6,
        10, 22,
        346, 6,
        34,
        24, 36,
        28,
        200, 170,
        74,
        {60, 92, 280, 68},
        148, 116,
        100, 0, 220, 20
    };
}

inline CompactBottomScreenLayout compactBottomScreenLayout() {
    return {
        20, 10,
        80, 8,
        140, 10,
        20, 35, 280, 20,
        62,
        84, 100, 118,
        136,
        20, 160, 120, 50, 20
    };
}

inline std::string compactJokerLabel(const std::string& name) {
    return name.substr(0, 6);
}

inline std::string compactStatusLine(int handsRemaining, int discardsRemaining, int deckRemaining) {
    return "Hands " + std::to_string(handsRemaining) +
        "   Discards " + std::to_string(discardsRemaining) +
        "   Deck " + std::to_string(deckRemaining);
}

inline int jokerStripWidth(int jokerCount, const CompactTopScreenLayout& layout) {
    if (jokerCount <= 0) {
        return 0;
    }
    return (jokerCount - 1) * layout.jokerSpacing + layout.jokerBoxW;
}

inline int jokerStripStartX(int jokerCount, const CompactTopScreenLayout& layout) {
    return layout.handCenterX - jokerStripWidth(jokerCount, layout) / 2;
}

inline int gameplayHandHitTop(const CompactTopScreenLayout& topLayout,
                              const CardRenderer::HandLayoutMetrics& handLayout) {
    return topLayout.handY - handLayout.selectOffset;
}

inline int gameplayHandHitBottom(const CompactTopScreenLayout& topLayout,
                                 const CardRenderer::HandLayoutMetrics& handLayout) {
    return topLayout.handY + handLayout.cardH + handLayout.cursorGap + handLayout.cursorH;
}

inline int gameplayHandIndexAtPoint(int px,
                                    int py,
                                    int cardCount,
                                    const CompactTopScreenLayout& topLayout,
                                    const CardRenderer::HandLayoutMetrics& handLayout) {
    if (py < gameplayHandHitTop(topLayout, handLayout) || py > gameplayHandHitBottom(topLayout, handLayout)) {
        return -1;
    }

    return CardRenderer::handIndexAtX(px, topLayout.handCenterX, cardCount, handLayout);
}

inline ScreenRect bottomPlayButtonRect(const CompactBottomScreenLayout& layout, int baseX = 0) {
    return { baseX + layout.buttonX, layout.buttonY, layout.buttonW, layout.buttonH };
}

inline ScreenRect bottomDiscardButtonRect(const CompactBottomScreenLayout& layout, int baseX = 0) {
    return {
        baseX + layout.buttonX + layout.buttonW + layout.buttonGap,
        layout.buttonY,
        layout.buttonW,
        layout.buttonH
    };
}

inline bool pointInRect(int px, int py, const ScreenRect& rect) {
    return px >= rect.x && px <= rect.x + rect.w &&
        py >= rect.y && py <= rect.y + rect.h;
}

} // namespace gameplay_state_helpers

enum class RoundPhase {
    Playing,       // Normal card play
    RoundWon,      // Beat the target — show summary
    GameOver,      // Ran out of hands — show final score
    GameWon        // Beat all 8 antes
};

class GameplayState : public State {
public:
    GameplayState(StateMachine* machine, std::shared_ptr<RunState> runState);
    ~GameplayState() override = default;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app, ScreenRenderer& r) override;
    void renderBottomScreen(Application* app, ScreenRenderer& r) override;

private:
    void playHand();
    void discardSelected();
    void drawToFull();
    void startNewRound();
    void checkRoundEnd();

    std::shared_ptr<RunState> m_runState;
    Hand m_hand;

    int m_cursorIndex;
    RoundPhase m_phase;
    float m_phaseTimer;  // Countdown for summary/gameover screens

    // Last played hand info (for display)
    HandType m_lastHandType;
    int m_lastChips;
    int m_lastMult;
    int m_lastScore;
    bool m_showResult;
    float m_resultTimer;
    float m_inputDelay;
};

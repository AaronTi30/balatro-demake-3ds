#pragma once

#include "../core/State.h"
#include "../game/CardRenderer.h"
#include "../game/Hand.h"
#include "../game/HandType.h"
#include "../game/HandEvaluator.h"
#include "../game/RunState.h"
#include "../game/ScoringAnimator.h"
#include "../states/GameplayLayout.h"
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

inline std::string compactJokerLabel(const std::string& name) {
    return name.substr(0, 6);
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

} // namespace gameplay_state_helpers

enum class RoundPhase {
    Playing,       // Normal card play
    Scoring,       // Hand is animating and score is being committed
    RoundWon,      // Beat the target — show summary
    GameOver,      // Ran out of hands — show final score
    GameWon        // Beat all 8 antes
};

enum class GameplaySortMode {
    Rank,
    Suit
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
    void handleHudAction(GameplayHudAction action);
    void playHand();
    void discardSelected();
    void applyCurrentSort();
    void toggleSortMode();
    void drawToFull();
    void startNewRound();
    void checkRoundEnd();

    std::shared_ptr<RunState> m_runState;
    Hand m_hand;

    int m_cursorIndex;
    GameplaySortMode m_sortMode;
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
    std::unique_ptr<ScoringAnimator> m_scorer;
};

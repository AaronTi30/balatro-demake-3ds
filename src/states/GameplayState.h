#pragma once

#include "../core/State.h"
#include "../game/Deck.h"
#include "../game/Hand.h"
#include "../game/HandEvaluator.h"

// ── Ante targets (Balatro-style escalation) ──
// Ante 1=300, 2=450, 3=600, 4=800, 5=1100, 6=1500, 7=2000, 8=2800
static const int ANTE_TARGETS[] = { 300, 450, 600, 800, 1100, 1500, 2000, 2800 };
static const int MAX_ANTE = 8;

enum class RoundPhase {
    Playing,       // Normal card play
    RoundWon,      // Beat the target — show summary
    GameOver,      // Ran out of hands — show final score
    GameWon        // Beat all 8 antes
};

class GameplayState : public State {
public:
    GameplayState(StateMachine* machine);
    ~GameplayState() override = default;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    void playHand();
    void discardSelected();
    void drawToFull();
    void startNewRound();
    void checkRoundEnd();

    Deck m_deck;
    Hand m_hand;
    int m_cursorIndex;
    int m_handsRemaining;
    int m_discardsRemaining;
    int m_score;
    int m_roundTarget;

    // Ante / round tracking
    int m_ante;       // 1-based (1 to MAX_ANTE)
    int m_round;      // Round within the ante (for display)
    RoundPhase m_phase;
    float m_phaseTimer;  // Countdown for summary/gameover screens

    // Last played hand info (for display)
    HandType m_lastHandType;
    int m_lastChips;
    int m_lastMult;
    int m_lastScore;
    bool m_showResult;
    float m_resultTimer;
};

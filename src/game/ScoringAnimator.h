#pragma once

#include "Card.h"
#include "HandType.h"
#include "HandEvaluator.h"
#include "Joker.h"
#include <utility>
#include <vector>

class Application;
class ScreenRenderer;

class ScoringAnimator {
public:
    struct RenderCardState {
        Card card;
        int drawX;
        int drawY;
        bool highlight;
    };

    ScoringAnimator(std::vector<Card> cards,
                    std::vector<std::pair<int,int>> startPositions,
                    std::vector<Joker> jokers,
                    HandResult result,
                    int currentRoundScore,
                    int stageCenterX,
                    int stageY);

    void update(float dt);
    void render(Application* app, ScreenRenderer& r);
    bool isDone() const;

    int displayChips() const;
    int displayMult() const;
    int displayRoundScore() const;
    HandType handType() const;
    int activeJokerIndex() const;
    std::vector<RenderCardState> cardRenderStates() const;

private:
    enum class Stage {
        FlyToStage,
        CardScoring,
        JokerTrigger,
        ScoreTally,
        Done
    };

    struct CardAnim {
        Card card;
        float startX, startY;
        float targetX, targetY;
        float currentX, currentY;
        bool isScoring;
    };

    struct JokerSnapshot {
        int chipsAfter;
        int multAfter;
    };

    static constexpr float kFlyDuration         = 0.4f;
    static constexpr float kCardScoringDuration = 0.3f;
    static constexpr float kJokerDuration       = 0.3f;
    static constexpr float kTallyDuration       = 0.4f;

    Stage m_stage;
    float m_elapsed;

    std::vector<CardAnim>      m_cards;
    std::vector<Joker>         m_jokers;
    std::vector<JokerSnapshot> m_jokerSnapshots;
    HandResult                 m_result;
    int                        m_chipsAfterCards;

    int m_activeCardIdx;
    int m_activeJokerIdx;

    int   m_displayChips;
    int   m_displayMult;
    int   m_displayRoundScore;
    int   m_currentRoundScore;
    float m_flyOffY;

    void transitionToCardScoring();
    void transitionToJokerTrigger();
    void transitionToScoreTally();
    void applyCardScore(int cardIdx);
    void applyJokerSnapshot(int jokerIdx);
};

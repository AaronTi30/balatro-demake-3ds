#include "ScoringAnimator.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
#include "CardRenderer.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

ScoringAnimator::ScoringAnimator(
    std::vector<Card> cards,
    std::vector<std::pair<int,int>> startPositions,
    std::vector<Joker> jokers,
    HandResult result,
    int currentRoundScore,
    int stageCenterX,
    int stageY)
    : m_stage(Stage::FlyToStage)
    , m_elapsed(0.0f)
    , m_jokers(std::move(jokers))
    , m_result(std::move(result))
    , m_chipsAfterCards(0)
    , m_activeCardIdx(0)
    , m_activeJokerIdx(0)
    , m_displayChips(m_result.baseHandChips)
    , m_displayMult(m_result.baseHandMult)
    , m_displayRoundScore(currentRoundScore)
    , m_currentRoundScore(currentRoundScore)
    , m_flyOffY(0.0f)
{
    if (cards.size() != startPositions.size()) {
        throw std::invalid_argument("ScoringAnimator requires one start position per card");
    }

    const int count = static_cast<int>(cards.size());
    const auto layout = CardRenderer::gameplayHandLayout();
    const int stageStartX = CardRenderer::handStartX(stageCenterX, count, layout);

    for (int i = 0; i < count; ++i) {
        CardAnim anim;
        anim.card = cards[i];
        anim.startX = static_cast<float>(startPositions[i].first);
        anim.startY = static_cast<float>(startPositions[i].second);
        anim.targetX = static_cast<float>(stageStartX + i * layout.cardSpacing);
        anim.targetY = static_cast<float>(stageY);
        anim.currentX = anim.startX;
        anim.currentY = anim.startY;

        anim.isScoring = false;
        for (const auto& sc : m_result.scoringCards) {
            if (sc.rank == anim.card.rank && sc.suit == anim.card.suit) {
                anim.isScoring = true;
                break;
            }
        }
        m_cards.push_back(anim);
    }

    m_chipsAfterCards = m_result.baseHandChips;
    for (const auto& sc : m_result.scoringCards) {
        m_chipsAfterCards += rankChipValue(sc.rank);
    }

    int simChips = m_chipsAfterCards;
    int simMult = m_result.baseHandMult;
    std::vector<Card> playedCopy = cards;
    std::vector<Card> scoringCopy = m_result.scoringCards;

    for (const auto& joker : m_jokers) {
        if (joker.evaluate) {
            HandEvalContext ctx{
                m_result.detectedHand,
                playedCopy,
                static_cast<int>(playedCopy.size()),
                scoringCopy,
                static_cast<int>(scoringCopy.size()),
                m_result.containsPair,
                simChips,
                simMult
            };
            joker.evaluate(ctx);
        }
        m_jokerSnapshots.push_back({ simChips, simMult });
    }
}

void ScoringAnimator::update(float dt) {
    if (m_stage == Stage::Done || dt <= 0.0f) return;

    constexpr float kEpsilon = 0.0001f;
    float remaining = dt;

    while (remaining > 0.0f && m_stage != Stage::Done) {
        switch (m_stage) {
            case Stage::FlyToStage: {
                const float timeToBoundary = std::max(0.0f, kFlyDuration - m_elapsed);
                const float step = std::min(remaining, timeToBoundary);
                m_elapsed += step;
                remaining -= step;

                float t = m_elapsed / kFlyDuration;
                if (t > 1.0f) t = 1.0f;
                for (auto& c : m_cards) {
                    c.currentX = c.startX + (c.targetX - c.startX) * t;
                    c.currentY = c.startY + (c.targetY - c.startY) * t;
                }

                if (m_elapsed + kEpsilon >= kFlyDuration) {
                    for (auto& c : m_cards) {
                        c.currentX = c.targetX;
                        c.currentY = c.targetY;
                    }
                    transitionToCardScoring();
                }
                break;
            }

            case Stage::CardScoring: {
                if (m_result.scoringCards.empty()) {
                    transitionToJokerTrigger();
                    break;
                }

                const float timeToBoundary = std::max(0.0f, kCardScoringDuration - m_elapsed);
                const float step = std::min(remaining, timeToBoundary);
                m_elapsed += step;
                remaining -= step;

                if (m_elapsed + kEpsilon >= kCardScoringDuration) {
                    m_activeCardIdx++;
                    const int totalScoring = static_cast<int>(m_result.scoringCards.size());
                    if (m_activeCardIdx >= totalScoring) {
                        transitionToJokerTrigger();
                    } else {
                        m_elapsed = 0.0f;
                        applyCardScore(m_activeCardIdx);
                    }
                }
                break;
            }

            case Stage::JokerTrigger: {
                if (m_jokers.empty()) {
                    transitionToScoreTally();
                    break;
                }

                const float timeToBoundary = std::max(0.0f, kJokerDuration - m_elapsed);
                const float step = std::min(remaining, timeToBoundary);
                m_elapsed += step;
                remaining -= step;

                if (m_elapsed + kEpsilon >= kJokerDuration) {
                    m_activeJokerIdx++;
                    if (m_activeJokerIdx >= static_cast<int>(m_jokers.size())) {
                        transitionToScoreTally();
                    } else {
                        m_elapsed = 0.0f;
                        applyJokerSnapshot(m_activeJokerIdx);
                    }
                }
                break;
            }

            case Stage::ScoreTally: {
                const float timeToBoundary = std::max(0.0f, kTallyDuration - m_elapsed);
                const float step = std::min(remaining, timeToBoundary);
                m_elapsed += step;
                remaining -= step;

                float t = m_elapsed / kTallyDuration;
                if (t > 1.0f) t = 1.0f;
                m_flyOffY = t * 260.0f;
                m_displayRoundScore = m_currentRoundScore +
                    static_cast<int>(t * static_cast<float>(m_result.finalScore));
                if (m_elapsed + kEpsilon >= kTallyDuration) {
                    m_displayRoundScore = m_currentRoundScore + m_result.finalScore;
                    m_stage = Stage::Done;
                }
                break;
            }

            case Stage::Done:
                break;
        }
    }
}

std::vector<ScoringAnimator::RenderCardState> ScoringAnimator::cardRenderStates() const {
    std::vector<RenderCardState> states;
    states.reserve(m_cards.size());

    for (int i = 0; i < static_cast<int>(m_cards.size()); ++i) {
        const auto& c = m_cards[i];
        int drawX = static_cast<int>(std::round(c.currentX));
        int drawY = static_cast<int>(std::round(c.currentY));

        if (m_stage == Stage::ScoreTally || m_stage == Stage::Done) {
            drawY -= static_cast<int>(std::round(m_flyOffY));
        }

        bool highlight = false;
        if (m_stage == Stage::CardScoring && c.isScoring) {
            for (int si = 0; si < static_cast<int>(m_result.scoringCards.size()); ++si) {
                if (m_result.scoringCards[si].rank == c.card.rank &&
                    m_result.scoringCards[si].suit == c.card.suit &&
                    si == m_activeCardIdx) {
                    highlight = true;
                    break;
                }
            }
        }

        states.push_back({ c.card, drawX, drawY, highlight });
    }

    return states;
}

void ScoringAnimator::render(Application* app, ScreenRenderer& r) {
    const auto layout = CardRenderer::gameplayHandLayout();
    const auto states = cardRenderStates();

    for (int i = 0; i < static_cast<int>(states.size()); ++i) {
        CardRenderer::drawCard(app, states[i].card, states[i].drawX, states[i].drawY, false, layout);

        if (states[i].highlight) {
            r.drawRectOutline(states[i].drawX - 2, states[i].drawY - 2,
                              layout.cardW + 4, layout.cardH + 4,
                              255, 220, 50);
        }
    }
}

bool ScoringAnimator::isDone() const { return m_stage == Stage::Done; }

int ScoringAnimator::displayChips() const { return m_displayChips; }
int ScoringAnimator::displayMult() const { return m_displayMult; }
int ScoringAnimator::displayRoundScore() const { return m_displayRoundScore; }
HandType ScoringAnimator::handType() const { return m_result.detectedHand; }
int ScoringAnimator::activeJokerIndex() const {
    return (m_stage == Stage::JokerTrigger) ? m_activeJokerIdx : -1;
}

void ScoringAnimator::transitionToCardScoring() {
    m_stage = Stage::CardScoring;
    m_elapsed = 0.0f;
    m_activeCardIdx = 0;
    m_displayChips = m_result.baseHandChips;
    m_displayMult = m_result.baseHandMult;
    if (!m_result.scoringCards.empty()) {
        applyCardScore(0);
    }
}

void ScoringAnimator::transitionToJokerTrigger() {
    m_displayChips = m_chipsAfterCards;
    m_activeJokerIdx = 0;
    if (m_jokers.empty()) {
        transitionToScoreTally();
        return;
    }
    m_stage = Stage::JokerTrigger;
    m_elapsed = 0.0f;
    applyJokerSnapshot(0);
}

void ScoringAnimator::transitionToScoreTally() {
    m_stage = Stage::ScoreTally;
    m_elapsed = 0.0f;
    m_flyOffY = 0.0f;
    m_displayChips = m_result.finalChips;
    m_displayMult = m_result.finalMult;
}

void ScoringAnimator::applyCardScore(int cardIdx) {
    m_displayChips += rankChipValue(m_result.scoringCards[cardIdx].rank);
}

void ScoringAnimator::applyJokerSnapshot(int jokerIdx) {
    m_displayChips = m_jokerSnapshots[jokerIdx].chipsAfter;
    m_displayMult = m_jokerSnapshots[jokerIdx].multAfter;
}

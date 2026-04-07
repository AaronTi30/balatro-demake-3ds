#include "ScoringAnimator.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
#include "CardRenderer.h"
#include <map>

namespace {

bool detectContainsPair(const std::vector<Card>& cards) {
    std::map<Rank, int> counts;
    for (const auto& c : cards) counts[c.rank]++;
    for (const auto& p : counts) if (p.second >= 2) return true;
    return false;
}

} // namespace

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
    bool hasPair = detectContainsPair(cards);
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
                hasPair,
                simChips,
                simMult
            };
            joker.evaluate(ctx);
        }
        m_jokerSnapshots.push_back({ simChips, simMult });
    }
}

void ScoringAnimator::update(float /*dt*/) {}

void ScoringAnimator::render(Application* /*app*/, ScreenRenderer& /*r*/) {}

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

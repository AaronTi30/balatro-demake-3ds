#pragma once

#include "HandType.h"
#include "RunState.h"
#include "Card.h"
#include "Joker.h"
#include <algorithm>
#include <string>
#include <vector>

// ── Result of evaluating a hand ──
struct HandResult {
    HandType detectedHand;
    bool containsPair;
    std::vector<Card> scoringCards;
    int baseHandChips;
    int baseHandMult;
    int scoringCardChipBonus;
    int finalChips;
    int finalMult;
    int finalScore;
    bool scoreEquationExact;
};

class HandEvaluator {
public:
    // Evaluate a set of played cards (up to 5) and return the best hand
    static HandResult evaluate(std::vector<Card> cards,
                               const std::vector<Joker>& jokers = {},
                               BossBlindModifier bossModifier = BossBlindModifier::None,
                               Suit blockedSuit = Suit::Clubs,
                               const RunState* runState = nullptr);

private:
    struct ScoreTotals {
        int chips;
        int mult;
        int score;
        bool scoreEquationExact;
    };

    // Helper: count how many of each rank appear
    static std::vector<std::pair<Rank, int>> rankCounts(const std::vector<Card>& cards);

    // Helper: check if the cards form a straight (assumes sorted)
    static bool isStraight(std::vector<Card> cards);

    // Helper: check if all cards share the same suit
    static bool isFlush(const std::vector<Card>& cards);

    // Helper: determine the best hand type for the played cards
    static HandType classifyHand(const std::vector<Card>& cards,
                                 const std::vector<std::pair<Rank, int>>& counts,
                                 bool flush,
                                 bool straight);

    // Helper: gather the cards that score for the evaluated hand
    static std::vector<Card> selectScoringCards(const std::vector<Card>& cards,
                                                const std::vector<std::pair<Rank, int>>& counts,
                                                HandType type);

    // Helper: look up the base chips and mult for a hand type
    static std::pair<int, int> lookupBaseValues(HandType type, int level = 1);

    // Helper: sum the rank chip value of the scoring cards
    static int calculateScoringCardChipBonus(const std::vector<Card>& scoringCards);

    // Helper: calculate the chip amount a boss modifier removes after joker callbacks
    static int calculateBossChipPenalty(const std::vector<Card>& scoringCards,
                                        BossBlindModifier bossModifier,
                                        Suit blockedSuit);

    // Helper: apply scoring card chips and joker callbacks in explicit order
    static ScoreTotals calculateFinalTotals(HandType type,
                                            const std::vector<Card>& playedCards,
                                            const std::vector<Card>& scoringCards,
                                            bool containsPair,
                                            int chipsAfterScoringCards,
                                            int baseMult,
                                            const std::vector<Joker>& jokers,
                                            int bossChipPenalty,
                                            BossBlindModifier bossModifier);
};

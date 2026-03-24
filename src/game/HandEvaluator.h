#pragma once

#include "RunState.h"
#include "Card.h"
#include "Joker.h"
#include <algorithm>
#include <string>
#include <vector>

// ── Poker Hand Types (ordered by Balatro's ranking) ──
enum class HandType {
    HighCard,
    Pair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    RoyalFlush
};

// ── Result of evaluating a hand ──
struct HandResult {
    HandType detectedHand;
    std::vector<Card> scoringCards;
    int baseHandChips;
    int baseHandMult;
    int scoringCardChipBonus;
    int finalChips;
    int finalMult;
    int finalScore;
};

inline const char* handTypeName(HandType t) {
    switch (t) {
        case HandType::HighCard:       return "High Card";
        case HandType::Pair:           return "Pair";
        case HandType::TwoPair:        return "Two Pair";
        case HandType::ThreeOfAKind:   return "Three of a Kind";
        case HandType::Straight:       return "Straight";
        case HandType::Flush:          return "Flush";
        case HandType::FullHouse:      return "Full House";
        case HandType::FourOfAKind:    return "Four of a Kind";
        case HandType::StraightFlush:  return "Straight Flush";
        case HandType::RoyalFlush:     return "Royal Flush";
        default:                       return "???";
    }
}

class HandEvaluator {
public:
    // Evaluate a set of played cards (up to 5) and return the best hand
    static HandResult evaluate(std::vector<Card> cards,
                               const std::vector<Joker>& jokers = {},
                               BossBlindModifier bossModifier = BossBlindModifier::None,
                               Suit blockedSuit = Suit::Clubs);

private:
    struct ScoreTotals {
        int chips;
        int mult;
        int score;
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
    static std::pair<int, int> lookupBaseValues(HandType type);

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

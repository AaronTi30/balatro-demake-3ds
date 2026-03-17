#pragma once

#include "Card.h"
#include "Joker.h"
#include <vector>
#include <string>
#include <algorithm>

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
    HandType type;
    int baseChips;
    int baseMult;
    std::vector<Card> scoringCards; // Cards that contribute to the hand
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
    static HandResult evaluate(std::vector<Card> cards, const std::vector<Joker>& jokers = {});

private:
    // Helper: count how many of each rank appear
    static std::vector<std::pair<Rank, int>> rankCounts(const std::vector<Card>& cards);
    
    // Helper: check if the cards form a straight (assumes sorted)
    static bool isStraight(std::vector<Card> cards);
    
    // Helper: check if all cards share the same suit
    static bool isFlush(const std::vector<Card>& cards);
};

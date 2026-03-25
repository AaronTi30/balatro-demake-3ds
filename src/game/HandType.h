#pragma once

#include <cstddef>

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

inline constexpr std::size_t kHandTypeCount = 10;

inline constexpr std::size_t handTypeIndex(HandType type) {
    return static_cast<std::size_t>(type);
}

inline const char* handTypeName(HandType t) {
    switch (t) {
        case HandType::HighCard:      return "High Card";
        case HandType::Pair:          return "Pair";
        case HandType::TwoPair:       return "Two Pair";
        case HandType::ThreeOfAKind:  return "Three of a Kind";
        case HandType::Straight:      return "Straight";
        case HandType::Flush:         return "Flush";
        case HandType::FullHouse:     return "Full House";
        case HandType::FourOfAKind:   return "Four of a Kind";
        case HandType::StraightFlush: return "Straight Flush";
        case HandType::RoyalFlush:    return "Royal Flush";
        default:                      return "???";
    }
}

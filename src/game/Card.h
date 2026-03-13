#pragma once

#include <cstdint>
#include <string>

enum class Suit {
    Hearts,
    Diamonds,
    Clubs,
    Spades
};

enum class Rank {
    Ace = 1,
    Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten,
    Jack, Queen, King
};

struct Card {
    Suit suit;
    Rank rank;
    bool selected = false;
};

// ── Helpers ──────────────────────────────────────────

inline const char* rankToString(Rank r) {
    switch (r) {
        case Rank::Ace:   return "A";
        case Rank::Two:   return "2";
        case Rank::Three: return "3";
        case Rank::Four:  return "4";
        case Rank::Five:  return "5";
        case Rank::Six:   return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine:  return "9";
        case Rank::Ten:   return "10";
        case Rank::Jack:  return "J";
        case Rank::Queen: return "Q";
        case Rank::King:  return "K";
        default:          return "?";
    }
}

inline const char* suitToString(Suit s) {
    switch (s) {
        case Suit::Hearts:   return "H";
        case Suit::Diamonds: return "D";
        case Suit::Clubs:    return "C";
        case Suit::Spades:   return "S";
        default:             return "?";
    }
}

// Returns true if the suit should be rendered in red (Hearts, Diamonds)
inline bool suitIsRed(Suit s) {
    return s == Suit::Hearts || s == Suit::Diamonds;
}

// Chip value of a card's rank in Balatro
inline int rankChipValue(Rank r) {
    switch (r) {
        case Rank::Ace:   return 11;
        case Rank::King:  return 10;
        case Rank::Queen: return 10;
        case Rank::Jack:  return 10;
        case Rank::Ten:   return 10;
        default:          return static_cast<int>(r);
    }
}

#pragma once

#include "Card.h"
#include <vector>
#include <functional>

enum class HandType;

struct HandEvalContext {
    HandType playedHand;
    const std::vector<Card>& scoringCards;
    int& chips;
    int& mult;
};

enum class JokerEffectType {
    AddChips,
    AddMult,
    MulMult
};

struct Joker {
    std::string name;
    std::string description;
    JokerEffectType effectType; // For UI rendering colors
    
    std::function<void(HandEvalContext&)> evaluate;

    // Factory method to generate a random joker from the phase 1 pool
    static Joker getRandom();
};

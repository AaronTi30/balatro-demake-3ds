#pragma once

#include <string>

enum class JokerEffectType {
    AddChips,
    AddMult,
    MulMult
};

struct Joker {
    std::string name;
    std::string description;
    JokerEffectType effectType;
    int effectValue;

    // Factory method to generate a random joker from the phase 1 pool
    static Joker getRandom();
};

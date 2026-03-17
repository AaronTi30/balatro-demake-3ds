#include "Joker.h"
#include <vector>
#include <random>

Joker Joker::getRandom() {
    static const std::vector<Joker> pool = {
        {"Joker", "+4 Mult", JokerEffectType::AddMult, 4},
        {"Greedy Joker", "+3 Mult", JokerEffectType::AddMult, 3},
        {"Lusty Joker", "+3 Mult", JokerEffectType::AddMult, 3},
        {"Wrathful Joker", "+3 Mult", JokerEffectType::AddMult, 3},
        {"Gluttonous Joker", "+3 Mult", JokerEffectType::AddMult, 3},
        
        {"Blue Joker", "+15 Chips", JokerEffectType::AddChips, 15},
        {"Green Joker", "+20 Chips", JokerEffectType::AddChips, 20},
        {"Red Joker", "+25 Chips", JokerEffectType::AddChips, 25},
        
        {"Cavendish", "X3 Mult", JokerEffectType::MulMult, 3},
        {"Gros Michel", "X2 Mult", JokerEffectType::MulMult, 2}
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, pool.size() - 1);

    return pool[dis(gen)];
}

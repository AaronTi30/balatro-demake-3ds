#include "Joker.h"
#include <vector>
#include <random>

Joker Joker::getRandom() {
    static const std::vector<Joker> pool = {
        // 1. Basic Joker (Always active)
        {"Joker", "+4 Mult", JokerEffectType::AddMult, 
            [](HandEvalContext& ctx) { 
                ctx.mult += 4; 
            }},
            
        // 2. Sly Joker (+50 Chips if played hand contains a Pair)
        {"Sly Joker", "+50 Chips if Pair", JokerEffectType::AddChips,
            [](HandEvalContext& ctx) {
                if (ctx.containsPair) {
                    ctx.chips += 50;
                }
            }},
            
        // 3. Wrathful Joker (+4 Mult for each Spade played)
        {"Wrathful Joker", "+4 Mult per Spade", JokerEffectType::AddMult,
            [](HandEvalContext& ctx) {
                for (const auto& card : ctx.scoringCards) {
                    if (card.suit == Suit::Spades) {
                        ctx.mult += 4;
                    }
                }
            }},
            
        // 4. Half Joker (+20 Mult if played hand contains 3 or fewer cards)
        {"Half Joker", "+20 Mult if <= 3 cards", JokerEffectType::AddMult,
            [](HandEvalContext& ctx) {
                if (ctx.playedCardCount <= 3) {
                    ctx.mult += 20;
                }
            }}
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, pool.size() - 1);

    return pool[dis(gen)];
}

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
                // HandEvaluator already determined if there is at least a pair
                // Actually, ctx.playedHand is the *best* hand, but the hand itself was evaluated.
                // For simplicity, we trigger if the evaluated best hand is Pair or better (which is everything except HighCard).
                if (static_cast<int>(ctx.playedHand) >= 1) { // 1 is Pair
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
                if (ctx.scoringCards.size() <= 3) {
                    ctx.mult += 20;
                }
            }}
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, pool.size() - 1);

    return pool[dis(gen)];
}

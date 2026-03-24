#include "Joker.h"

#include "HandEvaluator.h"

#include <random>
#include <vector>

namespace {

constexpr ShopPriceRange kWeakPriceRange{4, 6};
constexpr ShopPriceRange kMediumPriceRange{6, 8};
constexpr ShopPriceRange kStrongPriceRange{8, 10};

} // namespace

Joker Joker::plainJoker() {
    return {
        "Plain Joker",
        "+2 Mult",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            ctx.mult += 2;
        },
        JokerTier::Weak,
        kWeakPriceRange
    };
}

Joker Joker::greedyJoker() {
    return {
        "Greedy Joker",
        "+20 Chips if Pair",
        JokerEffectType::AddChips,
        [](HandEvalContext& ctx) {
            if (ctx.playedHand == HandType::Pair) {
                ctx.chips += 20;
            }
        },
        JokerTier::Weak,
        kWeakPriceRange
    };
}

Joker Joker::suitJoker() {
    return {
        "Suit Joker",
        "+2 Mult per scoring Spade",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            for (const Card& card : ctx.scoringCards) {
                if (card.suit == Suit::Spades) {
                    ctx.mult += 2;
                }
            }
        },
        JokerTier::Weak,
        kWeakPriceRange
    };
}

Joker Joker::focusedJoker() {
    return {
        "Focused Joker",
        "+8 Mult if <= 3 cards played",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            if (ctx.playedCardCount <= 3) {
                ctx.mult += 8;
            }
        },
        JokerTier::Medium,
        kMediumPriceRange
    };
}

Joker Joker::flushJoker() {
    return {
        "Flush Joker",
        "+40 Chips if played hand is Flush",
        JokerEffectType::AddChips,
        [](HandEvalContext& ctx) {
            if (ctx.playedHand == HandType::Flush) {
                ctx.chips += 40;
            }
        },
        JokerTier::Medium,
        kMediumPriceRange
    };
}

Joker Joker::straightJoker() {
    return {
        "Straight Joker",
        "+40 Chips if played hand is Straight",
        JokerEffectType::AddChips,
        [](HandEvalContext& ctx) {
            if (ctx.playedHand == HandType::Straight) {
                ctx.chips += 40;
            }
        },
        JokerTier::Medium,
        kMediumPriceRange
    };
}

Joker Joker::heavyJoker() {
    return {
        "Heavy Joker",
        "+5 Mult",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            ctx.mult += 5;
        },
        JokerTier::Strong,
        kStrongPriceRange
    };
}

Joker Joker::aggroJoker() {
    return {
        "Aggro Joker",
        "+60 Chips if Pair or better",
        JokerEffectType::AddChips,
        [](HandEvalContext& ctx) {
            if (ctx.playedHand != HandType::HighCard) {
                ctx.chips += 60;
            }
        },
        JokerTier::Strong,
        kStrongPriceRange
    };
}

Joker Joker::precisionJoker() {
    return {
        "Precision Joker",
        "+12 Mult if exactly 1 card scores",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            if (ctx.scoringCardCount == 1) {
                ctx.mult += 12;
            }
        },
        JokerTier::Strong,
        kStrongPriceRange
    };
}

const std::vector<Joker>& Joker::weakPool() {
    static const std::vector<Joker> pool = {
        plainJoker(),
        greedyJoker(),
        suitJoker()
    };
    return pool;
}

const std::vector<Joker>& Joker::mediumPool() {
    static const std::vector<Joker> pool = {
        focusedJoker(),
        flushJoker(),
        straightJoker()
    };
    return pool;
}

const std::vector<Joker>& Joker::strongPool() {
    static const std::vector<Joker> pool = {
        heavyJoker(),
        aggroJoker(),
        precisionJoker()
    };
    return pool;
}

Joker Joker::drawFromPool(const std::vector<Joker>& pool, std::mt19937& rng) {
    std::uniform_int_distribution<size_t> distribution(0, pool.size() - 1);
    return pool[distribution(rng)];
}

Joker Joker::drawWeakOrMedium(std::mt19937& rng) {
    static const std::vector<Joker> combinedPool = [] {
        std::vector<Joker> combined;
        combined.reserve(weakPool().size() + mediumPool().size());
        combined.insert(combined.end(), weakPool().begin(), weakPool().end());
        combined.insert(combined.end(), mediumPool().begin(), mediumPool().end());
        return combined;
    }();

    return drawFromPool(combinedPool, rng);
}

Joker Joker::drawWeightedFullPool(std::mt19937& rng) {
    std::uniform_int_distribution<int> distribution(1, 100);
    const int roll = distribution(rng);

    if (roll <= 50) {
        return drawFromPool(weakPool(), rng);
    }
    if (roll <= 85) {
        return drawFromPool(mediumPool(), rng);
    }
    return drawFromPool(strongPool(), rng);
}

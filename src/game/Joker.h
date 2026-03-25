#pragma once

#include "Card.h"
#include <functional>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

enum class HandType;

struct HandEvalContext {
    HandType playedHand;
    const std::vector<Card>& playedCards;
    int playedCardCount;
    const std::vector<Card>& scoringCards;
    int scoringCardCount;
    bool containsPair;
    int& chips;
    int& mult;
};

enum class JokerEffectType {
    AddChips,
    AddMult,
    MulMult
};

enum class JokerTier {
    Weak,
    Medium,
    Strong
};

struct ShopPriceRange {
    int min;
    int max;
};

struct Joker {
    std::string name;
    std::string description;
    JokerEffectType effectType; // For UI rendering colors
    std::function<void(HandEvalContext&)> evaluate;
    JokerTier tier = JokerTier::Weak;
    ShopPriceRange shopPriceRange{4, 6};
    int sellValue = 2;

    static Joker plainJoker();
    static Joker greedyJoker();
    static Joker suitJoker();
    static Joker focusedJoker();
    static Joker flushJoker();
    static Joker straightJoker();
    static Joker heavyJoker();
    static Joker aggroJoker();
    static Joker precisionJoker();

    static const std::vector<Joker>& weakPool();
    static const std::vector<Joker>& mediumPool();
    static const std::vector<Joker>& strongPool();

    static Joker drawFromPool(const std::vector<Joker>& pool, std::mt19937& rng);
    static JokerTier tierForWeightedRoll(int roll);
    static Joker drawWeakOrMedium(std::mt19937& rng);
    static Joker drawWeightedFullPool(std::mt19937& rng);

    static std::string idFor(const Joker& joker);
    static std::vector<Joker> weakPoolFiltered(const std::unordered_set<std::string>& excludedIds);
    static std::vector<Joker> mediumPoolFiltered(const std::unordered_set<std::string>& excludedIds);
    static std::vector<Joker> strongPoolFiltered(const std::unordered_set<std::string>& excludedIds);
    static std::vector<Joker> weakOrMediumPoolFiltered(const std::unordered_set<std::string>& excludedIds);
    static Joker drawFromCandidates(const std::vector<Joker>& candidates, std::mt19937& rng);
};

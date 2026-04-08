#include "HandEvaluator.h"
#include <map>
#include <set>

namespace {

int rankSortValue(Rank rank) {
    return rank == Rank::Ace ? 14 : static_cast<int>(rank);
}

bool isFaceRank(Rank rank) {
    return rank == Rank::Jack || rank == Rank::Queen || rank == Rank::King || rank == Rank::Ace;
}

} // namespace

// ── Balatro base values (chips, mult) by hand type ──
std::pair<int, int> HandEvaluator::lookupBaseValues(HandType t, int level) {
    std::pair<int, int> base;
    switch (t) {
        case HandType::HighCard:       base = {  5, 1 }; break;
        case HandType::Pair:           base = { 10, 2 }; break;
        case HandType::TwoPair:        base = { 20, 2 }; break;
        case HandType::ThreeOfAKind:   base = { 30, 3 }; break;
        case HandType::Straight:       base = { 30, 4 }; break;
        case HandType::Flush:          base = { 35, 4 }; break;
        case HandType::FullHouse:      base = { 40, 4 }; break;
        case HandType::FourOfAKind:    base = { 60, 7 }; break;
        case HandType::StraightFlush:  base = {100, 8 }; break;
        case HandType::RoyalFlush:     base = {100, 8 }; break;
        default:                       base = {  5, 1 }; break;
    }

    const int extraLevels = std::max(0, level - 1);
    base.first += extraLevels * 10;
    base.second += extraLevels;
    return base;
}

int HandEvaluator::calculateScoringCardChipBonus(const std::vector<Card>& scoringCards) {
    int chipBonus = 0;
    for (const auto& card : scoringCards) {
        chipBonus += rankChipValue(card.rank);
    }

    return chipBonus;
}

int HandEvaluator::calculateBossChipPenalty(const std::vector<Card>& scoringCards,
                                            BossBlindModifier bossModifier,
                                            Suit blockedSuit) {
    int penalty = 0;
    for (const auto& card : scoringCards) {
        if (bossModifier == BossBlindModifier::SuitLock && card.suit == blockedSuit) {
            penalty += rankChipValue(card.rank);
            continue;
        }

        if (bossModifier == BossBlindModifier::FaceTax && isFaceRank(card.rank)) {
            penalty += rankChipValue(card.rank) / 2;
        }
    }

    return penalty;
}

std::vector<std::pair<Rank, int>> HandEvaluator::rankCounts(const std::vector<Card>& cards) {
    std::map<Rank, int> counts;
    for (const auto& c : cards) {
        counts[c.rank]++;
    }
    
    // Convert to vector and sort by count descending, then rank descending
    std::vector<std::pair<Rank, int>> result(counts.begin(), counts.end());
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return rankSortValue(a.first) > rankSortValue(b.first);
    });
    return result;
}

bool HandEvaluator::isStraight(std::vector<Card> cards) {
    if (cards.size() < 5) return false;
    
    // Sort by rank
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return static_cast<int>(a.rank) < static_cast<int>(b.rank);
    });
    
    // Check consecutive
    for (size_t i = 1; i < cards.size(); ++i) {
        if (static_cast<int>(cards[i].rank) != static_cast<int>(cards[i-1].rank) + 1) {
            // Special case: A-2-3-4-5 (ace low straight)
            // Check if it's 10-J-Q-K-A
            if (i == cards.size() - 1 && cards[i].rank == Rank::Ace && 
                cards[0].rank == Rank::Two) {
                // Recheck: is it 2,3,4,5,A ?
                bool acelow = true;
                for (size_t j = 1; j < cards.size() - 1; ++j) {
                    if (static_cast<int>(cards[j].rank) != static_cast<int>(cards[j-1].rank) + 1) {
                        acelow = false;
                        break;
                    }
                }
                return acelow;
            }
            return false;
        }
    }
    return true;
}

bool HandEvaluator::isFlush(const std::vector<Card>& cards) {
    if (cards.size() < 5) return false;
    Suit s = cards[0].suit;
    for (const auto& c : cards) {
        if (c.suit != s) return false;
    }
    return true;
}

HandType HandEvaluator::classifyHand(const std::vector<Card>& cards,
                                     const std::vector<std::pair<Rank, int>>& counts,
                                     bool flush,
                                     bool straight) {
    if (flush && straight) {
        std::set<Rank> ranks;
        for (const auto& c : cards) ranks.insert(c.rank);
        if (ranks.count(Rank::Ten) && ranks.count(Rank::Jack) && 
            ranks.count(Rank::Queen) && ranks.count(Rank::King) && 
            ranks.count(Rank::Ace)) {
            return HandType::RoyalFlush;
        }
        return HandType::StraightFlush;
    }
    if (counts.size() >= 1 && counts[0].second == 4) {
        return HandType::FourOfAKind;
    }
    if (counts.size() >= 2 && counts[0].second == 3 && counts[1].second == 2) {
        return HandType::FullHouse;
    }
    if (flush) {
        return HandType::Flush;
    }
    if (straight) {
        return HandType::Straight;
    }
    if (counts.size() >= 1 && counts[0].second == 3) {
        return HandType::ThreeOfAKind;
    }
    if (counts.size() >= 2 && counts[0].second == 2 && counts[1].second == 2) {
        return HandType::TwoPair;
    }
    if (counts.size() >= 1 && counts[0].second == 2) {
        return HandType::Pair;
    }
    return HandType::HighCard;
}

std::vector<Card> HandEvaluator::selectScoringCards(const std::vector<Card>& cards,
                                                    const std::vector<std::pair<Rank, int>>& counts,
                                                    HandType type) {
    std::vector<Card> scoringCards;

    switch (type) {
        case HandType::RoyalFlush:
        case HandType::StraightFlush:
        case HandType::FullHouse:
        case HandType::Flush:
        case HandType::Straight:
            scoringCards = cards;
            break;
        case HandType::FourOfAKind: {
            Rank fourRank = counts[0].first;
            for (const auto& c : cards) {
                if (c.rank == fourRank) scoringCards.push_back(c);
            }
            break;
        }
        case HandType::ThreeOfAKind: {
            Rank tripleRank = counts[0].first;
            for (const auto& c : cards) {
                if (c.rank == tripleRank) scoringCards.push_back(c);
            }
            break;
        }
        case HandType::TwoPair: {
            Rank pair1 = counts[0].first;
            Rank pair2 = counts[1].first;
            for (const auto& c : cards) {
                if (c.rank == pair1 || c.rank == pair2) scoringCards.push_back(c);
            }
            break;
        }
        case HandType::Pair: {
            Rank pairRank = counts[0].first;
            for (const auto& c : cards) {
                if (c.rank == pairRank) scoringCards.push_back(c);
            }
            break;
        }
        case HandType::HighCard: {
            auto best = std::max_element(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
                return rankSortValue(a.rank) < rankSortValue(b.rank);
            });
            if (best != cards.end()) {
                scoringCards.push_back(*best);
            }
            break;
        }
    }

    return scoringCards;
}

HandEvaluator::ScoreTotals HandEvaluator::calculateFinalTotals(HandType type,
                                                               const std::vector<Card>& playedCards,
                                                               const std::vector<Card>& scoringCards,
                                                               bool containsPair,
                                                               int chipsAfterScoringCards,
                                                               int baseMult,
                                                               const std::vector<Joker>& jokers,
                                                               int bossChipPenalty,
                                                               BossBlindModifier bossModifier) {
    int finalChips = chipsAfterScoringCards;
    int finalMult = baseMult;
    HandEvalContext ctx{
        type,
        playedCards,
        static_cast<int>(playedCards.size()),
        scoringCards,
        static_cast<int>(scoringCards.size()),
        containsPair,
        finalChips,
        finalMult
    };
    for (const auto& joker : jokers) {
        if (joker.evaluate) {
            joker.evaluate(ctx);
        }
    }

    if (bossModifier == BossBlindModifier::SuitLock || bossModifier == BossBlindModifier::FaceTax) {
        finalChips -= bossChipPenalty;
    }

    int finalScore = finalChips * finalMult;
    bool scoreEquationExact = true;
    switch (bossModifier) {
        case BossBlindModifier::PairTax:
            if (type == HandType::Pair || type == HandType::TwoPair) {
                finalScore = finalScore * 75 / 100;
                scoreEquationExact = false;
            }
            break;
        case BossBlindModifier::SmallHandPunish:
            if (playedCards.size() <= 3) {
                finalScore = finalScore * 70 / 100;
                scoreEquationExact = false;
            }
            break;
        case BossBlindModifier::HighCardWall:
            if (type == HandType::HighCard || type == HandType::Pair) {
                finalScore = finalScore * 70 / 100;
                scoreEquationExact = false;
            }
            break;
        case BossBlindModifier::FaceTax:
        case BossBlindModifier::SuitLock:
        case BossBlindModifier::None:
        default:
            break;
    }

    return { finalChips, finalMult, finalScore, scoreEquationExact };
}

HandResult HandEvaluator::evaluate(std::vector<Card> cards,
                                   const std::vector<Joker>& jokers,
                                   BossBlindModifier bossModifier,
                                   Suit blockedSuit,
                                   const RunState* runState) {
    auto counts = rankCounts(cards);
    bool flush = isFlush(cards);
    bool straight = isStraight(cards);
    const bool containsPair = std::any_of(
        counts.begin(),
        counts.end(),
        [](const auto& count) { return count.second >= 2; });

    HandType detectedHand = cards.empty()
        ? HandType::HighCard
        : classifyHand(cards, counts, flush, straight);
    std::vector<Card> scoringCards = selectScoringCards(cards, counts, detectedHand);

    const int handLevel = runState ? runState->handLevel(detectedHand) : 1;
    std::pair<int, int> handBaseValues = lookupBaseValues(detectedHand, handLevel);
    const int rawScoringCardChipBonus = calculateScoringCardChipBonus(scoringCards);
    const int bossChipPenalty = calculateBossChipPenalty(scoringCards, bossModifier, blockedSuit);
    const int scoringCardChipBonus = rawScoringCardChipBonus - bossChipPenalty;
    ScoreTotals finalTotals = calculateFinalTotals(
        detectedHand,
        cards,
        scoringCards,
        containsPair,
        handBaseValues.first + rawScoringCardChipBonus,
        handBaseValues.second,
        jokers,
        bossChipPenalty,
        bossModifier);

    HandResult result{};
    result.detectedHand = detectedHand;
    result.containsPair = containsPair;
    result.scoringCards = scoringCards;
    result.baseHandChips = handBaseValues.first;
    result.baseHandMult = handBaseValues.second;
    result.scoringCardChipBonus = scoringCardChipBonus;
    result.finalChips = finalTotals.chips;
    result.finalMult = finalTotals.mult;
    result.finalScore = finalTotals.score;
    result.scoreEquationExact = finalTotals.scoreEquationExact;

    return result;
}

#include "game/HandEvaluator.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expectHand(HandType actual, HandType expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << handTypeName(expected)
            << ", got " << handTypeName(actual);
        fail(oss.str());
    }
}

Card makeCard(Suit suit, Rank rank) {
    return { suit, rank };
}

int rankSortValue(Rank rank) {
    return rank == Rank::Ace ? 14 : static_cast<int>(rank);
}

void expectScoringRanks(const std::vector<Card>& cards,
                        const std::vector<Rank>& expectedRanks,
                        const std::string& label) {
    expectEqual(static_cast<int>(cards.size()),
                static_cast<int>(expectedRanks.size()),
                label + " size");

    for (size_t i = 0; i < expectedRanks.size(); ++i) {
        if (cards[i].rank != expectedRanks[i]) {
            std::ostringstream oss;
            oss << label << " rank[" << i << "]: expected "
                << rankToString(expectedRanks[i]) << ", got "
                << rankToString(cards[i].rank);
            fail(oss.str());
        }
    }
}

void expectScoringRanksUnordered(std::vector<Card> cards,
                                 std::vector<Rank> expectedRanks,
                                 const std::string& label) {
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return rankSortValue(a.rank) > rankSortValue(b.rank);
    });
    std::sort(expectedRanks.begin(), expectedRanks.end(), [](Rank a, Rank b) {
        return rankSortValue(a) > rankSortValue(b);
    });

    expectEqual(static_cast<int>(cards.size()),
                static_cast<int>(expectedRanks.size()),
                label + " size");

    for (size_t i = 0; i < expectedRanks.size(); ++i) {
        if (cards[i].rank != expectedRanks[i]) {
            std::ostringstream oss;
            oss << label << " rank[" << i << "]: expected "
                << rankToString(expectedRanks[i]) << ", got "
                << rankToString(cards[i].rank);
            fail(oss.str());
        }
    }
}

void testHighCardUsesOnlyHighestCardForScoring() {
    HandResult result = HandEvaluator::evaluate({
        makeCard(Suit::Clubs, Rank::Three),
        makeCard(Suit::Diamonds, Rank::Ace),
        makeCard(Suit::Hearts, Rank::Seven)
    });

    expectHand(result.detectedHand, HandType::HighCard, "high card classification");
    expectScoringRanks(result.scoringCards, { Rank::Ace }, "high card scoring cards");
    expectEqual(result.baseHandChips, 5, "high card base chips");
    expectEqual(result.baseHandMult, 1, "high card base mult");
    expectEqual(result.scoringCardChipBonus, 11, "high card chip bonus");
    expectEqual(result.finalChips, 16, "high card final chips");
    expectEqual(result.finalMult, 1, "high card final mult");
    expectEqual(result.finalScore, 16, "high card final score");
}

void testPairUsesPairCardsOnlyForScoring() {
    HandResult result = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Four),
        makeCard(Suit::Spades, Rank::Four),
        makeCard(Suit::Diamonds, Rank::Nine)
    });

    expectHand(result.detectedHand, HandType::Pair, "pair classification");
    expectScoringRanks(result.scoringCards, { Rank::Four, Rank::Four }, "pair scoring cards");
    expectEqual(result.baseHandChips, 10, "pair base chips");
    expectEqual(result.baseHandMult, 2, "pair base mult");
    expectEqual(result.scoringCardChipBonus, 8, "pair chip bonus");
    expectEqual(result.finalChips, 18, "pair final chips");
    expectEqual(result.finalMult, 2, "pair final mult");
    expectEqual(result.finalScore, 36, "pair final score");
}

void testTwoPairSelectsBothPairsOnly() {
    HandResult result = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Three),
        makeCard(Suit::Spades, Rank::Three),
        makeCard(Suit::Diamonds, Rank::Nine),
        makeCard(Suit::Clubs, Rank::Nine),
        makeCard(Suit::Hearts, Rank::King)
    });

    expectHand(result.detectedHand, HandType::TwoPair, "two pair classification");
    expectScoringRanksUnordered(result.scoringCards,
                                { Rank::Nine, Rank::Nine, Rank::Three, Rank::Three },
                                "two pair scoring cards");
}

void testThreeOfAKindSelectsOnlyTripletCards() {
    HandResult result = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Ace),
        makeCard(Suit::Spades, Rank::Ace),
        makeCard(Suit::Diamonds, Rank::Ace),
        makeCard(Suit::Clubs, Rank::Nine),
        makeCard(Suit::Hearts, Rank::Four)
    });

    expectHand(result.detectedHand, HandType::ThreeOfAKind, "three of a kind classification");
    expectScoringRanks(result.scoringCards, { Rank::Ace, Rank::Ace, Rank::Ace }, "three of a kind scoring cards");
}

void testStraightFlushAndFullHouseClassification() {
    HandResult straightResult = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Two),
        makeCard(Suit::Hearts, Rank::Three),
        makeCard(Suit::Hearts, Rank::Four),
        makeCard(Suit::Hearts, Rank::Five),
        makeCard(Suit::Hearts, Rank::Six)
    });
    expectHand(straightResult.detectedHand, HandType::StraightFlush, "straight flush classification");
    expectEqual(static_cast<int>(straightResult.scoringCards.size()), 5, "straight flush scoring size");

    HandResult fullHouseResult = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Ten),
        makeCard(Suit::Clubs, Rank::Ten),
        makeCard(Suit::Spades, Rank::Ten),
        makeCard(Suit::Diamonds, Rank::Two),
        makeCard(Suit::Clubs, Rank::Two)
    });
    expectHand(fullHouseResult.detectedHand, HandType::FullHouse, "full house classification");
    expectEqual(static_cast<int>(fullHouseResult.scoringCards.size()), 5, "full house scoring size");
}

void testJokerContextExposesPlayedAndScoringCounts() {
    Joker inspector{
        "Inspector",
        "Verifies evaluation context",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            if (ctx.containsPair &&
                ctx.playedCardCount == 3 &&
                ctx.scoringCardCount == 2 &&
                ctx.playedCards.size() == 3 &&
                ctx.scoringCards.size() == 2) {
                ctx.mult += 10;
            }
        }
    };

    HandResult result = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Four),
        makeCard(Suit::Spades, Rank::Four),
        makeCard(Suit::Diamonds, Rank::King)
    }, { inspector });

    expectEqual(result.finalMult, 12, "context-aware joker should update mult");
    expectEqual(result.finalScore, 216, "context-aware joker should affect score");
}

void testSlyJokerRequiresAnActualPair() {
    Joker slyJoker{
        "Sly Joker",
        "+50 Chips if Pair",
        JokerEffectType::AddChips,
        [](HandEvalContext& ctx) {
            if (ctx.containsPair) {
                ctx.chips += 50;
            }
        }
    };

    HandResult uniqueRanks = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Two),
        makeCard(Suit::Clubs, Rank::Three),
        makeCard(Suit::Spades, Rank::Four),
        makeCard(Suit::Diamonds, Rank::Five),
        makeCard(Suit::Hearts, Rank::Six)
    }, { slyJoker });
    expectEqual(uniqueRanks.finalChips, 50, "sly joker should not trigger on unique ranks");

    HandResult pairHand = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Jack),
        makeCard(Suit::Spades, Rank::Jack),
        makeCard(Suit::Diamonds, Rank::Four)
    }, { slyJoker });
    expectEqual(pairHand.finalChips, 80, "sly joker should trigger when a pair is present");
}

void testHalfJokerUsesPlayedCardCount() {
    Joker halfJoker{
        "Half Joker",
        "+20 Mult if <= 3 cards",
        JokerEffectType::AddMult,
        [](HandEvalContext& ctx) {
            if (ctx.playedCardCount <= 3) {
                ctx.mult += 20;
            }
        }
    };

    HandResult threeCardHand = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Four),
        makeCard(Suit::Spades, Rank::Four),
        makeCard(Suit::Diamonds, Rank::King)
    }, { halfJoker });
    expectEqual(threeCardHand.finalMult, 22, "half joker should trigger on three played cards");

    HandResult fiveCardPair = HandEvaluator::evaluate({
        makeCard(Suit::Hearts, Rank::Four),
        makeCard(Suit::Spades, Rank::Four),
        makeCard(Suit::Diamonds, Rank::King),
        makeCard(Suit::Clubs, Rank::Nine),
        makeCard(Suit::Hearts, Rank::Two)
    }, { halfJoker });
    expectEqual(fiveCardPair.finalMult, 2, "half joker should not trigger on five played cards");
}

} // namespace

int main() {
    try {
        testHighCardUsesOnlyHighestCardForScoring();
        testPairUsesPairCardsOnlyForScoring();
        testTwoPairSelectsBothPairsOnly();
        testThreeOfAKindSelectsOnlyTripletCards();
        testStraightFlushAndFullHouseClassification();
        testJokerContextExposesPlayedAndScoringCounts();
        testSlyJokerRequiresAnActualPair();
        testHalfJokerUsesPlayedCardCount();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "HandEvaluator tests passed\n";
    return 0;
}

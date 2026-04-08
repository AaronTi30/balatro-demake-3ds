#include "game/ScoringAnimator.h"
#include "game/Card.h"
#include "game/HandEvaluator.h"
#include "game/Joker.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void fail(const std::string& msg) { std::cerr << msg << '\n'; std::exit(1); }
void expect(bool c, const std::string& lbl) { if (!c) fail(lbl); }
void expectEqual(int actual, int expected, const std::string& lbl) {
    if (actual != expected) {
        std::ostringstream s;
        s << lbl << ": expected " << expected << ", got " << actual;
        fail(s.str());
    }
}

void expectRenderCard(const ScoringAnimator::RenderCardState& state,
                      int expectedX,
                      int expectedY,
                      bool expectedHighlight,
                      const std::string& lbl) {
    expectEqual(state.drawX, expectedX, lbl + " x");
    expectEqual(state.drawY, expectedY, lbl + " y");
    expect(state.highlight == expectedHighlight, lbl + " highlight");
}

// Build a minimal HandResult for two Hearts scoring cards (A♥ and K♥)
// Used as a Pair: baseHandChips=10, baseHandMult=2
// Per-card chips: A=11, K=10 => scoringCardChipBonus=21
// finalChips=31, finalMult=2, finalScore=62
HandResult makePairResult() {
    HandResult r{};
    r.detectedHand = HandType::Pair;
    r.containsPair = true;
    r.baseHandChips = 10;
    r.baseHandMult = 2;
    r.scoringCards = {
        Card{Suit::Hearts, Rank::Ace},
        Card{Suit::Hearts, Rank::King}
    };
    r.scoringCardChipBonus = 21;
    r.finalChips = 31;
    r.finalMult = 2;
    r.finalScore = 62;
    r.scoreEquationExact = true;
    return r;
}

// Two played cards at y=158 (170 - selectOffset=12), centered on x=200
std::vector<std::pair<int,int>> makeStartPositions() {
    return { {170, 158}, {206, 158} };
}

} // namespace

int main() {
    // Test: mismatched start positions fail safely
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };

        bool threw = false;
        try {
            ScoringAnimator anim(cards, { {170, 158} }, {}, result, 0, 200, 100);
            (void)anim;
        } catch (const std::invalid_argument&) {
            threw = true;
        }

        expect(threw, "constructor should reject mismatched startPositions");

        std::cout << "PASS: mismatched start positions rejected\n";
    }

    // Test: initial state
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, /*currentRoundScore=*/0, 200, 100);

        expect(!anim.isDone(), "isDone() should be false initially");
        expectEqual(anim.displayChips(), 10, "displayChips() should equal baseHandChips initially");
        expectEqual(anim.displayMult(), 2, "displayMult() should equal baseHandMult initially");
        expectEqual(anim.displayRoundScore(), 0, "displayRoundScore() should equal currentRoundScore initially");
        expectEqual(anim.activeJokerIndex(), -1, "activeJokerIndex() should be -1 initially");
        expect(anim.handType() == HandType::Pair, "handType() should match result");

        std::cout << "PASS: initial state\n";
    }

    // Test: FlyToStage render state interpolates card positions
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, 0, 200, 100);

        anim.update(0.2f);

        auto renderCards = anim.cardRenderStates();
        expectEqual(static_cast<int>(renderCards.size()), 2, "FlyToStage render card count");
        expectRenderCard(renderCards[0], 166, 129, false, "FlyToStage card 0");
        expectRenderCard(renderCards[1], 202, 129, false, "FlyToStage card 1");

        std::cout << "PASS: FlyToStage render state interpolates positions\n";
    }

    // Test: after FlyToStage, stage transitions to CardScoring
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, 0, 200, 100);

        anim.update(0.41f);

        expect(!anim.isDone(), "isDone() should be false after FlyToStage");
        expectEqual(anim.displayChips(), 21, "displayChips() after FlyToStage = baseHandChips(10) + Ace(11)");
        expectEqual(anim.displayMult(), 2, "displayMult() unchanged after FlyToStage");
        expectEqual(anim.activeJokerIndex(), -1, "activeJokerIndex() should be -1 during CardScoring");

        std::cout << "PASS: after FlyToStage\n";
    }

    // Test: large dt carries through multiple stage windows without dropping leftover time
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, 0, 200, 100);

        anim.update(0.85f);

        expect(!anim.isDone(), "large dt should not finish the full sequence");
        expectEqual(anim.displayChips(), 31, "large dt should preserve carry-over into the second CardScoring window");
        expectEqual(anim.displayMult(), 2, "large dt preserves mult");
        expectEqual(anim.displayRoundScore(), 0, "large dt should not start tally before CardScoring ends");

        auto renderCards = anim.cardRenderStates();
        expectEqual(static_cast<int>(renderCards.size()), 2, "large dt render card count");
        expectRenderCard(renderCards[0], 162, 100, false, "large dt card 0");
        expectRenderCard(renderCards[1], 198, 100, true, "large dt card 1");

        std::cout << "PASS: large dt carries through multiple stage windows\n";
    }

    // Test: CardScoring advances per-card chips correctly
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, 0, 200, 100);

        anim.update(0.41f);
        anim.update(0.31f);

        expectEqual(anim.displayChips(), 31, "displayChips() after both scoring cards = 10+11+10=31");
        expect(!anim.isDone(), "isDone() should be false after CardScoring (no jokers, still need ScoreTally)");

        std::cout << "PASS: CardScoring advances chips\n";
    }

    // Test: JokerTrigger — activeJokerIndex and chips updated per joker
    {
        HandResult result = makePairResult();
        Joker testJoker;
        testJoker.name = "TestChips";
        testJoker.effectType = JokerEffectType::AddChips;
        testJoker.evaluate = [](HandEvalContext& ctx) { ctx.chips += 10; };

        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, { testJoker }, result, 0, 200, 100);

        anim.update(0.41f);
        anim.update(0.31f);
        anim.update(0.31f);
        expectEqual(anim.activeJokerIndex(), 0, "activeJokerIndex() == 0 during first joker window");
        expectEqual(anim.displayChips(), 41, "displayChips() after joker = 31 + 10 = 41");

        anim.update(0.31f);
        expectEqual(anim.activeJokerIndex(), -1, "activeJokerIndex() == -1 after JokerTrigger");

        std::cout << "PASS: JokerTrigger activates joker and updates chips\n";
    }

    // Test: full sequence without jokers -> isDone() and displayRoundScore
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, /*currentRoundScore=*/100, 200, 100);

        anim.update(0.41f);
        anim.update(0.31f);
        anim.update(0.31f);
        anim.update(0.41f);

        expect(anim.isDone(), "isDone() should be true after full sequence");
        expectEqual(anim.displayRoundScore(), 162, "displayRoundScore() = 100 + 62 = 162");

        std::cout << "PASS: full sequence completes and displayRoundScore correct\n";
    }

    // Test: ScoreTally render state applies upward fly-off offset
    {
        HandResult result = makePairResult();
        std::vector<Card> cards = {
            Card{Suit::Hearts, Rank::Ace},
            Card{Suit::Hearts, Rank::King}
        };
        auto positions = makeStartPositions();

        ScoringAnimator anim(cards, positions, {}, result, 0, 200, 100);

        anim.update(1.1f);

        expect(!anim.isDone(), "ScoreTally should still be in progress");
        expectEqual(anim.displayRoundScore(), 15, "ScoreTally displayRoundScore should include carry-over time");

        auto renderCards = anim.cardRenderStates();
        expectEqual(static_cast<int>(renderCards.size()), 2, "ScoreTally render card count");
        expectRenderCard(renderCards[0], 162, 35, false, "ScoreTally card 0");
        expectRenderCard(renderCards[1], 198, 35, false, "ScoreTally card 1");

        std::cout << "PASS: ScoreTally render state applies upward fly-off offset\n";
    }

    std::cout << "All ScoringAnimator tests passed.\n";
    return 0;
}

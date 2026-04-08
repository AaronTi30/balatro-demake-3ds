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

// Build a minimal HandResult for two Hearts scoring cards (A♥ and K♥)
// Used as a Pair: baseHandChips=10, baseHandMult=2
// Per-card chips: A=11, K=10 => scoringCardChipBonus=21
// finalChips=31, finalMult=2, finalScore=62
HandResult makePairResult() {
    HandResult r{};
    r.detectedHand = HandType::Pair;
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

    std::cout << "All ScoringAnimator tests passed.\n";
    return 0;
}

#include "game/ScoringAnimator.h"
#include "game/Card.h"
#include "game/HandEvaluator.h"
#include "game/Joker.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
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

    std::cout << "All ScoringAnimator tests passed.\n";
    return 0;
}

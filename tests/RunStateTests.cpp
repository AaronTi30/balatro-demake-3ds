#include "game/RunState.h"
#include "game/Joker.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expectEqual(const std::string& actual, const std::string& expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected \"" << expected << "\", got \"" << actual << "\"";
        fail(oss.str());
    }
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void testNewRunStartsAtSmallBlind() {
    RunState run;
    run.startNewRun();
    run.startRound();

    expectEqual(run.ante, 1, "new run ante");
    expectEqual(run.money, 4, "new run starts with four dollars");
    expect(run.blindStage == BlindStage::Small, "new run should begin at the small blind");
    expect(run.nextBossModifier != BossBlindModifier::None, "new run should preview a boss modifier");
    expectEqual(run.roundTarget, 300, "small blind ante 1 target");
    expectEqual(run.currentBlindReward(), 2, "small blind reward");
}

void testTargetsScaleByBlindStage() {
    expectEqual(RunState::kBlindTargets[0].small, 300, "ante 1 small blind table target");
    expectEqual(RunState::kBlindTargets[0].big, 450, "ante 1 big blind table target");
    expectEqual(RunState::kBlindTargets[2].boss, 4800, "ante 3 boss blind table target");
    expectEqual(RunState::kBlindRewards[0], 2, "small blind table reward");
    expectEqual(RunState::kBlindRewards[1], 3, "big blind table reward");
    expectEqual(RunState::kBlindRewards[2], 4, "boss blind table reward");
    expectEqual(RunState::targetForBlind(1, BlindStage::Big), 450, "ante 1 big blind target");
    expectEqual(RunState::targetForBlind(3, BlindStage::Boss), 4800, "ante 3 boss blind target");
    expectEqual(RunState::targetForBlind(8, BlindStage::Boss), 100000, "ante 8 boss blind target");
}

void testAdvancingBlindRequiresThreeClearsPerAnte() {
    RunState run;
    run.startNewRun();

    expect(run.blindStage == BlindStage::Small, "run should begin at small blind");
    run.advanceBlind();
    expect(run.blindStage == BlindStage::Big, "first clear should advance to big blind");
    expectEqual(run.ante, 1, "ante should not advance after small blind");

    run.advanceBlind();
    expect(run.blindStage == BlindStage::Boss, "second clear should advance to boss blind");
    expectEqual(run.ante, 1, "ante should not advance after big blind");

    run.advanceBlind();
    expect(run.blindStage == BlindStage::Small, "boss clear should roll to next ante small blind");
    expectEqual(run.ante, 2, "ante should advance only after boss blind");
}

void testBlindRewardIsFixedByBlindType() {
    RunState run;
    run.startNewRun();
    run.handsRemaining = 0;
    run.discardsRemaining = 0;

    run.awardRoundWin();
    expectEqual(run.money, 6, "small blind fixed payout");

    run.advanceBlind();
    run.awardRoundWin();
    expectEqual(run.money, 9, "big blind fixed payout");

    run.advanceBlind();
    run.awardRoundWin();
    expectEqual(run.money, 13, "boss blind fixed payout");
}

void testRunCompletesOnAnteEightBossClear() {
    RunState run;
    run.startNewRun();
    run.ante = RunState::kMaxAnte;
    run.blindStage = BlindStage::Boss;
    run.startRound();
    run.addRoundScore(run.roundTarget);

    expect(run.isRunComplete(), "ante 8 boss blind clear should complete the run");
}

void testNextBlindPreviewUsesNextShopStep() {
    RunState run;
    run.startNewRun();

    expect(run.shouldVisitShopAfterBlindWin(), "shop should follow a blind win");
    expect(run.nextBlindStage() == BlindStage::Big, "small blind should preview big blind next");
    expectEqual(run.nextBlindAnte(), 1, "small blind should stay in ante 1");

    run.blindStage = BlindStage::Big;
    expect(run.nextBlindStage() == BlindStage::Boss, "big blind should preview boss blind next");
    expectEqual(run.nextBlindAnte(), 1, "big blind should stay in same ante");

    run.blindStage = BlindStage::Boss;
    expect(run.nextBlindStage() == BlindStage::Small, "boss blind should preview next ante small blind");
    expectEqual(run.nextBlindAnte(), 2, "boss blind should preview next ante");
}

void testBossModifierRollCanUseSeededRng() {
    std::mt19937 leftRng(17);
    std::mt19937 rightRng(17);
    RunState left;
    RunState right;

    left.rollNextBossModifier(leftRng);
    right.rollNextBossModifier(rightRng);

    expect(left.nextBossModifier != BossBlindModifier::None, "rolled preview should never be none");
    expect(left.nextBossModifier == right.nextBossModifier, "seeded boss modifier rolls should match");
    expect(left.nextBlockedSuit == right.nextBlockedSuit, "seeded blocked suit rolls should match");
}

void testEnterCurrentBlindPromotesPreviewedBossModifier() {
    RunState run;
    run.startNewRun();
    run.blindStage = BlindStage::Boss;
    run.nextBossModifier = BossBlindModifier::SuitLock;
    run.nextBlockedSuit = Suit::Spades;

    run.enterCurrentBlind();

    expect(run.currentBossModifier == BossBlindModifier::SuitLock, "entering boss blind should promote stored modifier");
    expect(run.currentBlockedSuit == Suit::Spades, "entering boss blind should promote stored blocked suit");
}

void testAdvanceBlindActivatesStoredBossModifierAndRollsPreview() {
    RunState run;
    run.startNewRun();
    run.blindStage = BlindStage::Big;
    run.nextBossModifier = BossBlindModifier::FaceTax;
    run.nextBlockedSuit = Suit::Hearts;

    run.advanceBlind();

    expect(run.blindStage == BlindStage::Boss, "big blind should advance into boss blind");
    expect(run.currentBossModifier == BossBlindModifier::FaceTax, "boss blind should activate stored modifier");
    expect(run.currentBlockedSuit == Suit::Hearts, "boss blind should activate stored blocked suit");
    expect(run.nextBossModifier != BossBlindModifier::None, "boss blind should roll the next preview");
}

void testLeavingBossBlindClearsCurrentModifier() {
    RunState run;
    run.startNewRun();
    run.ante = 1;
    run.blindStage = BlindStage::Boss;
    run.currentBossModifier = BossBlindModifier::PairTax;
    run.currentBlockedSuit = Suit::Diamonds;

    run.advanceBlind();

    expectEqual(run.ante, 2, "clearing boss blind should advance the ante");
    expect(run.blindStage == BlindStage::Small, "clearing boss blind should return to small blind");
    expect(run.currentBossModifier == BossBlindModifier::None, "leaving boss blind should clear current modifier");
    expect(run.currentBlockedSuit == Suit::Clubs, "leaving boss blind should reset blocked suit");
}

void testBossModifierDescriptionsMatchImplementedSemantics() {
    expectEqual(std::string(RunState::bossModifierDescription(BossBlindModifier::SuitLock, Suit::Hearts)),
                "Subtracts Hearts rank chips after jokers",
                "suit lock description should match post-joker semantics");
    expectEqual(std::string(RunState::bossModifierDescription(BossBlindModifier::FaceTax, Suit::Clubs)),
                "Subtracts half of J, Q, K, A chips after jokers",
                "face tax description should match post-joker semantics");
}

void testRunScopedShopAvailability() {
    RunState runState;
    runState.startNewRun();

    // All catalog jokers are available after a fresh run
    for (const Joker& j : Joker::weakPool()) {
        expect(runState.isJokerShopAvailable(Joker::idFor(j)),
               "all weak jokers should start available");
    }
    for (const Joker& j : Joker::mediumPool()) {
        expect(runState.isJokerShopAvailable(Joker::idFor(j)),
               "all medium jokers should start available");
    }
    for (const Joker& j : Joker::strongPool()) {
        expect(runState.isJokerShopAvailable(Joker::idFor(j)),
               "all strong jokers should start available");
    }

    // Remove a joker from the pool
    const std::string plainId = Joker::idFor(Joker::plainJoker());
    runState.markJokerRemovedFromShopPool(plainId);
    expect(!runState.isJokerShopAvailable(plainId),
           "removed joker should not be available");

    // Re-add the joker to the pool
    runState.markJokerReturnedToShopPool(plainId);
    expect(runState.isJokerShopAvailable(plainId),
           "returned joker should be available again");

    // Calling startNewRun() again resets availability — joker removed mid-run comes back
    runState.markJokerRemovedFromShopPool(plainId);
    expect(!runState.isJokerShopAvailable(plainId), "joker should be gone before re-run");
    runState.startNewRun();
    expect(runState.isJokerShopAvailable(plainId), "startNewRun should restore all jokers to shop pool");

    // currentOwnedJokerIds reflects jokers vector
    Joker plain = Joker::plainJoker();
    runState.jokers.push_back(plain);
    const std::unordered_set<std::string> owned = runState.currentOwnedJokerIds();
    expect(owned.count(plainId) == 1,
           "owned joker ids should include plain joker");
}

void testAwardBlindSkipAddsMoney() {
    RunState run;
    run.startNewRun();
    const int before = run.money;
    run.awardBlindSkip();
    expectEqual(run.money, before + RunState::kBlindSkipReward,
                "awardBlindSkip should add kBlindSkipReward to money");
}

void testRerollCostResetsOnNewRun() {
    RunState run;
    run.startNewRun();
    expectEqual(run.rerollCost, 5, "rerollCost should start at 5 on new run");

    run.rerollCost = 9;
    run.startNewRun();
    expectEqual(run.rerollCost, 5, "rerollCost should reset to 5 on startNewRun");
}

} // namespace

int main() {
    try {
        testNewRunStartsAtSmallBlind();
        testTargetsScaleByBlindStage();
        testAdvancingBlindRequiresThreeClearsPerAnte();
        testBlindRewardIsFixedByBlindType();
        testRunCompletesOnAnteEightBossClear();
        testNextBlindPreviewUsesNextShopStep();
        testBossModifierRollCanUseSeededRng();
        testEnterCurrentBlindPromotesPreviewedBossModifier();
        testAdvanceBlindActivatesStoredBossModifierAndRollsPreview();
        testLeavingBossBlindClearsCurrentModifier();
        testBossModifierDescriptionsMatchImplementedSemantics();
        testRunScopedShopAvailability();
        testAwardBlindSkipAddsMoney();
        testRerollCostResetsOnNewRun();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "RunState tests passed\n";
    return 0;
}

#include "game/RunState.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

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

} // namespace

int main() {
    try {
        testNewRunStartsAtSmallBlind();
        testTargetsScaleByBlindStage();
        testAdvancingBlindRequiresThreeClearsPerAnte();
        testBlindRewardIsFixedByBlindType();
        testRunCompletesOnAnteEightBossClear();
        testNextBlindPreviewUsesNextShopStep();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "RunState tests passed\n";
    return 0;
}

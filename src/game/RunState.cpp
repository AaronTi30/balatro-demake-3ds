#include "RunState.h"

namespace {
constexpr int kStartingAnte = 1;
constexpr int kStartingMoney = 4;
constexpr int kStartingHands = 4;
constexpr int kStartingDiscards = 3;
constexpr int kStartingJokerLimit = 5;

int blindStageIndex(BlindStage stage) {
    switch (stage) {
        case BlindStage::Small:
            return 0;
        case BlindStage::Big:
            return 1;
        case BlindStage::Boss:
            return 2;
        default:
            return 0;
    }
}

int clampAnteIndex(int ante) {
    if (ante < 1) {
        return 0;
    }

    if (ante > RunState::kMaxAnte) {
        return RunState::kMaxAnte - 1;
    }

    return ante - 1;
}
} // namespace

int RunState::targetForAnte(int ante) {
    return RunState::kBlindTargets[clampAnteIndex(ante)].small;
}

int RunState::targetForBlind(int ante, BlindStage stage) {
    const BlindTargets& targets = RunState::kBlindTargets[clampAnteIndex(ante)];

    switch (stage) {
        case BlindStage::Small:
            return targets.small;
        case BlindStage::Big:
            return targets.big;
        case BlindStage::Boss:
            return targets.boss;
        default:
            return targets.small;
    }
}

const char* RunState::blindStageName(BlindStage stage) {
    switch (stage) {
        case BlindStage::Small:
            return "Small Blind";
        case BlindStage::Big:
            return "Big Blind";
        case BlindStage::Boss:
            return "Boss Blind";
        default:
            return "Blind";
    }
}

void RunState::startNewRun() {
    ante = kStartingAnte;
    blindStage = BlindStage::Small;
    money = kStartingMoney;
    jokerLimit = kStartingJokerLimit;
    jokers.clear();
}

void RunState::startRound() {
    handsRemaining = kStartingHands;
    discardsRemaining = kStartingDiscards;
    roundScore = 0;
    roundTarget = targetForBlind(ante, blindStage);
    deck.reset();
}

void RunState::addRoundScore(int points) {
    roundScore += points;
}

void RunState::awardRoundWin() {
    money += currentBlindReward();
}

void RunState::advanceBlind() {
    switch (blindStage) {
        case BlindStage::Small:
            blindStage = BlindStage::Big;
            return;
        case BlindStage::Big:
            blindStage = BlindStage::Boss;
            return;
        case BlindStage::Boss:
            if (ante < kMaxAnte) {
                ++ante;
                blindStage = BlindStage::Small;
            }
            return;
        default:
            return;
    }
}

bool RunState::isRoundWon() const {
    return roundScore >= roundTarget;
}

bool RunState::isRunComplete() const {
    return ante >= kMaxAnte && blindStage == BlindStage::Boss && isRoundWon();
}

bool RunState::isBossBlind() const {
    return blindStage == BlindStage::Boss;
}

bool RunState::shouldVisitShopAfterBlindWin() const {
    return !isRunComplete();
}

int RunState::currentBlindReward() const {
    return RunState::kBlindRewards[blindStageIndex(blindStage)];
}

const char* RunState::currentBlindName() const {
    return blindStageName(blindStage);
}

BlindStage RunState::nextBlindStage() const {
    switch (blindStage) {
        case BlindStage::Small:
            return BlindStage::Big;
        case BlindStage::Big:
            return BlindStage::Boss;
        case BlindStage::Boss:
        default:
            return BlindStage::Small;
    }
}

int RunState::nextBlindAnte() const {
    if (blindStage == BlindStage::Boss && ante < kMaxAnte) {
        return ante + 1;
    }

    return ante;
}

#include "RunState.h"

namespace {
constexpr int kStartingAnte = 1;
constexpr int kStartingMoney = 4;
constexpr int kStartingHands = 4;
constexpr int kStartingDiscards = 3;
constexpr int kStartingJokerLimit = 5;
constexpr int kSmallBlindReward = 3;
constexpr int kBigBlindReward = 4;
constexpr int kBossBlindReward = 5;
constexpr int kAnteTargets[] = { 300, 800, 2000, 5000, 11000, 20000, 35000, 50000 };
} // namespace

int RunState::targetForAnte(int ante) {
    if (ante < 1) {
        return kAnteTargets[0];
    }

    if (ante > kMaxAnte) {
        return kAnteTargets[kMaxAnte - 1];
    }

    return kAnteTargets[ante - 1];
}

int RunState::targetForBlind(int ante, BlindStage stage) {
    const int baseTarget = targetForAnte(ante);

    switch (stage) {
        case BlindStage::Small:
            return baseTarget;
        case BlindStage::Big:
            return (baseTarget * 3) / 2;
        case BlindStage::Boss:
            return baseTarget * 2;
        default:
            return baseTarget;
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
    switch (blindStage) {
        case BlindStage::Small:
            return kSmallBlindReward;
        case BlindStage::Big:
            return kBigBlindReward;
        case BlindStage::Boss:
            return kBossBlindReward;
        default:
            return kSmallBlindReward;
    }
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

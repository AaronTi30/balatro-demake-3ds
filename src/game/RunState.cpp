#include "RunState.h"

namespace {
constexpr int kStartingAnte = 1;
constexpr int kStartingMoney = 4;
constexpr int kStartingHands = 4;
constexpr int kStartingDiscards = 3;
constexpr int kStartingJokerLimit = 5;
constexpr int kMaxAnte = 8;
constexpr int kAnteTargets[] = { 300, 450, 600, 800, 1100, 1500, 2000, 2800 };

int targetForAnte(int ante) {
    if (ante < 1) {
        return kAnteTargets[0];
    }

    if (ante > kMaxAnte) {
        return kAnteTargets[kMaxAnte - 1];
    }

    return kAnteTargets[ante - 1];
}
} // namespace

void RunState::startNewRun() {
    ante = kStartingAnte;
    money = kStartingMoney;
    jokerLimit = kStartingJokerLimit;
    jokers.clear();
    startRound();
}

void RunState::startRound() {
    handsRemaining = kStartingHands;
    discardsRemaining = kStartingDiscards;
    roundScore = 0;
    roundTarget = targetForAnte(ante);
    deck.reset();
}

void RunState::addRoundScore(int points) {
    roundScore += points;
}

void RunState::awardRoundWin() {
    money += 3 + handsRemaining + discardsRemaining;
}

void RunState::advanceAnte() {
    ++ante;
    startRound();
}

bool RunState::isRoundWon() const {
    return roundScore >= roundTarget;
}

bool RunState::isRunComplete() const {
    return ante > kMaxAnte;
}

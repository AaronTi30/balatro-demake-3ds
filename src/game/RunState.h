#pragma once

#include "Deck.h"
#include "Joker.h"
#include <vector>

class RunState {
public:
    static constexpr int kMaxAnte = 8;

    int ante = 1;
    int roundTarget = 300;
    int roundScore = 0;
    int money = 4;
    int handsRemaining = 4;
    int discardsRemaining = 3;
    int jokerLimit = 5;
    std::vector<Joker> jokers;
    Deck deck;

    void startNewRun();
    void startRound();
    void addRoundScore(int points);
    void awardRoundWin();
    void advanceAnte();
    bool isRoundWon() const;
    bool isRunComplete() const;
    static int targetForAnte(int ante);
};

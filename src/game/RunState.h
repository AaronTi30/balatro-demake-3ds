#pragma once

#include "Deck.h"
#include "Joker.h"
#include <vector>

enum class BlindStage {
    Small,
    Big,
    Boss
};

class RunState {
public:
    static constexpr int kMaxAnte = 8;

    int ante = 1;
    BlindStage blindStage = BlindStage::Small;
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
    void advanceBlind();
    bool isRoundWon() const;
    bool isRunComplete() const;
    bool isBossBlind() const;
    bool shouldVisitShopAfterBlindWin() const;
    int currentBlindReward() const;
    const char* currentBlindName() const;
    BlindStage nextBlindStage() const;
    int nextBlindAnte() const;
    static int targetForAnte(int ante);
    static int targetForBlind(int ante, BlindStage stage);
    static const char* blindStageName(BlindStage stage);
};

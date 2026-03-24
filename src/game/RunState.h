#pragma once

#include "Deck.h"
#include "Joker.h"
#include <random>
#include <vector>

struct BlindTargets {
    int small;
    int big;
    int boss;
};

enum class BlindStage {
    Small,
    Big,
    Boss
};

enum class BossBlindModifier {
    None,
    PairTax,
    SmallHandPunish,
    SuitLock,
    FaceTax,
    HighCardWall
};

class RunState {
public:
    static constexpr int kMaxAnte = 8;
    inline static constexpr BlindTargets kBlindTargets[] = {
        { 300, 450, 600 },
        { 900, 1350, 1800 },
        { 2400, 3600, 4800 },
        { 5000, 7500, 10000 },
        { 11000, 16500, 22000 },
        { 20000, 30000, 40000 },
        { 35000, 52500, 70000 },
        { 50000, 75000, 100000 }
    };
    inline static constexpr int kBlindRewards[] = { 2, 3, 4 };

    int ante = 1;
    BlindStage blindStage = BlindStage::Small;
    int roundTarget = 300;
    int roundScore = 0;
    int money = 4;
    int handsRemaining = 4;
    int discardsRemaining = 3;
    int jokerLimit = 5;
    BossBlindModifier currentBossModifier = BossBlindModifier::None;
    BossBlindModifier nextBossModifier = BossBlindModifier::None;
    Suit currentBlockedSuit = Suit::Clubs;
    Suit nextBlockedSuit = Suit::Clubs;
    std::vector<Joker> jokers;
    Deck deck;

    void startNewRun();
    void startRound();
    void addRoundScore(int points);
    void awardRoundWin();
    void advanceBlind();
    void rollNextBossModifier(std::mt19937& rng);
    void enterCurrentBlind();
    bool isRoundWon() const;
    bool isRunComplete() const;
    bool isBossBlind() const;
    bool shouldVisitShopAfterBlindWin() const;
    int currentBlindReward() const;
    const char* currentBlindName() const;
    static const char* bossModifierName(BossBlindModifier modifier);
    static const char* bossModifierDescription(BossBlindModifier modifier, Suit blockedSuit);
    BlindStage nextBlindStage() const;
    int nextBlindAnte() const;
    static int targetForAnte(int ante);
    static int targetForBlind(int ante, BlindStage stage);
    static const char* blindStageName(BlindStage stage);
};

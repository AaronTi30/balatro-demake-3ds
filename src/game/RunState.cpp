#include "RunState.h"

#include <array>

namespace {
constexpr int kStartingAnte = 1;
constexpr int kStartingMoney = 4;
constexpr int kStartingHands = 4;
constexpr int kStartingDiscards = 3;
constexpr int kStartingJokerLimit = 5;
constexpr std::array kBossBlindModifiers{
    BossBlindModifier::PairTax,
    BossBlindModifier::SmallHandPunish,
    BossBlindModifier::SuitLock,
    BossBlindModifier::FaceTax,
    BossBlindModifier::HighCardWall
};

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

std::mt19937 makeBossModifierRng() {
    std::random_device device;
    return std::mt19937(device());
}

Suit suitFromIndex(int index) {
    switch (index) {
        case 0:
            return Suit::Hearts;
        case 1:
            return Suit::Diamonds;
        case 2:
            return Suit::Clubs;
        case 3:
            return Suit::Spades;
        default:
            return Suit::Clubs;
    }
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

const char* RunState::bossModifierName(BossBlindModifier modifier) {
    switch (modifier) {
        case BossBlindModifier::PairTax:
            return "Pair Tax";
        case BossBlindModifier::SmallHandPunish:
            return "Small Hand Punish";
        case BossBlindModifier::SuitLock:
            return "Suit Lock";
        case BossBlindModifier::FaceTax:
            return "Face Tax";
        case BossBlindModifier::HighCardWall:
            return "High Card Wall";
        case BossBlindModifier::None:
        default:
            return "None";
    }
}

const char* RunState::bossModifierDescription(BossBlindModifier modifier, Suit blockedSuit) {
    switch (modifier) {
        case BossBlindModifier::PairTax:
            return "Pair and Two Pair score 75%";
        case BossBlindModifier::SmallHandPunish:
            return "Hands of 3 cards or less score 70%";
        case BossBlindModifier::SuitLock:
            switch (blockedSuit) {
                case Suit::Hearts:
                    return "Subtracts Hearts rank chips after jokers";
                case Suit::Diamonds:
                    return "Subtracts Diamonds rank chips after jokers";
                case Suit::Clubs:
                    return "Subtracts Clubs rank chips after jokers";
                case Suit::Spades:
                    return "Subtracts Spades rank chips after jokers";
                default:
                    return "Subtracts one suit's rank chips after jokers";
            }
        case BossBlindModifier::FaceTax:
            return "Subtracts half of J, Q, K, A chips after jokers";
        case BossBlindModifier::HighCardWall:
            return "High Card and Pair score 70%";
        case BossBlindModifier::None:
        default:
            return "No modifier";
    }
}

void RunState::startNewRun() {
    ante = kStartingAnte;
    blindStage = BlindStage::Small;
    money = kStartingMoney;
    jokerLimit = kStartingJokerLimit;
    currentBossModifier = BossBlindModifier::None;
    nextBossModifier = BossBlindModifier::None;
    currentBlockedSuit = Suit::Clubs;
    nextBlockedSuit = Suit::Clubs;
    jokers.clear();

    std::mt19937 rng = makeBossModifierRng();
    rollNextBossModifier(rng);
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

void RunState::rollNextBossModifier(std::mt19937& rng) {
    std::uniform_int_distribution<size_t> modifierDist(0, kBossBlindModifiers.size() - 1);
    nextBossModifier = kBossBlindModifiers[modifierDist(rng)];

    if (nextBossModifier == BossBlindModifier::SuitLock) {
        std::uniform_int_distribution<int> suitDist(0, 3);
        nextBlockedSuit = suitFromIndex(suitDist(rng));
    } else {
        nextBlockedSuit = Suit::Clubs;
    }
}

void RunState::enterCurrentBlind() {
    if (blindStage == BlindStage::Boss) {
        currentBossModifier = nextBossModifier;
        currentBlockedSuit = nextBlockedSuit;
        return;
    }

    currentBossModifier = BossBlindModifier::None;
    currentBlockedSuit = Suit::Clubs;
}

void RunState::advanceBlind() {
    switch (blindStage) {
        case BlindStage::Small:
            blindStage = BlindStage::Big;
            enterCurrentBlind();
            return;
        case BlindStage::Big:
            blindStage = BlindStage::Boss;
            enterCurrentBlind();
            {
                std::mt19937 rng = makeBossModifierRng();
                rollNextBossModifier(rng);
            }
            return;
        case BlindStage::Boss:
            if (ante < kMaxAnte) {
                ++ante;
                blindStage = BlindStage::Small;
                enterCurrentBlind();
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

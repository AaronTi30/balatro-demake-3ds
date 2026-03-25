#include "game/ShopOffer.h"
#include "game/RunState.h"

#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) fail(label);
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

void testMixedOfferGenerationUsesFixedSlotKinds() {
    RunState run;
    run.startNewRun();

    std::mt19937 rng(77);
    const auto slots = generateShopOffers(rng, run);

    expect(slots[0].offer.kind == ShopOfferKind::Joker, "slot 0 should be a joker offer");
    expect(slots[1].offer.kind == ShopOfferKind::DeckCard, "slot 1 should be a deck-card offer");
    expect(slots[2].offer.kind == ShopOfferKind::HandUpgrade, "slot 2 should be a hand-upgrade offer");
    expectEqual(slots[1].offer.price, 4, "deck-card offer should use the fixed first-pass price");
    expectEqual(slots[2].offer.price, 6, "hand-upgrade offer should use the fixed first-pass price");
}

void testBuyingDeckCardOfferAddsOneCardToRunDeck() {
    RunState run;
    run.startNewRun();
    run.money = 20;

    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::DeckCard;
    slot.offer.card = Card{Suit::Spades, Rank::Ace};
    slot.offer.price = 4;

    const int beforeMoney = run.money;
    const int beforeDeck = run.runDeckSize();
    expect(applyShopOfferPurchase(run, slot), "deck-card offer should be purchasable");
    expectEqual(run.money, beforeMoney - 4, "deck-card offer should spend money");
    expectEqual(run.runDeckSize(), beforeDeck + 1, "deck-card offer should add one card to the run deck");
}

void testBuyingHandUpgradeOfferLevelsOnlyThatHand() {
    RunState run;
    run.startNewRun();
    run.money = 20;

    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::HandUpgrade;
    slot.offer.handType = HandType::Flush;
    slot.offer.price = 6;

    expect(applyShopOfferPurchase(run, slot), "hand-upgrade offer should be purchasable");
    expectEqual(run.handLevel(HandType::Flush), 2, "flush should level up");
    expectEqual(run.handLevel(HandType::Pair), 1, "other hands should stay at level one");
}

void testBuyingJokerOfferAddsOwnedJokerAndRemovesItFromPool() {
    RunState run;
    run.startNewRun();
    run.money = 20;

    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::Joker;
    slot.offer.joker = Joker::plainJoker();
    slot.offer.price = 4;

    expect(applyShopOfferPurchase(run, slot), "joker offer should be purchasable");
    expectEqual(run.money, 16, "joker offer should spend money");
    expectEqual(static_cast<int>(run.jokers.size()), 1, "joker offer should add one owned joker");
    expect(!run.isJokerShopAvailable(Joker::idFor(Joker::plainJoker())),
           "bought joker should be removed from the shop pool");
}

void testShopOfferStringsDescribeNonJokerOffers() {
    ShopOffer deckCardOffer;
    deckCardOffer.kind = ShopOfferKind::DeckCard;
    deckCardOffer.card = Card{Suit::Spades, Rank::Ace};

    expectEqual(shopOfferTitle(deckCardOffer), std::string("Add AS"),
                "deck-card title should use short rank and suit");
    expectEqual(shopOfferDescription(deckCardOffer), std::string("Add Ace of Spades to your deck"),
                "deck-card description should use full card names");

    ShopOffer handUpgradeOffer;
    handUpgradeOffer.kind = ShopOfferKind::HandUpgrade;
    handUpgradeOffer.handType = HandType::Pair;

    expectEqual(shopOfferTitle(handUpgradeOffer), std::string("Level Pair"),
                "hand-upgrade title should name the leveled hand");
    expectEqual(shopOfferDescription(handUpgradeOffer), std::string("Pair gains +10 chips and +1 mult"),
                "hand-upgrade description should use the first-pass upgrade text");
}

} // namespace

int main() {
    testMixedOfferGenerationUsesFixedSlotKinds();
    testBuyingDeckCardOfferAddsOneCardToRunDeck();
    testBuyingHandUpgradeOfferLevelsOnlyThatHand();
    testBuyingJokerOfferAddsOwnedJokerAndRemovesItFromPool();
    testShopOfferStringsDescribeNonJokerOffers();
    std::cout << "Shop offer tests passed\n";
    return 0;
}

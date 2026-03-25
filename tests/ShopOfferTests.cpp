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

} // namespace

int main() {
    testMixedOfferGenerationUsesFixedSlotKinds();
    testBuyingDeckCardOfferAddsOneCardToRunDeck();
    testBuyingHandUpgradeOfferLevelsOnlyThatHand();
    std::cout << "Shop offer tests passed\n";
    return 0;
}

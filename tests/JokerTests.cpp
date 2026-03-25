#include "game/Joker.h"
#include "states/ShopState.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <array>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
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
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

std::set<std::string> catalogNames() {
    std::set<std::string> names;

    for (const Joker& joker : Joker::weakPool()) {
        names.insert(joker.name);
    }
    for (const Joker& joker : Joker::mediumPool()) {
        names.insert(joker.name);
    }
    for (const Joker& joker : Joker::strongPool()) {
        names.insert(joker.name);
    }

    return names;
}

bool shopOffersJokerNamed(const std::array<ShopSlot, kVisibleShopSlots>& slots, const std::string& name) {
    for (const ShopSlot& slot : slots) {
        if (!slot.unavailable && slot.item.joker.name == name) {
            return true;
        }
    }

    return false;
}

void testJokerMetadataMatchesTierBalance() {
    const Joker plainJoker = Joker::plainJoker();
    const Joker heavyJoker = Joker::heavyJoker();

    expect(plainJoker.tier == JokerTier::Weak, "plain joker tier");
    expectEqual(plainJoker.shopPriceRange.min, 4, "weak joker min price");
    expectEqual(heavyJoker.shopPriceRange.max, 10, "strong joker max price");
}

void testWeakOrMediumDrawNeverReturnsStrongJoker() {
    std::mt19937 rng(1337);

    for (int i = 0; i < 512; ++i) {
        const Joker joker = Joker::drawWeakOrMedium(rng);
        expect(joker.tier != JokerTier::Strong, "slot-one draw should exclude strong jokers");
    }
}

void testWeightedDrawAlwaysComesFromNineJokerCatalog() {
    const std::set<std::string> names = catalogNames();
    expectEqual(static_cast<int>(names.size()), 9, "catalog should expose nine joker names");

    std::mt19937 rng(2026);
    for (int i = 0; i < 1024; ++i) {
        const Joker joker = Joker::drawWeightedFullPool(rng);
        expect(names.count(joker.name) == 1, "weighted draw should stay within joker catalog");
    }
}

void testWeightedTierRollPreservesFiftyThirtyFiveFifteenSplit() {
    int weakCount = 0;
    int mediumCount = 0;
    int strongCount = 0;

    for (int roll = 1; roll <= 100; ++roll) {
        const JokerTier tier = Joker::tierForWeightedRoll(roll);
        if (tier == JokerTier::Weak) {
            ++weakCount;
        } else if (tier == JokerTier::Medium) {
            ++mediumCount;
        } else {
            ++strongCount;
        }
    }

    expectEqual(weakCount, 50, "weighted roll weak odds");
    expectEqual(mediumCount, 35, "weighted roll medium odds");
    expectEqual(strongCount, 15, "weighted roll strong odds");
}

void testShopOfferGenerationUsesSlotRulesAndPriceRanges() {
    RunState runState;
    runState.startNewRun();

    std::mt19937 actualRng(77);
    const std::array<ShopSlot, kVisibleShopSlots> slots = ShopState::generateShopItems(actualRng, runState);

    expect(!slots[0].unavailable, "shop slot 0 should have an eligible offer in a fresh run");
    expect(!slots[1].unavailable, "shop slot 1 should have an eligible offer in a fresh run");
    expect(slots[0].item.joker.tier != JokerTier::Strong, "shop slot 0 should exclude strong jokers");
    expect(slots[0].item.price >= slots[0].item.joker.shopPriceRange.min, "shop slot 0 price should respect min");
    expect(slots[0].item.price <= slots[0].item.joker.shopPriceRange.max, "shop slot 0 price should respect max");
    expect(slots[1].item.price >= slots[1].item.joker.shopPriceRange.min, "shop slot 1 price should respect min");
    expect(slots[1].item.price <= slots[1].item.joker.shopPriceRange.max, "shop slot 1 price should respect max");

    std::mt19937 expectedRng(77);
    std::unordered_set<std::string> excludedIds = runState.currentOwnedJokerIds();
    const Joker expectedSlot0 = Joker::drawFromCandidates(Joker::weakOrMediumPoolFiltered(excludedIds), expectedRng);
    const int expectedSlot0Price =
        std::uniform_int_distribution<int>(expectedSlot0.shopPriceRange.min, expectedSlot0.shopPriceRange.max)(expectedRng);
    excludedIds.insert(Joker::idFor(expectedSlot0));

    const JokerTier expectedSlot1Tier =
        Joker::tierForWeightedRoll(std::uniform_int_distribution<int>(1, 100)(expectedRng));
    std::vector<Joker> expectedSlot1Candidates;
    if (expectedSlot1Tier == JokerTier::Weak) {
        expectedSlot1Candidates = Joker::weakPoolFiltered(excludedIds);
    } else if (expectedSlot1Tier == JokerTier::Medium) {
        expectedSlot1Candidates = Joker::mediumPoolFiltered(excludedIds);
    } else {
        expectedSlot1Candidates = Joker::strongPoolFiltered(excludedIds);
    }

    expectEqual(slots[0].item.joker.name, expectedSlot0.name, "shop slot 0 should use filtered weak-or-medium draw");
    expectEqual(slots[0].item.price, expectedSlot0Price, "shop slot 0 should use joker price range");

    if (expectedSlot1Candidates.empty()) {
        expect(slots[1].unavailable, "shop slot 1 should become unavailable when its rolled tier is exhausted");
    } else {
        const Joker expectedSlot1 = Joker::drawFromCandidates(expectedSlot1Candidates, expectedRng);
        const int expectedSlot1Price =
            std::uniform_int_distribution<int>(expectedSlot1.shopPriceRange.min, expectedSlot1.shopPriceRange.max)(expectedRng);
        expectEqual(slots[1].item.joker.name, expectedSlot1.name, "shop slot 1 should use filtered weighted draw");
        expectEqual(slots[1].item.price, expectedSlot1Price, "shop slot 1 should use joker price range");
    }
}

void testJokerIdentityAndFilteredCandidates() {
    // idFor returns stable name-based identity
    expectEqual(Joker::idFor(Joker::plainJoker()), std::string("Plain Joker"),
                "plain joker id should use stable catalog identity");

    // filtering "Plain Joker" excludes it from weak candidates
    std::unordered_set<std::string> excluded = {"Plain Joker"};
    const std::vector<Joker> filteredWeak = Joker::weakPoolFiltered(excluded);
    for (const Joker& j : filteredWeak) {
        expect(j.name != "Plain Joker", "filtered weak pool should exclude Plain Joker");
    }

    // filtering "Plain Joker" excludes it from weak+medium candidates
    const std::vector<Joker> filteredWeakMedium = Joker::weakOrMediumPoolFiltered(excluded);
    for (const Joker& j : filteredWeakMedium) {
        expect(j.name != "Plain Joker", "filtered weak+medium pool should exclude Plain Joker");
    }
}

void testShopOfferGenerationAvoidsSameShopDuplicates() {
    RunState runState;
    runState.startNewRun();

    std::mt19937 rng(0);
    const std::array<ShopSlot, kVisibleShopSlots> slots = ShopState::generateShopItems(rng, runState);

    expect(!slots[0].unavailable, "duplicate-coverage seed should keep slot 0 live");
    expect(!slots[1].unavailable, "duplicate-coverage seed should keep slot 1 live");
    expect(slots[0].item.joker.name != slots[1].item.joker.name,
           "shop should not generate duplicate joker offers");
}

void testOwnedJokersAreExcludedFromFutureShopOffers() {
    RunState runState;
    runState.startNewRun();
    runState.jokers.push_back(Joker::plainJoker());

    for (int seed = 0; seed < 128; ++seed) {
        std::mt19937 rng(seed);
        const std::array<ShopSlot, kVisibleShopSlots> slots = ShopState::generateShopItems(rng, runState);
        expect(!shopOffersJokerNamed(slots, "Plain Joker"),
               "owned jokers should be excluded from future shop offers");
    }
}

void testUnboughtJokersRemainEligibleForLaterShops() {
    RunState runState;
    runState.startNewRun();

    std::mt19937 firstRng(77);
    const std::array<ShopSlot, kVisibleShopSlots> firstShop = ShopState::generateShopItems(firstRng, runState);
    expect(!firstShop[0].unavailable, "first shop should contain a live offer");

    std::mt19937 laterRng(77);
    const std::array<ShopSlot, kVisibleShopSlots> laterShop = ShopState::generateShopItems(laterRng, runState);
    expect(!laterShop[0].unavailable, "later shop should contain a live offer");
    expectEqual(laterShop[0].item.joker.name, firstShop[0].item.joker.name,
                "an unbought joker should remain eligible for later shops");
}

void testShopMarksExhaustedWeightedTierUnavailable() {
    RunState runState;
    runState.startNewRun();
    for (const Joker& joker : Joker::strongPool()) {
        runState.markJokerRemovedFromShopPool(Joker::idFor(joker));
    }

    std::mt19937 rng(6);
    const std::array<ShopSlot, kVisibleShopSlots> slots = ShopState::generateShopItems(rng, runState);

    expect(!slots[0].unavailable, "slot 0 should still find an eligible weak-or-medium offer");
    expect(slots[1].unavailable, "slot 1 should become unavailable when the rolled strong tier is exhausted");
}

void testBuyingJokerRemovesItFromFutureShopOffers() {
    RunState runState;
    runState.startNewRun();
    runState.money = 20;

    std::mt19937 initialRng(77);
    std::array<ShopSlot, kVisibleShopSlots> initialShop = ShopState::generateShopItems(initialRng, runState);
    expect(!initialShop[0].unavailable, "purchase test needs a live slot 0 offer");

    const std::string purchasedName = initialShop[0].item.joker.name;
    const std::string purchasedId = Joker::idFor(initialShop[0].item.joker);
    const int purchasePrice = initialShop[0].item.price;

    shop_state_helpers::purchaseShopSlotAndAdvanceCursor(runState, initialShop, 0);

    expect(!runState.isJokerShopAvailable(purchasedId),
           "buying a joker should remove it from the shop pool");
    expectEqual(runState.money, 20 - purchasePrice,
                "buying a joker should spend the slot price");
    expectEqual(static_cast<int>(runState.jokers.size()), 1,
                "buying a joker should add it to owned jokers");

    std::mt19937 futureRng(77);
    const std::array<ShopSlot, kVisibleShopSlots> futureShop = ShopState::generateShopItems(futureRng, runState);
    expect(!shopOffersJokerNamed(futureShop, purchasedName),
           "bought jokers should not appear in future shops while owned");
}

void testReturnedJokerBecomesEligibleAgainAfterLeavingInventory() {
    RunState runState;
    runState.startNewRun();
    runState.money = 20;

    std::mt19937 initialRng(77);
    std::array<ShopSlot, kVisibleShopSlots> initialShop = ShopState::generateShopItems(initialRng, runState);
    expect(!initialShop[0].unavailable, "return-hook test needs a live slot 0 offer");

    const std::string returnedName = initialShop[0].item.joker.name;
    const std::string returnedId = Joker::idFor(initialShop[0].item.joker);

    shop_state_helpers::purchaseShopSlotAndAdvanceCursor(runState, initialShop, 0);

    runState.jokers.clear();
    runState.markJokerReturnedToShopPool(returnedId);

    std::mt19937 rerollRng(77);
    const std::array<ShopSlot, kVisibleShopSlots> rerolledShop = ShopState::generateShopItems(rerollRng, runState);
    expect(shopOffersJokerNamed(rerolledShop, returnedName),
           "returning a joker to the pool after inventory removal should make it eligible again");
}

} // namespace

int main() {
    try {
        testJokerMetadataMatchesTierBalance();
        testWeakOrMediumDrawNeverReturnsStrongJoker();
        testWeightedDrawAlwaysComesFromNineJokerCatalog();
        testWeightedTierRollPreservesFiftyThirtyFiveFifteenSplit();
        testShopOfferGenerationUsesSlotRulesAndPriceRanges();
        testJokerIdentityAndFilteredCandidates();
        testShopOfferGenerationAvoidsSameShopDuplicates();
        testOwnedJokersAreExcludedFromFutureShopOffers();
        testUnboughtJokersRemainEligibleForLaterShops();
        testShopMarksExhaustedWeightedTierUnavailable();
        testBuyingJokerRemovesItFromFutureShopOffers();
        testReturnedJokerBecomesEligibleAgainAfterLeavingInventory();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "Joker tests passed\n";
    return 0;
}

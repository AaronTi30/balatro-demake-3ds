#include "game/Joker.h"
#include "states/ShopState.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>

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

void testShopOfferGenerationUsesSlotRulesAndPriceRanges() {
    std::mt19937 actualRng(77);
    const std::vector<ShopItem> items = ShopState::generateShopItems(actualRng);

    expectEqual(static_cast<int>(items.size()), 2, "shop should generate two offers");
    expect(items[0].joker.tier != JokerTier::Strong, "shop slot 0 should exclude strong jokers");
    expect(items[0].price >= items[0].joker.shopPriceRange.min, "shop slot 0 price should respect min");
    expect(items[0].price <= items[0].joker.shopPriceRange.max, "shop slot 0 price should respect max");
    expect(items[1].price >= items[1].joker.shopPriceRange.min, "shop slot 1 price should respect min");
    expect(items[1].price <= items[1].joker.shopPriceRange.max, "shop slot 1 price should respect max");

    std::mt19937 expectedRng(77);
    const Joker expectedSlot0 = Joker::drawWeakOrMedium(expectedRng);
    const int expectedSlot0Price =
        std::uniform_int_distribution<int>(expectedSlot0.shopPriceRange.min, expectedSlot0.shopPriceRange.max)(expectedRng);
    const Joker expectedSlot1 = Joker::drawWeightedFullPool(expectedRng);
    const int expectedSlot1Price =
        std::uniform_int_distribution<int>(expectedSlot1.shopPriceRange.min, expectedSlot1.shopPriceRange.max)(expectedRng);

    expectEqual(items[0].joker.name, expectedSlot0.name, "shop slot 0 should use weak-or-medium draw");
    expectEqual(items[0].price, expectedSlot0Price, "shop slot 0 should use joker price range");
    expectEqual(items[1].joker.name, expectedSlot1.name, "shop slot 1 should use weighted full-pool draw");
    expectEqual(items[1].price, expectedSlot1Price, "shop slot 1 should use joker price range");
}

} // namespace

int main() {
    try {
        testJokerMetadataMatchesTierBalance();
        testWeakOrMediumDrawNeverReturnsStrongJoker();
        testWeightedDrawAlwaysComesFromNineJokerCatalog();
        testShopOfferGenerationUsesSlotRulesAndPriceRanges();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "Joker tests passed\n";
    return 0;
}

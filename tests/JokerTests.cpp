#include "game/Joker.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
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

void testJokerMetadataMatchesTierBalance() {
    const Joker plainJoker = Joker::plainJoker();
    const Joker heavyJoker = Joker::heavyJoker();

    expect(plainJoker.tier == JokerTier::Weak, "plain joker tier");
    expectEqual(plainJoker.shopPriceRange.min, 4, "weak joker min price");
    expectEqual(heavyJoker.shopPriceRange.max, 10, "strong joker max price");
}

void testAllJokersHavePositiveSellValue() {
    const std::vector<std::vector<Joker>> pools = {
        Joker::weakPool(), Joker::mediumPool(), Joker::strongPool()
    };
    for (const auto& pool : pools) {
        for (const Joker& j : pool) {
            if (j.sellValue <= 0) {
                fail("Joker \"" + j.name + "\" has non-positive sellValue: "
                     + std::to_string(j.sellValue));
            }
        }
    }
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

void testDrawFromCandidatesFallsBackWhenEmpty() {
    std::mt19937 rng(17);
    const Joker joker = Joker::drawFromCandidates({}, rng);
    expectEqual(joker.name, std::string("Plain Joker"),
                "empty candidate draw should fall back to plain joker");
}

} // namespace

int main() {
    try {
        testJokerMetadataMatchesTierBalance();
        testAllJokersHavePositiveSellValue();
        testWeakOrMediumDrawNeverReturnsStrongJoker();
        testWeightedDrawAlwaysComesFromNineJokerCatalog();
        testWeightedTierRollPreservesFiftyThirtyFiveFifteenSplit();
        testJokerIdentityAndFilteredCandidates();
        testDrawFromCandidatesFallsBackWhenEmpty();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "Joker tests passed\n";
    return 0;
}

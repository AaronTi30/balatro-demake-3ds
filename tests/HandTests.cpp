#include "game/Hand.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

Card makeCard(int rankValue) {
    return { Suit::Spades, static_cast<Rank>(rankValue) };
}

void testSelectionCapsAtFiveCards() {
    Hand hand;
    for (int i = 1; i <= 8; ++i) {
        hand.addCard(makeCard(i));
    }

    for (int i = 0; i < 6; ++i) {
        hand.toggleSelect(i);
    }

    expectEqual(static_cast<int>(hand.getSelected().size()), 5, "hand should not allow selecting more than five cards");
}

void testDeselectionStillWorksAtLimit() {
    Hand hand;
    for (int i = 1; i <= 8; ++i) {
        hand.addCard(makeCard(i));
    }

    for (int i = 0; i < 5; ++i) {
        hand.toggleSelect(i);
    }
    hand.toggleSelect(0);
    hand.toggleSelect(5);

    expectEqual(static_cast<int>(hand.getSelected().size()), 5, "deselecting one card should allow selecting a different card");
}

} // namespace

int main() {
    testSelectionCapsAtFiveCards();
    testDeselectionStillWorksAtLimit();
    std::cout << "Hand tests passed\n";
    return 0;
}

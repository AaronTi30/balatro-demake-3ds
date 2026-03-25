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

Card makePersistentCard(int rankValue, uint32_t instanceId) {
    Card card{ Suit::Spades, static_cast<Rank>(rankValue) };
    card.instanceId = instanceId;
    card.enhancement = CardEnhancement::None;
    card.edition = CardEdition::None;
    card.seal = CardSeal::None;
    return card;
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

void testRemoveSelectedPreservesPersistentCardIdentity() {
    Hand hand;
    hand.addCard(makePersistentCard(1, 101));
    hand.addCard(makePersistentCard(2, 102));
    hand.addCard(makePersistentCard(3, 103));

    hand.toggleSelect(0);
    hand.toggleSelect(2);
    hand.removeSelected();

    expectEqual(hand.size(), 1, "removeSelected should remove only selected cards");
    expectEqual(static_cast<int>(hand.getSelected().size()), 0, "removeSelected should clear selection state");
    expectEqual(static_cast<int>(hand.at(0).instanceId), 102, "remaining card should preserve its original instance id");
}

} // namespace

int main() {
    testSelectionCapsAtFiveCards();
    testDeselectionStillWorksAtLimit();
    testRemoveSelectedPreservesPersistentCardIdentity();
    std::cout << "Hand tests passed\n";
    return 0;
}

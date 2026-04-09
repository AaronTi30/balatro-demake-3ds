#pragma once

#include "Card.h"
#include <vector>

class Hand {
public:
    static constexpr int MAX_HAND_SIZE = 8;

    void addCard(Card card);
    void removeCard(int index);
    void toggleSelect(int index);
    void clearSelection();
    void sortByRankDescending();
    void sortBySuitThenRank();
    
    std::vector<Card> getSelected() const;
    std::vector<int> getSelectedIndices() const;
    void removeSelected();
    bool isSelected(int index) const;

    int size() const;
    bool empty() const;
    bool full() const;
    const Card& at(int i) const;
    Card& at(int i);

private:
    struct HeldCard {
        Card card;
        bool selected = false;
    };

    std::vector<HeldCard> m_cards;
};

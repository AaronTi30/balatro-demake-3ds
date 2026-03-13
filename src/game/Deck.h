#pragma once

#include "Card.h"
#include <vector>
#include <algorithm>
#include <random>

class Deck {
public:
    Deck();
    
    void reset();
    void shuffle();
    Card draw();
    bool empty() const;
    int remaining() const;

private:
    std::vector<Card> m_cards;
    std::mt19937 m_rng;
};

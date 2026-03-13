#include "Deck.h"
#include <chrono>

Deck::Deck() 
    : m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    reset();
}

void Deck::reset() {
    m_cards.clear();
    m_cards.reserve(52);
    
    for (int s = 0; s < 4; ++s) {
        for (int r = 1; r <= 13; ++r) {
            m_cards.push_back({ static_cast<Suit>(s), static_cast<Rank>(r) });
        }
    }
    
    shuffle();
}

void Deck::shuffle() {
    std::shuffle(m_cards.begin(), m_cards.end(), m_rng);
}

Card Deck::draw() {
    Card c = m_cards.back();
    m_cards.pop_back();
    return c;
}

bool Deck::empty() const {
    return m_cards.empty();
}

int Deck::remaining() const {
    return static_cast<int>(m_cards.size());
}

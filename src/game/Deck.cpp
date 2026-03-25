#include "Deck.h"
#include <chrono>

Deck::Deck() 
    : m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

void Deck::loadCards(const std::vector<Card>& cards) {
    m_cards = cards;
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

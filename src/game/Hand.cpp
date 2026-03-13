#include "Hand.h"
#include <algorithm>

void Hand::addCard(Card card) {
    if (static_cast<int>(m_cards.size()) < MAX_HAND_SIZE) {
        m_cards.push_back(card);
    }
}

void Hand::removeCard(int index) {
    if (index >= 0 && index < static_cast<int>(m_cards.size())) {
        m_cards.erase(m_cards.begin() + index);
    }
}

void Hand::toggleSelect(int index) {
    if (index >= 0 && index < static_cast<int>(m_cards.size())) {
        m_cards[index].selected = !m_cards[index].selected;
    }
}

void Hand::clearSelection() {
    for (auto& c : m_cards) {
        c.selected = false;
    }
}

std::vector<Card> Hand::getSelected() const {
    std::vector<Card> sel;
    for (const auto& c : m_cards) {
        if (c.selected) sel.push_back(c);
    }
    return sel;
}

std::vector<int> Hand::getSelectedIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(m_cards.size()); ++i) {
        if (m_cards[i].selected) indices.push_back(i);
    }
    return indices;
}

void Hand::removeSelected() {
    m_cards.erase(
        std::remove_if(m_cards.begin(), m_cards.end(),
                        [](const Card& c) { return c.selected; }),
        m_cards.end()
    );
}

int Hand::size() const {
    return static_cast<int>(m_cards.size());
}

bool Hand::empty() const {
    return m_cards.empty();
}

bool Hand::full() const {
    return static_cast<int>(m_cards.size()) >= MAX_HAND_SIZE;
}

const Card& Hand::at(int i) const {
    return m_cards[i];
}

Card& Hand::at(int i) {
    return m_cards[i];
}

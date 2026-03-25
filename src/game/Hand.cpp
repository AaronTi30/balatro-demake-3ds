#include "Hand.h"
#include <algorithm>

void Hand::addCard(Card card) {
    if (static_cast<int>(m_cards.size()) < MAX_HAND_SIZE) {
        m_cards.push_back(HeldCard{ card, false });
    }
}

void Hand::removeCard(int index) {
    if (index >= 0 && index < static_cast<int>(m_cards.size())) {
        m_cards.erase(m_cards.begin() + index);
    }
}

void Hand::toggleSelect(int index) {
    if (index >= 0 && index < static_cast<int>(m_cards.size())) {
        if (m_cards[index].selected) {
            m_cards[index].selected = false;
            return;
        }

        const int selectedCount = static_cast<int>(std::count_if(
            m_cards.begin(),
            m_cards.end(),
            [](const HeldCard& held) { return held.selected; }));

        if (selectedCount < 5) {
            m_cards[index].selected = true;
        }
    }
}

void Hand::clearSelection() {
    for (auto& held : m_cards) {
        held.selected = false;
    }
}

std::vector<Card> Hand::getSelected() const {
    std::vector<Card> sel;
    for (const auto& held : m_cards) {
        if (held.selected) {
            sel.push_back(held.card);
        }
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
                        [](const HeldCard& held) { return held.selected; }),
        m_cards.end()
    );
}

bool Hand::isSelected(int index) const {
    return index >= 0 &&
           index < static_cast<int>(m_cards.size()) &&
           m_cards[index].selected;
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
    return m_cards[i].card;
}

Card& Hand::at(int i) {
    return m_cards[i].card;
}

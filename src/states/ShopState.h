#pragma once

#include "../core/State.h"
#include "../game/Joker.h"
#include "../game/RunState.h"
#include "states/ShopLayout.h"
#include <array>
#include <memory>
#include <random>
#include <vector>

struct ShopItem {
    Joker joker;
    int price;
};

struct ShopSlot {
    ShopItem item;
    bool sold = false;
};

class ShopState : public State {
public:
    ShopState(StateMachine* machine, std::shared_ptr<RunState> runState);
    static std::vector<ShopItem> generateShopItems(std::mt19937& rng);

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void handleInput() override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    void generateItems();
    void clearHeldInspect();
    bool tryBuySelectedItem();
    std::array<bool, kVisibleShopSlots> soldMask() const;

    std::shared_ptr<RunState> m_runState;
    std::array<ShopSlot, kVisibleShopSlots> m_slots{};
    int m_cursorIndex = 0;
    int m_heldInspectIndex = -1;
    float m_inputDelay = 0.3f;
    std::mt19937 m_rng;
#ifndef N3DS
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;
#endif
};

inline std::vector<ShopItem> ShopState::generateShopItems(std::mt19937& rng) {
    auto drawPrice = [&rng](const ShopPriceRange& range) {
        return std::uniform_int_distribution<int>(range.min, range.max)(rng);
    };

    ShopItem firstItem;
    firstItem.joker = Joker::drawWeakOrMedium(rng);
    firstItem.price = drawPrice(firstItem.joker.shopPriceRange);

    ShopItem secondItem;
    secondItem.joker = Joker::drawWeightedFullPool(rng);
    secondItem.price = drawPrice(secondItem.joker.shopPriceRange);

    return { firstItem, secondItem };
}

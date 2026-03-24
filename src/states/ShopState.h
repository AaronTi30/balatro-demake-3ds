#pragma once

#include "../core/State.h"
#include "../game/Joker.h"
#include "../game/RunState.h"
#include <memory>
#include <random>
#include <vector>

struct ShopItem {
    Joker joker;
    int price;
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

    std::shared_ptr<RunState> m_runState;
    std::vector<ShopItem> m_items;
    int m_cursorIndex;
    float m_inputDelay;
    std::mt19937 m_rng;
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

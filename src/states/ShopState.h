#pragma once

#include "../core/State.h"
#include "../game/Joker.h"
#include <vector>

struct ShopItem {
    Joker joker;
    int price;
};

class ShopState : public State {
public:
    ShopState(StateMachine* machine, int nextAnte, int currentMoney, const std::vector<Joker>& currentJokers);

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void handleInput() override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    void generateItems();

    int m_nextAnte;
    int m_money;
    std::vector<Joker> m_jokers;
    
    std::vector<ShopItem> m_items;
    int m_cursorIndex;
    float m_inputDelay;
};

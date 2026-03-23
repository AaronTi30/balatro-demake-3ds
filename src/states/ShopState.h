#pragma once

#include "../core/State.h"
#include "../game/Joker.h"
#include "../game/RunState.h"
#include <memory>
#include <vector>

struct ShopItem {
    Joker joker;
    int price;
};

class ShopState : public State {
public:
    ShopState(StateMachine* machine, std::shared_ptr<RunState> runState);

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
};

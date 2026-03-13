#pragma once

#include "../core/State.h"
#include "../game/Hand.h"
#include <string>

class TitleState : public State {
public:
    TitleState(StateMachine* machine);
    ~TitleState() override = default;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    std::string m_titleText;
    Hand m_sampleHand;
};

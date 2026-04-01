#pragma once

#include "../core/State.h"

class TitleState : public State {
public:
    TitleState(StateMachine* machine);
    ~TitleState() override = default;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app, ScreenRenderer& r) override;
    void renderBottomScreen(Application* app, ScreenRenderer& r) override;

private:
    void startRun();

#ifndef N3DS
    bool m_mouseWasPressed = false;
#endif
};

#pragma once

#include "../core/State.h"
#include "../game/RunState.h"
#include <memory>

class BlindSelectState : public State {
public:
    BlindSelectState(StateMachine* machine, std::shared_ptr<RunState> runState);
    ~BlindSelectState() override = default;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app, ScreenRenderer& r) override;
    void renderBottomScreen(Application* app, ScreenRenderer& r) override;

private:
    void confirmSelection();
    bool canSkip() const;

    std::shared_ptr<RunState> m_runState;
    int m_cursorIndex = 0;
    float m_inputDelay = 0.3f;
};

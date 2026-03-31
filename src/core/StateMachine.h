#pragma once

#include "State.h"
#include <memory>
#include <vector>

class StateMachine {
public:
    StateMachine();
    ~StateMachine() = default;

    void pushState(std::shared_ptr<State> state);
    void popState();
    void changeState(std::shared_ptr<State> state);

    void processStateChanges();

    // Call through to the active state
    void handleInput();
    void update(float dt);
    void renderTopScreen(Application* app, ScreenRenderer& r);
    void renderBottomScreen(Application* app, ScreenRenderer& r);

private:
    std::vector<std::shared_ptr<State>> m_states;

    // Track state changing so we don't accidentally rip out the active state
    // mid-update loop
    std::shared_ptr<State> m_newState;
    bool m_isAdding;
    bool m_isRemoving;
    bool m_isReplacing;
};

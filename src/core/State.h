#pragma once

class Application;
class StateMachine;

class State {
public:
    virtual ~State() = default;

    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleInput() = 0;
    virtual void update(float dt) = 0;
    virtual void renderTopScreen(Application* app) = 0;
    virtual void renderBottomScreen(Application* app) = 0;

protected:
    StateMachine* m_stateMachine;
};

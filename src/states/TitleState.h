#pragma once

#include "../core/State.h"

#ifndef N3DS
struct SDL_Texture;
#endif

class TitleState : public State {
public:
    TitleState(StateMachine* machine);
    ~TitleState() override;

    void enter() override;
    void exit() override;
    void handleInput() override;
    void update(float dt) override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    void startRun();

#ifndef N3DS
    void ensureDesktopAssets(Application* app);
    void destroyDesktopAssets();

    SDL_Texture* m_logoTex;
    bool m_mouseWasPressed;
#endif
};

#pragma once

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <memory>
#include <string>

// Forward declaration for state machine
class StateMachine;

class Application {
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    bool init();
    void run();
    void quit();

#ifndef N3DS
    SDL_Renderer* getRenderer() const { return m_renderer; }
#endif

private:
    void handleEvents();
    void update(float dt);
    void render();

    bool m_isRunning;
    std::string m_title;
    int m_width;
    int m_height;

#ifdef N3DS
    C3D_RenderTarget* m_topScreen;
    C3D_RenderTarget* m_bottomScreen;
#else
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
#endif

    std::unique_ptr<StateMachine> m_stateMachine;

};

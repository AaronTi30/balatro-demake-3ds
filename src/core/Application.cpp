#include "Application.h"
#include "ScreenRenderer.h"
#include "TextRenderer.h"
#include "../states/TitleState.h"
#include "StateMachine.h"
#include <iostream>

#ifdef N3DS
// 3DS includes are in the header
#else
#include <SDL.h>
#endif

Application::Application(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_isRunning(false) {
#ifdef N3DS
    m_topScreen = nullptr;
    m_bottomScreen = nullptr;
#else
    m_window = nullptr;
    m_renderer = nullptr;
#endif

    m_stateMachine = std::make_unique<StateMachine>();
    m_stateMachine->pushState(std::make_shared<TitleState>(m_stateMachine.get()));
}

Application::~Application() {
    quit();
}

bool Application::init() {
#ifdef N3DS
    // Initialize 3DS Graphics
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    m_topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    m_bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    m_isRunning = true;
    return true;
#else
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window = SDL_CreateWindow(m_title.c_str(), 
                                SDL_WINDOWPOS_UNDEFINED, 
                                SDL_WINDOWPOS_UNDEFINED, 
                                m_width, m_height, 
                                SDL_WINDOW_SHOWN);
    if (m_window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_isRunning = true;
    TextRenderer::init();
    return true;
#endif
}

void Application::run() {
    if (!init()) {
        std::cerr << "Application Failed to Initialize" << std::endl;
        return;
    }

#ifdef N3DS
    // 3DS Main Loop
    while (aptMainLoop() && m_isRunning) {
        handleEvents();
        update(1.0f / 60.0f); // Fixed dt for now
        render();
    }
#else
    // PC Main Loop
    Uint32 lastTime = SDL_GetTicks();
    while (m_isRunning) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        handleEvents();
        update(dt);
        render();
    }
#endif
}

void Application::handleEvents() {
#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START) {
        m_isRunning = false;
    }
#else
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            m_isRunning = false;
        }
    }
#endif
}

void Application::update(float dt) {
    if (m_stateMachine) {
        m_stateMachine->processStateChanges();
        m_stateMachine->handleInput();
        m_stateMachine->update(dt);
    }
}

void Application::render() {
#ifdef N3DS
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    ScreenRenderer topR;
    ScreenRenderer botR;

    // Top Screen Content
    C2D_TargetClear(m_topScreen, C2D_Color32(40, 50, 40, 255));
    C2D_SceneBegin(m_topScreen);
    if (m_stateMachine) m_stateMachine->renderTopScreen(this, topR);

    // Bottom Screen Content
    C2D_TargetClear(m_bottomScreen, C2D_Color32(30, 40, 30, 255));
    C2D_SceneBegin(m_bottomScreen);
    if (m_stateMachine) m_stateMachine->renderBottomScreen(this, botR);

    C3D_FrameEnd(0);
#else
    ScreenRenderer topR(m_renderer, 0);
    ScreenRenderer botR(m_renderer, 400);

    // Clear screen to a dark grey
    SDL_SetRenderDrawColor(m_renderer, 50, 60, 50, 255);
    SDL_RenderClear(m_renderer);

    // Render "Top Screen" content through state machine
    // We'll use a viewport/clipping approach or just pass the renderer
    // For now, let's keep it simple and draw relative to 0,0
    if (m_stateMachine) m_stateMachine->renderTopScreen(this, topR);

    // The bottom screen is shifted right by 400 in our PC proxy
    // We'll need a way for the state to know its offset or handle it here
    // For now, let's just use a simple offset in the state's render call if we can
    if (m_stateMachine) m_stateMachine->renderBottomScreen(this, botR);

    // Update screen
    SDL_RenderPresent(m_renderer);
#endif
}

void Application::quit() {
    m_isRunning = false;
    TextRenderer::shutdown();

#ifdef N3DS
    C2D_Fini();
    C3D_Fini();
    gfxExit();
#else
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
#endif
}

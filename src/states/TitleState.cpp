#include "TitleState.h"

#include "GameplayState.h"
#include "../core/Application.h"
#include "../core/AssetPath.h"
#include "../core/ScreenRenderer.h"
#include "../core/StateMachine.h"
#include "../game/RunState.h"

#include <memory>

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#endif

namespace {

constexpr int kBackgroundR = 0x1a;
constexpr int kBackgroundG = 0x0a;
constexpr int kBackgroundB = 0x14;

struct Rect {
    int x;
    int y;
    int w;
    int h;
};

struct ButtonVisual {
    Rect rect;
    uint8_t fillR;
    uint8_t fillG;
    uint8_t fillB;
    uint8_t outlineR;
    uint8_t outlineG;
    uint8_t outlineB;
    const char* label;
    float labelX;
    float labelY;
    float labelScale;
};

constexpr int kBottomScreenOffsetX = 400;
constexpr Rect kTopBackgroundRect{0, 0, 400, 240};
constexpr Rect kLogoPanelOuterRect{62, 24, 276, 148};
constexpr Rect kLogoPanelInnerRect{74, 36, 252, 124};
constexpr Rect kLogoDividerRect{114, 98, 172, 2};
constexpr Rect kBottomBackgroundRect{0, 0, 320, 240};

constexpr ButtonVisual kPlayButton{
    {70, 156, 180, 44},
    54, 144, 74,
    104, 208, 128,
    "PLAY",
    130.0f, 170.0f, 0.55f
};

constexpr ButtonVisual kCollectionButton{
    {70, 28, 180, 32},
    30, 107, 58,
    62, 144, 85,
    "COLLECTION",
    96.0f, 39.0f, 0.38f
};

constexpr ButtonVisual kOptionsButton{
    {70, 76, 180, 32},
    155, 74, 26,
    197, 111, 56,
    "OPTIONS",
    108.0f, 87.0f, 0.38f
};

constexpr ButtonVisual kQuitButton{
    {70, 116, 180, 32},
    139, 32, 32,
    183, 76, 76,
    "QUIT",
    130.0f, 127.0f, 0.42f
};

#ifndef N3DS
enum class TitleButton {
    Collection,
    Options,
    Quit,
    Play
};

Rect desktopInputRect(const ButtonVisual& button) {
    return {button.rect.x + kBottomScreenOffsetX, button.rect.y, button.rect.w, button.rect.h};
}

Rect buttonRect(TitleButton button) {
    switch (button) {
        case TitleButton::Collection: return desktopInputRect(kCollectionButton);
        case TitleButton::Options: return desktopInputRect(kOptionsButton);
        case TitleButton::Quit: return desktopInputRect(kQuitButton);
        case TitleButton::Play: return desktopInputRect(kPlayButton);
        default: return desktopInputRect(kPlayButton);
    }
}

bool pointInRect(const Rect& rect, int px, int py) {
    return px >= rect.x && px < rect.x + rect.w && py >= rect.y && py < rect.y + rect.h;
}
#endif

void drawButton(ScreenRenderer& r, const ButtonVisual& button) {
    r.fillRect(button.rect.x,
               button.rect.y,
               button.rect.w,
               button.rect.h,
               button.fillR,
               button.fillG,
               button.fillB);
    r.drawRectOutline(button.rect.x,
                      button.rect.y,
                      button.rect.w,
                      button.rect.h,
                      button.outlineR,
                      button.outlineG,
                      button.outlineB);
    r.drawText(button.label,
               button.labelX,
               button.labelY,
               button.labelScale,
               255,
               255,
               255);
}

} // namespace

TitleState::TitleState(StateMachine* machine) {
    m_stateMachine = machine;
#ifndef N3DS
    m_logoTex = nullptr;
    m_mouseWasPressed = false;
#endif
}

TitleState::~TitleState() {
#ifndef N3DS
    destroyDesktopAssets();
#endif
}

void TitleState::enter() {
#ifndef N3DS
    m_mouseWasPressed = false;
#endif
}

void TitleState::exit() {
#ifndef N3DS
    destroyDesktopAssets();
#endif
}

void TitleState::startRun() {
    auto runState = std::make_shared<RunState>();
    runState->startNewRun();
    m_stateMachine->changeState(
        std::make_shared<GameplayState>(m_stateMachine, runState));
}

#ifndef N3DS
void TitleState::ensureDesktopAssets(Application* app) {
    if (!m_logoTex) {
        m_logoTex = sdlLoadTexture(app->getRenderer(), "assets/textures/balatro.png");
    }
}

void TitleState::destroyDesktopAssets() {
    if (m_logoTex && SDL_WasInit(SDL_INIT_VIDEO) != 0) {
        SDL_DestroyTexture(m_logoTex);
    }
    m_logoTex = nullptr;
}
#endif

void TitleState::handleInput() {
#ifdef N3DS
    hidScanInput();
    const u32 kDown = hidKeysDown();
    if (kDown & KEY_A) {
        startRun();
    }
#else
    int mx = 0;
    int my = 0;
    const Uint32 buttons = SDL_GetMouseState(&mx, &my);
    const bool isPressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (isPressed && !m_mouseWasPressed) {
        if (pointInRect(buttonRect(TitleButton::Collection), mx, my) ||
            pointInRect(buttonRect(TitleButton::Options), mx, my)) {
            m_mouseWasPressed = isPressed;
            return;
        }

        if (pointInRect(buttonRect(TitleButton::Play), mx, my)) {
            startRun();
            m_mouseWasPressed = isPressed;
            return;
        }

        if (pointInRect(buttonRect(TitleButton::Quit), mx, my)) {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }
    }

    m_mouseWasPressed = isPressed;
#endif
}

void TitleState::update(float dt) {
    (void)dt;
}

void TitleState::renderTopScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    r.fillRect(kTopBackgroundRect.x,
               kTopBackgroundRect.y,
               kTopBackgroundRect.w,
               kTopBackgroundRect.h,
               kBackgroundR,
               kBackgroundG,
               kBackgroundB);
    r.fillRect(kLogoPanelOuterRect.x, kLogoPanelOuterRect.y, kLogoPanelOuterRect.w, kLogoPanelOuterRect.h,
               61, 38, 24);
    r.drawRectOutline(kLogoPanelOuterRect.x, kLogoPanelOuterRect.y, kLogoPanelOuterRect.w, kLogoPanelOuterRect.h,
                      130, 90, 54);
    r.fillRect(kLogoPanelInnerRect.x, kLogoPanelInnerRect.y, kLogoPanelInnerRect.w, kLogoPanelInnerRect.h,
               30, 18, 29);
    r.drawRectOutline(kLogoPanelInnerRect.x, kLogoPanelInnerRect.y, kLogoPanelInnerRect.w, kLogoPanelInnerRect.h,
                      210, 164, 94);
    r.fillRect(kLogoDividerRect.x, kLogoDividerRect.y, kLogoDividerRect.w, kLogoDividerRect.h,
               118, 88, 48);

    r.drawText("BALATRO", 108.0f, 66.0f, 0.75f, 240, 220, 180);
    r.drawText("DEMAKE", 137.0f, 102.0f, 0.55f, 215, 215, 230);
    r.drawText("PRESS START", 124.0f, 136.0f, 0.40f, 220, 196, 110);
    r.drawText("A tiny handheld run", 110.0f, 168.0f, 0.33f, 200, 200, 210);
}

void TitleState::renderBottomScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    r.fillRect(kBottomBackgroundRect.x,
               kBottomBackgroundRect.y,
               kBottomBackgroundRect.w,
               kBottomBackgroundRect.h,
               kBackgroundR,
               kBackgroundG,
               kBackgroundB);

    r.drawText("SELECT AN OPTION", 86.0f, 12.0f, 0.35f, 210, 210, 220);
    drawButton(r, kCollectionButton);
    drawButton(r, kOptionsButton);
    drawButton(r, kQuitButton);
    drawButton(r, kPlayButton);
    r.drawText("Press A to start a run", 78.0f, 214.0f, 0.38f, 210, 210, 220);
}

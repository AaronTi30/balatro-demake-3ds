#include "TitleState.h"

#include "GameplayState.h"
#include "../core/Application.h"
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

constexpr int kBottomScreenOffsetX = 400;
constexpr Rect kTopBackgroundRect{0, 0, 400, 240};
constexpr Rect kLogoPanelOuterRect{62, 24, 276, 148};
constexpr Rect kLogoPanelInnerRect{74, 36, 252, 124};
constexpr Rect kLogoDividerRect{114, 98, 172, 2};
constexpr Rect kBottomBackgroundRect{0, 0, 320, 240};
constexpr Rect kPlayButtonRect{70, 156, 180, 44};

#ifndef N3DS
Rect desktopPlayButtonRect() {
    return {kPlayButtonRect.x + kBottomScreenOffsetX, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h};
}

bool pointInRect(const Rect& rect, int px, int py) {
    return px >= rect.x && px < rect.x + rect.w && py >= rect.y && py < rect.y + rect.h;
}
#endif

} // namespace

TitleState::TitleState(StateMachine* machine) {
    m_stateMachine = machine;
#ifndef N3DS
    m_mouseWasPressed = false;
#endif
}

void TitleState::enter() {
#ifndef N3DS
    m_mouseWasPressed = false;
#endif
}

void TitleState::exit() {
}

void TitleState::startRun() {
    auto runState = std::make_shared<RunState>();
    runState->startNewRun();
    m_stateMachine->changeState(
        std::make_shared<GameplayState>(m_stateMachine, runState));
}

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
    if (isPressed && !m_mouseWasPressed &&
        pointInRect(desktopPlayButtonRect(), mx, my)) {
        startRun();
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
    r.drawText("READY TO START", 114.0f, 136.0f, 0.35f, 220, 196, 110);
    r.drawText("Start a new run", 126.0f, 168.0f, 0.33f, 200, 200, 210);
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

    r.drawText("START A NEW RUN", 96.0f, 72.0f, 0.35f, 210, 210, 220);
    r.fillRect(kPlayButtonRect.x, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h,
               54, 144, 74);
    r.drawRectOutline(kPlayButtonRect.x, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h,
                      104, 208, 128);
    r.drawText("PLAY", 130.0f, 170.0f, 0.55f, 255, 255, 255);
    r.drawText("Select PLAY to begin", 92.0f, 214.0f, 0.30f, 210, 210, 220);
}

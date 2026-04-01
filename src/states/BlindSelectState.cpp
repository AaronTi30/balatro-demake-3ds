#include "BlindSelectState.h"

#include "GameplayState.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
#include "../core/StateMachine.h"

#ifdef N3DS
#include <3ds.h>
#else
#include <SDL.h>
#endif

#include <string>
#include <utility>

namespace {

struct BRect {
    int x;
    int y;
    int w;
    int h;
};

constexpr int kBottomScreenOffsetX = 400;
constexpr BRect kPlayButtonRect{20, 170, 120, 45};
constexpr BRect kSkipButtonRect{180, 170, 120, 45};

#ifndef N3DS
BRect desktopButtonRect(const BRect& rect) {
    return {rect.x + kBottomScreenOffsetX, rect.y, rect.w, rect.h};
}
#endif

bool ptInBRect(const BRect& r, int px, int py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

} // namespace

BlindSelectState::BlindSelectState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)) {
    m_stateMachine = machine;
}

void BlindSelectState::enter() {
    m_cursorIndex = 0;
    m_inputDelay = 0.3f;
}

void BlindSelectState::exit() {}

bool BlindSelectState::canSkip() const {
    return m_runState->nextBlindStage() != BlindStage::Boss;
}

void BlindSelectState::confirmSelection() {
    if (m_cursorIndex == 1 && canSkip()) {
        m_runState->awardBlindSkip();
        m_runState->advanceBlind();
        m_stateMachine->changeState(
            std::make_shared<BlindSelectState>(m_stateMachine, m_runState));
    } else {
        m_runState->advanceBlind();
        m_stateMachine->changeState(
            std::make_shared<GameplayState>(m_stateMachine, m_runState));
    }
}

void BlindSelectState::update(float dt) {
    if (m_inputDelay > 0.0f) {
        m_inputDelay -= dt;
    }
}

void BlindSelectState::handleInput() {
    if (m_inputDelay > 0.0f) {
        return;
    }

#ifdef N3DS
    u32 kDown = hidKeysDown();

    if (kDown & (KEY_LEFT | KEY_DLEFT)) {
        if (m_cursorIndex > 0) {
            m_cursorIndex--;
        }
    }
    if (kDown & (KEY_RIGHT | KEY_DRIGHT)) {
        if (canSkip() && m_cursorIndex < 1) {
            m_cursorIndex++;
        }
    }
    if (kDown & KEY_A) {
        confirmSelection();
        return;
    }
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (ptInBRect(kPlayButtonRect, touch.px, touch.py)) {
            m_cursorIndex = 0;
            confirmSelection();
        } else if (canSkip() && ptInBRect(kSkipButtonRect, touch.px, touch.py)) {
            m_cursorIndex = 1;
            confirmSelection();
        }
    }
#else
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    static bool leftPressed = false;
    static bool rightPressed = false;
    static bool confirmPressed = false;

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!leftPressed) {
            leftPressed = true;
            if (m_cursorIndex > 0) {
                m_cursorIndex--;
            }
        }
    } else {
        leftPressed = false;
    }

    if (keys[SDL_SCANCODE_RIGHT]) {
        if (!rightPressed) {
            rightPressed = true;
            if (canSkip() && m_cursorIndex < 1) {
                m_cursorIndex++;
            }
        }
    } else {
        rightPressed = false;
    }

    if (keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE]) {
        if (!confirmPressed) {
            confirmPressed = true;
            confirmSelection();
            return;
        }
    } else {
        confirmPressed = false;
    }

    static bool mousePressed = false;
    int mx;
    int my;
    uint32_t mouseState = SDL_GetMouseState(&mx, &my);
    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        if (!mousePressed) {
            mousePressed = true;
            if (mx >= 400) {
                if (ptInBRect(desktopButtonRect(kPlayButtonRect), mx, my)) {
                    m_cursorIndex = 0;
                    confirmSelection();
                } else if (canSkip() && ptInBRect(desktopButtonRect(kSkipButtonRect), mx, my)) {
                    m_cursorIndex = 1;
                    confirmSelection();
                }
            }
        }
    } else {
        mousePressed = false;
    }
#endif
}

void BlindSelectState::renderTopScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    const BlindStage upcoming = m_runState->nextBlindStage();
    const int upcomingAnte = m_runState->nextBlindAnte();
    const bool isBoss = (upcoming == BlindStage::Boss);
    const int target = RunState::targetForBlind(upcomingAnte, upcoming);
    const int stageIdx = (upcoming == BlindStage::Small) ? 0
                       : (upcoming == BlindStage::Big) ? 1 : 2;
    const int reward = RunState::kBlindRewards[stageIdx];

    r.fillRect(0, 0, 400, 240, 15, 20, 35);
    r.drawText("ANTE " + std::to_string(upcomingAnte),
               14.0f, 20.0f, 0.70f, 255, 200, 80);
    r.drawText(RunState::blindStageName(upcoming),
               14.0f, 50.0f, 0.55f, 180, 200, 255);
    r.drawText("Target: " + std::to_string(target) + " chips",
               14.0f, 80.0f, 0.45f, 220, 220, 220);
    r.drawText("Reward: $" + std::to_string(reward),
               14.0f, 105.0f, 0.45f, 255, 215, 0);
    if (isBoss) {
        r.drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                   14.0f, 135.0f, 0.38f, 255, 140, 80);
        r.drawText(RunState::bossModifierDescription(m_runState->nextBossModifier,
                                                     m_runState->nextBlockedSuit),
                   14.0f, 158.0f, 0.33f, 210, 210, 210);
    }
}

void BlindSelectState::renderBottomScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    const bool boss = !canSkip();
    const bool playSelected = (m_cursorIndex == 0);
    const bool skipSelected = (m_cursorIndex == 1);

    r.fillRect(0, 0, 320, 240, 15, 20, 35);

    r.fillRect(kPlayButtonRect.x, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h, 25, 70, 25);
    if (playSelected) {
        r.drawRectOutline(kPlayButtonRect.x, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h,
                          255, 255, 80);
    } else {
        r.drawRectOutline(kPlayButtonRect.x, kPlayButtonRect.y, kPlayButtonRect.w, kPlayButtonRect.h,
                          80, 200, 80);
    }
    r.drawText("PLAY", 48.0f, 184.0f, 0.55f, 255, 255, 255);

    if (!boss) {
        r.fillRect(kSkipButtonRect.x, kSkipButtonRect.y, kSkipButtonRect.w, kSkipButtonRect.h, 55, 40, 15);
        if (skipSelected) {
            r.drawRectOutline(kSkipButtonRect.x, kSkipButtonRect.y, kSkipButtonRect.w, kSkipButtonRect.h,
                              255, 255, 80);
        } else {
            r.drawRectOutline(kSkipButtonRect.x, kSkipButtonRect.y, kSkipButtonRect.w, kSkipButtonRect.h,
                              200, 150, 50);
        }
        r.drawText("SKIP +$" + std::to_string(RunState::kBlindSkipReward),
                   185.0f, 184.0f, 0.42f, 255, 215, 80);
    } else {
        r.fillRect(kSkipButtonRect.x, kSkipButtonRect.y, kSkipButtonRect.w, kSkipButtonRect.h, 35, 35, 35);
        r.drawRectOutline(kSkipButtonRect.x, kSkipButtonRect.y, kSkipButtonRect.w, kSkipButtonRect.h,
                          65, 65, 65);
        r.drawText("SKIP", 200.0f, 184.0f, 0.45f, 85, 85, 85);
    }
}

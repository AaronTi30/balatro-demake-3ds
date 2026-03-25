#include "BlindSelectState.h"

#include "GameplayState.h"
#include "../core/Application.h"
#include "../core/StateMachine.h"
#include "../core/TextRenderer.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
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

BRect playBtnRect(bool is3DS) {
    return is3DS ? BRect{20, 170, 120, 45} : BRect{420, 170, 140, 45};
}

BRect skipBtnRect(bool is3DS) {
    return is3DS ? BRect{180, 170, 120, 45} : BRect{590, 170, 140, 45};
}

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

void BlindSelectState::handleInput() {}

void BlindSelectState::renderTopScreen(Application* app) {
    const BlindStage upcoming = m_runState->nextBlindStage();
    const int upcomingAnte = m_runState->nextBlindAnte();
    const bool isBoss = (upcoming == BlindStage::Boss);
    const int target = RunState::targetForBlind(upcomingAnte, upcoming);
    const int stageIdx = (upcoming == BlindStage::Small) ? 0
                       : (upcoming == BlindStage::Big) ? 1 : 2;
    const int reward = RunState::kBlindRewards[stageIdx];

#ifdef N3DS
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, C2D_Color32(15, 20, 35, 255));
    TextRenderer::drawText("ANTE " + std::to_string(upcomingAnte),
                           14, 20, 0.7f, 0.7f, C2D_Color32(255, 200, 80, 255));
    TextRenderer::drawText(RunState::blindStageName(upcoming),
                           14, 50, 0.55f, 0.55f, C2D_Color32(180, 200, 255, 255));
    TextRenderer::drawText("Target: " + std::to_string(target) + " chips",
                           14, 80, 0.45f, 0.45f, C2D_Color32(220, 220, 220, 255));
    TextRenderer::drawText("Reward: $" + std::to_string(reward),
                           14, 105, 0.45f, 0.45f, C2D_Color32(255, 215, 0, 255));
    if (isBoss) {
        TextRenderer::drawText(
            "Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
            14, 135, 0.38f, 0.38f, C2D_Color32(255, 140, 80, 255));
        TextRenderer::drawText(
            RunState::bossModifierDescription(m_runState->nextBossModifier,
                                              m_runState->nextBlockedSuit),
            14, 158, 0.33f, 0.33f, C2D_Color32(210, 210, 210, 255));
    }
#else
    SDL_Renderer* renderer = app->getRenderer();
    SDL_SetRenderDrawColor(renderer, 15, 20, 35, 255);
    SDL_Rect bg = {0, 0, 400, 240};
    SDL_RenderFillRect(renderer, &bg);

    TextRenderer::drawText(renderer, "ANTE " + std::to_string(upcomingAnte),
                           14, 20, 2, 255, 200, 80);
    TextRenderer::drawText(renderer, RunState::blindStageName(upcoming),
                           14, 50, 1, 180, 200, 255);
    TextRenderer::drawText(renderer, "Target: " + std::to_string(target) + " chips",
                           14, 80, 0, 220, 220, 220);
    TextRenderer::drawText(renderer, "Reward: $" + std::to_string(reward),
                           14, 105, 0, 255, 215, 0);
    if (isBoss) {
        TextRenderer::drawText(renderer,
            "Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
            14, 135, 0, 255, 140, 80);
        TextRenderer::drawText(renderer,
            RunState::bossModifierDescription(m_runState->nextBossModifier,
                                              m_runState->nextBlockedSuit),
            14, 158, 0, 210, 210, 210);
    }
#endif
}

void BlindSelectState::renderBottomScreen(Application* app) {
    const bool boss = !canSkip();
    const bool playSelected = (m_cursorIndex == 0);
    const bool skipSelected = (m_cursorIndex == 1);

#ifdef N3DS
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 240, C2D_Color32(15, 20, 35, 255));

    const u32 playBorder = playSelected
        ? C2D_Color32(255, 255, 80, 255) : C2D_Color32(80, 200, 80, 255);
    C2D_DrawRectSolid(20, 170, 0.5f, 120, 45, C2D_Color32(25, 70, 25, 255));
    C2D_DrawRectSolid(20, 170, 0.5f, 120, 2, playBorder);
    C2D_DrawRectSolid(20, 213, 0.5f, 120, 2, playBorder);
    C2D_DrawRectSolid(20, 170, 0.5f, 2, 45, playBorder);
    C2D_DrawRectSolid(138, 170, 0.5f, 2, 45, playBorder);
    TextRenderer::drawText("PLAY", 48, 184, 0.55f, 0.55f, C2D_Color32(255, 255, 255, 255));

    if (!boss) {
        const u32 skipBorder = skipSelected
            ? C2D_Color32(255, 255, 80, 255) : C2D_Color32(200, 150, 50, 255);
        C2D_DrawRectSolid(180, 170, 0.5f, 120, 45, C2D_Color32(55, 40, 15, 255));
        C2D_DrawRectSolid(180, 170, 0.5f, 120, 2, skipBorder);
        C2D_DrawRectSolid(180, 213, 0.5f, 120, 2, skipBorder);
        C2D_DrawRectSolid(180, 170, 0.5f, 2, 45, skipBorder);
        C2D_DrawRectSolid(298, 170, 0.5f, 2, 45, skipBorder);
        TextRenderer::drawText("SKIP +$" + std::to_string(RunState::kBlindSkipReward),
                               185, 184, 0.42f, 0.42f, C2D_Color32(255, 215, 80, 255));
    } else {
        C2D_DrawRectSolid(180, 170, 0.5f, 120, 45, C2D_Color32(35, 35, 35, 255));
        C2D_DrawRectSolid(180, 170, 0.5f, 120, 2, C2D_Color32(65, 65, 65, 255));
        C2D_DrawRectSolid(180, 213, 0.5f, 120, 2, C2D_Color32(65, 65, 65, 255));
        C2D_DrawRectSolid(180, 170, 0.5f, 2, 45, C2D_Color32(65, 65, 65, 255));
        C2D_DrawRectSolid(298, 170, 0.5f, 2, 45, C2D_Color32(65, 65, 65, 255));
        TextRenderer::drawText("SKIP", 200, 184, 0.45f, 0.45f,
                               C2D_Color32(85, 85, 85, 255));
    }
#else
    SDL_Renderer* renderer = app->getRenderer();
    SDL_SetRenderDrawColor(renderer, 15, 20, 35, 255);
    SDL_Rect bg = {400, 0, 400, 240};
    SDL_RenderFillRect(renderer, &bg);

    {
        SDL_Rect fill = {420, 170, 140, 45};
        SDL_SetRenderDrawColor(renderer, 25, 70, 25, 255);
        SDL_RenderFillRect(renderer, &fill);
        if (playSelected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 80, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
        }
        SDL_RenderDrawRect(renderer, &fill);
        TextRenderer::drawText(renderer, "PLAY", 465, 184, 1, 255, 255, 255);
    }

    if (!boss) {
        SDL_Rect fill = {590, 170, 140, 45};
        SDL_SetRenderDrawColor(renderer, 55, 40, 15, 255);
        SDL_RenderFillRect(renderer, &fill);
        if (skipSelected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 80, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 150, 50, 255);
        }
        SDL_RenderDrawRect(renderer, &fill);
        TextRenderer::drawText(renderer,
                               "SKIP +$" + std::to_string(RunState::kBlindSkipReward),
                               600, 184, 0, 255, 215, 80);
    } else {
        SDL_Rect fill = {590, 170, 140, 45};
        SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
        SDL_RenderFillRect(renderer, &fill);
        SDL_SetRenderDrawColor(renderer, 65, 65, 65, 255);
        SDL_RenderDrawRect(renderer, &fill);
        TextRenderer::drawText(renderer, "SKIP", 625, 184, 0, 85, 85, 85);
    }
#endif
}

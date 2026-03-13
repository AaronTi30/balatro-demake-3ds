#include "TitleState.h"
#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include "../game/CardRenderer.h"
#include "../game/Hand.h"
#include "../game/Card.h"

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#endif

TitleState::TitleState(StateMachine* machine) {
    m_stateMachine = machine;
    m_titleText = "BALATRO DEMAKE";

    m_sampleHand.addCard({ Suit::Spades,   Rank::Ace });
    m_sampleHand.addCard({ Suit::Hearts,   Rank::King });
    m_sampleHand.addCard({ Suit::Diamonds, Rank::Queen });
    m_sampleHand.addCard({ Suit::Clubs,    Rank::Jack });
    m_sampleHand.addCard({ Suit::Hearts,   Rank::Ten });
}

void TitleState::enter() {}
void TitleState::exit() {}

void TitleState::handleInput() {
#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_A) {
        m_stateMachine->changeState(
            std::make_shared<GameplayState>(m_stateMachine));
    }
#else
    // Mouse click on PLAY button only (no Enter key — prevents carry-over from gameplay)
    int mx, my;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);
    static bool wasPressed = false;
    bool isPressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    // Only trigger on click down (not hold)
    if (isPressed && !wasPressed) {
        if (mx >= 165 && mx <= 235 && my >= 185 && my <= 215) {
            m_stateMachine->changeState(
                std::make_shared<GameplayState>(m_stateMachine));
        }
    }
    wasPressed = isPressed;
#endif
}

void TitleState::update(float dt) {}

void TitleState::renderTopScreen(Application* app) {
    // ── Card fan logo ──
    CardRenderer::drawHand(app, m_sampleHand, 200, 40);

    // ── Title text ──
#ifdef N3DS
    TextRenderer::drawText("BALATRO", 140, 120, 0.7f, 0.7f, C2D_Color32(220, 50, 50, 255));
    TextRenderer::drawText("DEMAKE", 150, 140, 0.5f, 0.5f, C2D_Color32(200, 200, 220, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    TextRenderer::drawText(renderer, "BALATRO", 155, 115, 2, 220, 50, 50);
    TextRenderer::drawText(renderer, "DEMAKE", 165, 145, 1, 200, 200, 220);
#endif

    // ── PLAY button ──
#ifdef N3DS
    C2D_DrawRectSolid(155, 175, 0.5f, 90, 30, C2D_Color32(50, 160, 70, 255));
    C2D_DrawRectSolid(158, 178, 0.5f, 84, 24, C2D_Color32(70, 190, 90, 255));
    TextRenderer::drawText("PLAY", 175, 180, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
#else
    // Shadow
    SDL_SetRenderDrawColor(renderer, 35, 110, 45, 255);
    SDL_Rect shadow = { 167, 188, 70, 28 };
    SDL_RenderFillRect(renderer, &shadow);
    // Body
    SDL_SetRenderDrawColor(renderer, 50, 160, 70, 255);
    SDL_Rect btn = { 165, 185, 70, 28 };
    SDL_RenderFillRect(renderer, &btn);
    // Highlight
    SDL_SetRenderDrawColor(renderer, 70, 190, 90, 255);
    SDL_Rect highlight = { 168, 188, 64, 22 };
    SDL_RenderFillRect(renderer, &highlight);
    // Text
    TextRenderer::drawText(renderer, "PLAY", 182, 190, 1, 255, 255, 255);
#endif
}

void TitleState::renderBottomScreen(Application* app) {
#ifdef N3DS
    TextRenderer::drawText("Press A to start", 80, 110, 0.45f, 0.45f, C2D_Color32(180, 180, 200, 255));
    TextRenderer::drawText("3DS Homebrew", 90, 200, 0.35f, 0.35f, C2D_Color32(120, 120, 140, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    TextRenderer::drawText(renderer, "Click PLAY or press Enter", 420, 110, 0, 180, 180, 200);
    TextRenderer::drawText(renderer, "PC Build", 460, 210, 0, 120, 120, 140);
#endif
}

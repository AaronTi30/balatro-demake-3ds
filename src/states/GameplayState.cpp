#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include "../game/CardRenderer.h"
#include "../game/HandEvaluator.h"
#include "../states/TitleState.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <string>

GameplayState::GameplayState(StateMachine* machine)
    : m_cursorIndex(0), m_handsRemaining(4), m_discardsRemaining(3),
      m_score(0), m_roundTarget(300), m_ante(1), m_round(1),
      m_phase(RoundPhase::Playing), m_phaseTimer(0.0f),
      m_lastHandType(HandType::HighCard), m_lastChips(0), m_lastMult(0),
      m_lastScore(0), m_showResult(false), m_resultTimer(0.0f)
{
    m_stateMachine = machine;
}

void GameplayState::enter() {
    m_ante = 1;
    m_round = 1;
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    startNewRound();
}

void GameplayState::exit() {}

void GameplayState::startNewRound() {
    m_deck.reset();
    m_hand = Hand();
    m_cursorIndex = 0;
    m_score = 0;
    m_handsRemaining = 4;
    m_discardsRemaining = 3;
    m_roundTarget = (m_ante >= 1 && m_ante <= MAX_ANTE) ? ANTE_TARGETS[m_ante - 1] : 9999;
    m_showResult = false;
    m_resultTimer = 0.0f;
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    drawToFull();
}

void GameplayState::drawToFull() {
    while (!m_hand.full() && !m_deck.empty()) {
        m_hand.addCard(m_deck.draw());
    }
}

void GameplayState::checkRoundEnd() {
    // Already won?
    if (m_score >= m_roundTarget) {
        if (m_ante >= MAX_ANTE) {
            m_phase = RoundPhase::GameWon;
        } else {
            m_phase = RoundPhase::RoundWon;
        }
        m_phaseTimer = 0.0f;
        return;
    }
    
    // Out of hands?
    if (m_handsRemaining <= 0) {
        m_phase = RoundPhase::GameOver;
        m_phaseTimer = 0.0f;
    }
}

void GameplayState::playHand() {
    if (m_phase != RoundPhase::Playing) return;
    
    auto selected = m_hand.getSelected();
    if (selected.empty() || selected.size() > 5) return;
    if (m_handsRemaining <= 0) return;

    HandResult result = HandEvaluator::evaluate(selected);
    m_lastHandType = result.type;
    m_lastChips = result.baseChips;
    m_lastMult = result.baseMult;
    m_lastScore = result.baseChips * result.baseMult;
    m_score += m_lastScore;
    m_showResult = true;
    m_resultTimer = 2.0f;

    m_hand.removeSelected();
    m_handsRemaining--;
    drawToFull();

    // Check if round ended
    checkRoundEnd();
}

void GameplayState::discardSelected() {
    if (m_phase != RoundPhase::Playing) return;
    
    auto selected = m_hand.getSelected();
    if (selected.empty() || selected.size() > 5) return;
    if (m_discardsRemaining <= 0) return;

    m_hand.removeSelected();
    m_discardsRemaining--;
    drawToFull();
}

void GameplayState::handleInput() {
#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (m_phase == RoundPhase::Playing) {
        if (kDown & KEY_LEFT) {
            if (m_cursorIndex > 0) m_cursorIndex--;
        }
        if (kDown & KEY_RIGHT) {
            if (m_cursorIndex < m_hand.size() - 1) m_cursorIndex++;
        }
        if (kDown & KEY_A) {
            m_hand.toggleSelect(m_cursorIndex);
        }
        if (kDown & KEY_X) {
            playHand();
        }
        if (kDown & KEY_Y) {
            discardSelected();
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        if (kDown & KEY_A) {
            m_ante++;
            m_round++;
            startNewRound();
        }
    }
    else if (m_phase == RoundPhase::GameOver || m_phase == RoundPhase::GameWon) {
        if (kDown & KEY_A) {
            m_stateMachine->changeState(
                std::make_shared<TitleState>(m_stateMachine));
        }
    }
#else
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    
    static int leftCD = 0, rightCD = 0, selectCD = 0, playCD = 0, discardCD = 0, confirmCD = 0;
    if (leftCD > 0) leftCD--;
    if (rightCD > 0) rightCD--;
    if (selectCD > 0) selectCD--;
    if (playCD > 0) playCD--;
    if (discardCD > 0) discardCD--;
    if (confirmCD > 0) confirmCD--;

    if (m_phase == RoundPhase::Playing) {
        if (keys[SDL_SCANCODE_LEFT] && leftCD == 0) {
            if (m_cursorIndex > 0) m_cursorIndex--;
            leftCD = 10;
        }
        if (keys[SDL_SCANCODE_RIGHT] && rightCD == 0) {
            if (m_cursorIndex < m_hand.size() - 1) m_cursorIndex++;
            rightCD = 10;
        }
        if (keys[SDL_SCANCODE_SPACE] && selectCD == 0) {
            m_hand.toggleSelect(m_cursorIndex);
            selectCD = 10;
        }
        if (keys[SDL_SCANCODE_RETURN] && playCD == 0) {
            playHand();
            playCD = 20;
        }
        if (keys[SDL_SCANCODE_D] && discardCD == 0) {
            discardSelected();
            discardCD = 20;
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        if (keys[SDL_SCANCODE_RETURN] && confirmCD == 0) {
            m_ante++;
            m_round++;
            startNewRound();
            confirmCD = 20;
        }
    }
    else if (m_phase == RoundPhase::GameOver || m_phase == RoundPhase::GameWon) {
        if (keys[SDL_SCANCODE_RETURN] && confirmCD == 0) {
            m_stateMachine->changeState(
                std::make_shared<TitleState>(m_stateMachine));
            confirmCD = 20;
        }
    }
#endif
}

void GameplayState::update(float dt) {
    if (m_hand.size() > 0 && m_cursorIndex >= m_hand.size()) {
        m_cursorIndex = m_hand.size() - 1;
    }
    
    if (m_showResult) {
        m_resultTimer -= dt;
        if (m_resultTimer <= 0.0f) {
            m_showResult = false;
        }
    }
}

// ═══════════════════════════════════════════════════════
//  RENDERING
// ═══════════════════════════════════════════════════════

void GameplayState::renderTopScreen(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
#endif

    if (m_phase == RoundPhase::Playing) {
        // ── HUD ──
#ifdef N3DS
        TextRenderer::drawText("Ante " + std::to_string(m_ante), 10, 6, 0.4f, 0.4f,
                               C2D_Color32(255, 200, 80, 255));
        TextRenderer::drawText("Hands: " + std::to_string(m_handsRemaining), 80, 6, 0.4f, 0.4f,
                               C2D_Color32(100, 220, 100, 255));
        TextRenderer::drawText("Discards: " + std::to_string(m_discardsRemaining), 160, 6, 0.4f, 0.4f,
                               C2D_Color32(240, 170, 60, 255));
        TextRenderer::drawText("Deck: " + std::to_string(m_deck.remaining()), 340, 6, 0.35f, 0.35f,
                               C2D_Color32(180, 180, 200, 255));
#else
        TextRenderer::drawText(renderer, "Ante " + std::to_string(m_ante), 10, 6, 0, 255, 200, 80);
        TextRenderer::drawText(renderer, "Hands: " + std::to_string(m_handsRemaining), 80, 6, 0, 100, 220, 100);
        TextRenderer::drawText(renderer, "Discards: " + std::to_string(m_discardsRemaining), 170, 6, 0, 240, 170, 60);
        TextRenderer::drawText(renderer, "Deck: " + std::to_string(m_deck.remaining()), 340, 6, 0, 180, 180, 200);
#endif

        // ── Hand result banner ──
        if (m_showResult) {
#ifdef N3DS
            C2D_DrawRectSolid(80, 20, 0.5f, 240, 40, C2D_Color32(25, 25, 50, 230));
            TextRenderer::drawText(handTypeName(m_lastHandType), 90, 22, 0.45f, 0.45f,
                                   C2D_Color32(255, 255, 255, 255));
            std::string scoreStr = std::to_string(m_lastChips) + " x " + std::to_string(m_lastMult) +
                                   " = " + std::to_string(m_lastScore);
            TextRenderer::drawText(scoreStr, 90, 38, 0.5f, 0.5f, C2D_Color32(255, 220, 80, 255));
#else
            SDL_SetRenderDrawColor(renderer, 25, 25, 50, 230);
            SDL_Rect bg = { 100, 20, 220, 45 };
            SDL_RenderFillRect(renderer, &bg);
            TextRenderer::drawText(renderer, handTypeName(m_lastHandType), 110, 22, 1, 255, 255, 255);
            std::string scoreStr = std::to_string(m_lastChips) + " x " + std::to_string(m_lastMult) +
                                   " = " + std::to_string(m_lastScore);
            TextRenderer::drawText(renderer, scoreStr, 110, 42, 1, 255, 220, 80);
#endif
        }

        // ── Cards ──
        CardRenderer::drawHand(app, m_hand, 200, 85, m_cursorIndex);
    }
    else if (m_phase == RoundPhase::RoundWon) {
        // ── ROUND WON ──
#ifdef N3DS
        TextRenderer::drawText("ROUND CLEAR!", 100, 40, 0.7f, 0.7f, C2D_Color32(80, 255, 120, 255));
        TextRenderer::drawText("Ante " + std::to_string(m_ante) + " complete", 110, 80, 0.5f, 0.5f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("Score: " + std::to_string(m_score) + " / " + std::to_string(m_roundTarget),
                               110, 110, 0.45f, 0.45f, C2D_Color32(255, 220, 80, 255));
        TextRenderer::drawText("Press A for next round", 100, 170, 0.4f, 0.4f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "ROUND CLEAR!", 110, 40, 2, 80, 255, 120);
        TextRenderer::drawText(renderer, "Ante " + std::to_string(m_ante) + " complete", 130, 80, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, "Score: " + std::to_string(m_score) + " / " + std::to_string(m_roundTarget),
                               130, 110, 1, 255, 220, 80);
        std::string nextStr = "Next target: " + std::to_string(ANTE_TARGETS[m_ante]);
        TextRenderer::drawText(renderer, nextStr, 130, 140, 0, 180, 180, 200);
        TextRenderer::drawText(renderer, "Press Enter for next round", 120, 190, 0, 200, 200, 220);
#endif
    }
    else if (m_phase == RoundPhase::GameOver) {
        // ── GAME OVER ──
#ifdef N3DS
        TextRenderer::drawText("GAME OVER", 110, 50, 0.7f, 0.7f, C2D_Color32(255, 80, 80, 255));
        TextRenderer::drawText("Reached Ante " + std::to_string(m_ante), 110, 100, 0.5f, 0.5f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("Score: " + std::to_string(m_score) + " / " + std::to_string(m_roundTarget),
                               110, 130, 0.45f, 0.45f, C2D_Color32(255, 180, 80, 255));
        TextRenderer::drawText("Press A to return", 110, 180, 0.4f, 0.4f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "GAME OVER", 130, 50, 2, 255, 80, 80);
        TextRenderer::drawText(renderer, "Reached Ante " + std::to_string(m_ante), 140, 100, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, "Score: " + std::to_string(m_score) + " / " + std::to_string(m_roundTarget),
                               140, 130, 1, 255, 180, 80);
        TextRenderer::drawText(renderer, "Press Enter to return", 140, 180, 0, 200, 200, 220);
#endif
    }
    else if (m_phase == RoundPhase::GameWon) {
        // ── YOU WIN ──
#ifdef N3DS
        TextRenderer::drawText("YOU WIN!", 120, 50, 0.8f, 0.8f, C2D_Color32(255, 220, 80, 255));
        TextRenderer::drawText("All 8 antes cleared!", 100, 100, 0.5f, 0.5f,
                               C2D_Color32(100, 255, 140, 255));
        TextRenderer::drawText("Press A to return", 110, 170, 0.4f, 0.4f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "YOU WIN!", 140, 50, 2, 255, 220, 80);
        TextRenderer::drawText(renderer, "All 8 antes cleared!", 120, 100, 1, 100, 255, 140);
        TextRenderer::drawText(renderer, "Press Enter to return", 140, 170, 0, 200, 200, 220);
#endif
    }
}

void GameplayState::renderBottomScreen(Application* app) {
#ifdef N3DS
    int baseX = 0;
#else
    int baseX = 400;
    SDL_Renderer* renderer = app->getRenderer();
#endif

    if (m_phase == RoundPhase::Playing) {
        // ── Score section ──
#ifdef N3DS
        TextRenderer::drawText("SCORE", baseX + 20, 10, 0.45f, 0.45f, C2D_Color32(200, 200, 220, 255));
        TextRenderer::drawText(std::to_string(m_score), baseX + 80, 8, 0.55f, 0.55f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("/ " + std::to_string(m_roundTarget), baseX + 140, 10, 0.4f, 0.4f,
                               C2D_Color32(180, 180, 200, 255));
#else
        TextRenderer::drawText(renderer, "SCORE", baseX + 20, 10, 1, 200, 200, 220);
        TextRenderer::drawText(renderer, std::to_string(m_score), baseX + 80, 8, 2, 255, 255, 255);
        TextRenderer::drawText(renderer, "/ " + std::to_string(m_roundTarget), baseX + 140, 12, 1, 180, 180, 200);
#endif

        // ── Score progress bar ──
#ifdef N3DS
        C2D_DrawRectSolid(baseX + 20, 35, 0.5f, 280, 20, C2D_Color32(40, 40, 60, 255));
        int fillW = m_roundTarget > 0 ? (m_score * 260) / m_roundTarget : 0;
        if (fillW > 260) fillW = 260;
        u32 fillColor = (m_score >= m_roundTarget)
            ? C2D_Color32(80, 220, 80, 255)
            : C2D_Color32(80, 140, 255, 255);
        if (fillW > 0) C2D_DrawRectSolid(baseX + 30, 38, 0.5f, fillW, 14, fillColor);
#else
        SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
        SDL_Rect bgBar = { baseX + 20, 35, 280, 20 };
        SDL_RenderFillRect(renderer, &bgBar);

        int fillW = m_roundTarget > 0 ? (m_score * 260) / m_roundTarget : 0;
        if (fillW > 260) fillW = 260;
        if (m_score >= m_roundTarget)
            SDL_SetRenderDrawColor(renderer, 80, 220, 80, 255);
        else
            SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
        if (fillW > 0) {
            SDL_Rect fillBar = { baseX + 30, 38, fillW, 14 };
            SDL_RenderFillRect(renderer, &fillBar);
        }
#endif

        // ── Selected cards preview ──
        auto selected = m_hand.getSelected();
        int numSelected = static_cast<int>(selected.size());
        std::string selStr = "Selected: " + std::to_string(numSelected) + "/5";

#ifdef N3DS
        TextRenderer::drawText(selStr, baseX + 20, 70, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
        if (numSelected >= 1 && numSelected <= 5) {
            HandResult preview = HandEvaluator::evaluate(selected);
            TextRenderer::drawText(handTypeName(preview.type), baseX + 20, 86, 0.45f, 0.45f,
                                   C2D_Color32(255, 255, 180, 255));
            std::string ps = std::to_string(preview.baseChips) + " x " + std::to_string(preview.baseMult);
            TextRenderer::drawText(ps, baseX + 20, 102, 0.4f, 0.4f, C2D_Color32(200, 200, 255, 255));
        }
#else
        TextRenderer::drawText(renderer, selStr, baseX + 20, 70, 0, 200, 200, 220);
        if (numSelected >= 1 && numSelected <= 5) {
            HandResult preview = HandEvaluator::evaluate(selected);
            TextRenderer::drawText(renderer, handTypeName(preview.type), baseX + 20, 86, 1, 255, 255, 180);
            std::string ps = std::to_string(preview.baseChips) + " x " + std::to_string(preview.baseMult);
            TextRenderer::drawText(renderer, ps, baseX + 20, 106, 0, 200, 200, 255);
        }
#endif

        // ── Controls ──
#ifdef N3DS
        TextRenderer::drawText("[X] Play Hand", baseX + 20, 180, 0.4f, 0.4f, C2D_Color32(100, 220, 100, 255));
        TextRenderer::drawText("[Y] Discard", baseX + 20, 196, 0.4f, 0.4f, C2D_Color32(240, 170, 60, 255));
        TextRenderer::drawText("[A] Select", baseX + 160, 180, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
        TextRenderer::drawText("[D-Pad] Move", baseX + 160, 196, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "[Enter] Play Hand", baseX + 20, 180, 0, 100, 220, 100);
        TextRenderer::drawText(renderer, "[D] Discard", baseX + 20, 196, 0, 240, 170, 60);
        TextRenderer::drawText(renderer, "[Space] Select", baseX + 160, 180, 0, 200, 200, 220);
        TextRenderer::drawText(renderer, "[Arrows] Move", baseX + 160, 196, 0, 200, 200, 220);
#endif
    }
    else {
        // Non-playing phases: show ante info on bottom screen
#ifdef N3DS
        TextRenderer::drawText("Ante " + std::to_string(m_ante) + " / " + std::to_string(MAX_ANTE),
                               baseX + 80, 80, 0.5f, 0.5f, C2D_Color32(255, 200, 80, 255));
        TextRenderer::drawText("Round " + std::to_string(m_round), baseX + 100, 110, 0.45f, 0.45f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "Ante " + std::to_string(m_ante) + " / " + std::to_string(MAX_ANTE),
                               baseX + 80, 80, 1, 255, 200, 80);
        TextRenderer::drawText(renderer, "Round " + std::to_string(m_round), baseX + 100, 110, 1, 200, 200, 220);
#endif
    }
}

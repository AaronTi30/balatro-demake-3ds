#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include "../game/CardRenderer.h"
#include "../game/HandEvaluator.h"
#include "../states/TitleState.h"
#include "../states/ShopState.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <string>
#include <utility>

namespace {

constexpr int kGameplayHandCenterX = 200;
constexpr int kGameplayHandY = 96;

std::string formatScoreLine(int chips, int mult, int score, bool scoreEquationExact) {
    if (!scoreEquationExact) {
        return "Score: " + std::to_string(score);
    }

    return std::to_string(chips) + " x " + std::to_string(mult) +
        " = " + std::to_string(score);
}

} // namespace

GameplayState::GameplayState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)), m_cursorIndex(0),
      m_phase(RoundPhase::Playing), m_phaseTimer(0.0f),
      m_lastHandType(HandType::HighCard), m_lastChips(0), m_lastMult(0),
      m_lastScore(0), m_showResult(false), m_resultTimer(0.0f), m_inputDelay(0.3f)
{
    m_stateMachine = machine;
}

void GameplayState::enter() {
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    startNewRound();
}

void GameplayState::exit() {}

void GameplayState::startNewRound() {
    m_runState->startRound();
    m_hand = Hand();
    m_cursorIndex = 0;
    m_showResult = false;
    m_resultTimer = 0.0f;
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    drawToFull();
}

void GameplayState::drawToFull() {
    while (!m_hand.full() && !m_runState->roundDeck().empty()) {
        m_hand.addCard(m_runState->roundDeck().draw());
    }
}

void GameplayState::checkRoundEnd() {
    if (m_runState->isRoundWon()) {
        if (m_runState->isRunComplete()) {
            m_phase = RoundPhase::GameWon;
        } else {
            m_phase = RoundPhase::RoundWon;
            m_runState->awardRoundWin();
        }
        m_phaseTimer = 0.0f;
        return;
    }

    if (m_runState->handsRemaining <= 0) {
        m_phase = RoundPhase::GameOver;
        m_phaseTimer = 0.0f;
    }
}

void GameplayState::playHand() {
    if (m_phase != RoundPhase::Playing) return;
    
    auto selected = m_hand.getSelected();
    if (selected.empty() || selected.size() > 5) return;
    if (m_runState->handsRemaining <= 0) return;

    HandResult result = HandEvaluator::evaluate(
        selected,
        m_runState->jokers,
        m_runState->currentBossModifier,
        m_runState->currentBlockedSuit,
        m_runState.get());
    m_lastHandType = result.detectedHand;
    m_lastChips = result.finalChips;
    m_lastMult = result.finalMult;
    m_lastScore = result.finalScore;
    m_runState->addRoundScore(m_lastScore);
    m_showResult = true;
    m_resultTimer = 2.0f;

    m_hand.removeSelected();
    m_runState->handsRemaining--;
    drawToFull();

    checkRoundEnd();
}

void GameplayState::discardSelected() {
    if (m_phase != RoundPhase::Playing) return;
    
    auto selected = m_hand.getSelected();
    if (selected.empty() || selected.size() > 5) return;
    if (m_runState->discardsRemaining <= 0) return;

    m_hand.removeSelected();
    m_runState->discardsRemaining--;
    drawToFull();
}

void GameplayState::handleInput() {
    if (m_inputDelay > 0.0f) return;

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
        
        // Touch Input for Bottom Screen
        if (kDown & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            
            // Play Button: x: 20 -> 140, y: 160 -> 210
            if (touch.px >= 20 && touch.px <= 140 && touch.py >= 160 && touch.py <= 210) {
                playHand();
            }
            // Discard Button: x: 160 -> 280, y: 160 -> 210
            else if (touch.px >= 160 && touch.px <= 280 && touch.py >= 160 && touch.py <= 210) {
                discardSelected();
            }
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        if (kDown & KEY_A) {
            if (m_runState->shouldVisitShopAfterBlindWin()) {
                m_stateMachine->changeState(std::make_shared<ShopState>(m_stateMachine, m_runState));
            }
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
        
        // Mouse Input for Bottom Screen
        int mx, my;
        uint32_t mouseState = SDL_GetMouseState(&mx, &my);
        static bool mousePressed = false;
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            if (!mousePressed) {
                mousePressed = true;
                
                // Top Screen offset is baseX = 0 (x < 400). Cards rendered at kGameplayHandCenterX, kGameplayHandY.
                if (mx < 400) {
                    int numCards = static_cast<int>(m_hand.size());
                    const auto hitLayout = CardRenderer::gameplayHandLayout();
                    const int yTop = kGameplayHandY - hitLayout.selectOffset;
                    const int yBottom = kGameplayHandY + hitLayout.cardH + hitLayout.cursorGap + hitLayout.cursorH;
                    if (my >= yTop && my <= yBottom) {
                        int cardIndex = CardRenderer::handIndexAtX(mx, kGameplayHandCenterX, numCards, hitLayout);
                        if (cardIndex >= 0 && cardIndex < numCards) {
                            m_cursorIndex = cardIndex;
                            m_hand.toggleSelect(m_cursorIndex);
                        }
                    }
                }
                
                // Bottom screen offset is baseX = 400
                // Play Button: x: 420 -> 540, y: 160 -> 210
                if (mx >= 420 && mx <= 540 && my >= 160 && my <= 210) {
                    playHand();
                }
                // Discard Button: x: 560 -> 680, y: 160 -> 210
                else if (mx >= 560 && mx <= 680 && my >= 160 && my <= 210) {
                    discardSelected();
                }
            }
        } else {
            mousePressed = false;
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        if (keys[SDL_SCANCODE_RETURN] && confirmCD == 0) {
            if (m_runState->shouldVisitShopAfterBlindWin()) {
                m_stateMachine->changeState(std::make_shared<ShopState>(m_stateMachine, m_runState));
            }
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
    if (m_inputDelay > 0.0f) {
        m_inputDelay -= dt;
    }
    
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
        TextRenderer::drawText("Ante " + std::to_string(m_runState->ante), 10, 6, 0.4f, 0.4f,
                               C2D_Color32(255, 200, 80, 255));
        TextRenderer::drawText(m_runState->currentBlindName(), 10, 24, 0.35f, 0.35f,
                               C2D_Color32(180, 180, 200, 255));
        TextRenderer::drawText("Hands: " + std::to_string(m_runState->handsRemaining), 80, 6, 0.4f, 0.4f,
                               C2D_Color32(100, 220, 100, 255));
        TextRenderer::drawText("Discards: " + std::to_string(m_runState->discardsRemaining), 160, 6, 0.4f, 0.4f,
                               C2D_Color32(240, 170, 60, 255));
        TextRenderer::drawText("Deck: " + std::to_string(m_runState->roundDeck().remaining()), 340, 6, 0.35f, 0.35f,
                               C2D_Color32(180, 180, 200, 255));
#else
        TextRenderer::drawText(renderer, "Ante " + std::to_string(m_runState->ante), 10, 6, 0, 255, 200, 80);
        TextRenderer::drawText(renderer, m_runState->currentBlindName(), 10, 24, 0, 180, 180, 200);
        TextRenderer::drawText(renderer, "Hands: " + std::to_string(m_runState->handsRemaining), 80, 6, 0, 100, 220, 100);
        TextRenderer::drawText(renderer, "Discards: " + std::to_string(m_runState->discardsRemaining), 170, 6, 0, 240, 170, 60);
        TextRenderer::drawText(renderer, "Deck: " + std::to_string(m_runState->roundDeck().remaining()), 340, 6, 0, 180, 180, 200);
#endif
        // ── Money display ──
#ifdef N3DS
        TextRenderer::drawText("$" + std::to_string(m_runState->money), 350, 42, 0.5f, 0.5f, C2D_Color32(255, 215, 0, 255));
#else
        TextRenderer::drawText(renderer, "$" + std::to_string(m_runState->money), 360, 42, 1, 255, 215, 0);
#endif

        // ── Hand result banner ──
        if (m_showResult) {
#ifdef N3DS
            C2D_DrawRectSolid(80, 20, 0.5f, 240, 40, C2D_Color32(25, 25, 50, 230));
            TextRenderer::drawText(handTypeName(m_lastHandType), 90, 22, 0.45f, 0.45f,
                                   C2D_Color32(255, 255, 255, 255));
            std::string scoreStr = formatScoreLine(
                m_lastChips,
                m_lastMult,
                m_lastScore,
                m_lastScore == m_lastChips * m_lastMult);
            TextRenderer::drawText(scoreStr, 90, 38, 0.5f, 0.5f, C2D_Color32(255, 220, 80, 255));
#else
            SDL_SetRenderDrawColor(renderer, 25, 25, 50, 230);
            SDL_Rect bg = { 100, 20, 220, 45 };
            SDL_RenderFillRect(renderer, &bg);
            TextRenderer::drawText(renderer, handTypeName(m_lastHandType), 110, 22, 1, 255, 255, 255);
            std::string scoreStr = formatScoreLine(
                m_lastChips,
                m_lastMult,
                m_lastScore,
                m_lastScore == m_lastChips * m_lastMult);
            TextRenderer::drawText(renderer, scoreStr, 110, 42, 1, 255, 220, 80);
#endif
        }

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            const std::string bossLabel = "Boss: " + std::string(RunState::bossModifierName(m_runState->currentBossModifier));
            const char* bossDescription = RunState::bossModifierDescription(
                m_runState->currentBossModifier,
                m_runState->currentBlockedSuit);
#ifdef N3DS
            TextRenderer::drawText(bossLabel, 10, 42, 0.32f, 0.32f, C2D_Color32(255, 170, 120, 255));
            TextRenderer::drawText(bossDescription, 10, 56, 0.28f, 0.28f, C2D_Color32(220, 220, 220, 255));
#else
            TextRenderer::drawText(renderer, bossLabel, 10, 42, 0, 255, 170, 120);
            TextRenderer::drawText(renderer, bossDescription, 10, 58, 0, 220, 220, 220);
#endif
        }

        // ── Cards ──
        const auto layout = CardRenderer::gameplayHandLayout();
        CardRenderer::drawHand(app, m_hand, kGameplayHandCenterX, kGameplayHandY, m_cursorIndex, layout);

        // ── Jokers ──
        int numJokers = static_cast<int>(m_runState->jokers.size());
        if (numJokers > 0) {
            // Draw Jokers at the top of the screen
            int startX = 200 - (numJokers * 30) / 2; // Assuming ~30px per joker box
            
#ifdef N3DS
            for (int i = 0; i < numJokers; ++i) {
                int jx = startX + i * 32;
                int jy = 25;
                u32 color = C2D_Color32(100, 100, 100, 255);
                if (m_runState->jokers[i].effectType == JokerEffectType::AddChips) color = C2D_Color32(80, 120, 220, 255);
                else if (m_runState->jokers[i].effectType == JokerEffectType::AddMult) color = C2D_Color32(220, 60, 60, 255);
                else if (m_runState->jokers[i].effectType == JokerEffectType::MulMult) color = C2D_Color32(180, 60, 220, 255);

                C2D_DrawRectSolid(jx, jy, 0.5f, 30, 45, color);
                C2D_DrawRectSolid(jx, jy, 0.5f, 30, 1, C2D_Color32(255, 255, 255, 255));
                C2D_DrawRectSolid(jx, jy + 44, 0.5f, 30, 1, C2D_Color32(255, 255, 255, 255));
                C2D_DrawRectSolid(jx, jy, 0.5f, 1, 45, C2D_Color32(255, 255, 255, 255));
                C2D_DrawRectSolid(jx + 29, jy, 0.5f, 1, 45, C2D_Color32(255, 255, 255, 255));
                
                TextRenderer::drawText(m_runState->jokers[i].description, jx + 2, jy + 18, 0.3f, 0.3f, C2D_Color32(255, 255, 255, 255));
            }
#else
            for (int i = 0; i < numJokers; ++i) {
                int jx = startX + i * 35;
                int jy = 25;
                
                int r = 100, g = 100, b = 100;
                if (m_runState->jokers[i].effectType == JokerEffectType::AddChips) { r = 80; g = 120; b = 220; }
                else if (m_runState->jokers[i].effectType == JokerEffectType::AddMult) { r = 220; g = 60; b = 60; }
                else if (m_runState->jokers[i].effectType == JokerEffectType::MulMult) { r = 180; g = 60; b = 220; }

                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_Rect jbox = { jx, jy, 30, 45 };
                SDL_RenderFillRect(renderer, &jbox);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &jbox);

                TextRenderer::drawText(renderer, m_runState->jokers[i].description, jx + 2, jy + 18, 0, 255, 255, 255);
            }
#endif
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        // ── ROUND WON ──
        const BlindStage upcomingBlind = m_runState->nextBlindStage();
        const int upcomingAnte = m_runState->nextBlindAnte();
        const std::string nextBlindLabel = std::string(RunState::blindStageName(upcomingBlind));
        const std::string nextTargetLabel = "Next: Ante " + std::to_string(upcomingAnte) + " " + nextBlindLabel +
            " (" + std::to_string(RunState::targetForBlind(upcomingAnte, upcomingBlind)) + ")";
        const bool previewBoss = upcomingBlind == BlindStage::Boss &&
            m_runState->nextBossModifier != BossBlindModifier::None;
        const std::string bossLabel = "Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier));
        const char* bossDescription = RunState::bossModifierDescription(
            m_runState->nextBossModifier,
            m_runState->nextBlockedSuit);
#ifdef N3DS
        TextRenderer::drawText("BLIND CLEAR!", 100, 40, 0.7f, 0.7f, C2D_Color32(80, 255, 120, 255));
        TextRenderer::drawText(std::string(m_runState->currentBlindName()) + " cleared", 100, 80, 0.5f, 0.5f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
                               110, 110, 0.45f, 0.45f, C2D_Color32(255, 220, 80, 255));
        TextRenderer::drawText("Reward: $" + std::to_string(m_runState->currentBlindReward()),
                               80, 136, 0.4f, 0.4f, C2D_Color32(255, 215, 0, 255));
        TextRenderer::drawText(nextTargetLabel, 60, 154, 0.35f, 0.35f, C2D_Color32(180, 180, 200, 255));
        if (previewBoss) {
            TextRenderer::drawText(bossLabel, 70, 174, 0.35f, 0.35f, C2D_Color32(255, 170, 120, 255));
            TextRenderer::drawText(bossDescription, 46, 190, 0.3f, 0.3f, C2D_Color32(220, 220, 220, 255));
        }
        TextRenderer::drawText("Press A to enter Shop", 100, previewBoss ? 210 : 185, 0.4f, 0.4f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "BLIND CLEAR!", 110, 40, 2, 80, 255, 120);
        TextRenderer::drawText(renderer, std::string(m_runState->currentBlindName()) + " cleared", 120, 80, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, "Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
                               130, 110, 1, 255, 220, 80);
        TextRenderer::drawText(renderer, "Reward: $" + std::to_string(m_runState->currentBlindReward()),
                               130, 136, 0, 255, 215, 0);
        TextRenderer::drawText(renderer, nextTargetLabel, 100, 154, 0, 180, 180, 200);
        if (previewBoss) {
            TextRenderer::drawText(renderer, bossLabel, 110, 174, 0, 255, 170, 120);
            TextRenderer::drawText(renderer, bossDescription, 80, 192, 0, 220, 220, 220);
        }
        TextRenderer::drawText(renderer, "Press Enter to enter Shop", 105, previewBoss ? 214 : 190, 0, 200, 200, 220);
#endif
    }
    else if (m_phase == RoundPhase::GameOver) {
        // ── GAME OVER ──
#ifdef N3DS
        TextRenderer::drawText("GAME OVER", 110, 50, 0.7f, 0.7f, C2D_Color32(255, 80, 80, 255));
        TextRenderer::drawText("Reached Ante " + std::to_string(m_runState->ante) + " " + m_runState->currentBlindName(), 70, 100, 0.45f, 0.45f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
                               110, 130, 0.45f, 0.45f, C2D_Color32(255, 180, 80, 255));
        TextRenderer::drawText("Press A to return", 110, 180, 0.4f, 0.4f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "GAME OVER", 130, 50, 2, 255, 80, 80);
        TextRenderer::drawText(renderer, "Reached Ante " + std::to_string(m_runState->ante) + " " + m_runState->currentBlindName(), 95, 100, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, "Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
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
        TextRenderer::drawText(std::to_string(m_runState->roundScore), baseX + 80, 8, 0.55f, 0.55f,
                               C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("/ " + std::to_string(m_runState->roundTarget), baseX + 140, 10, 0.4f, 0.4f,
                               C2D_Color32(180, 180, 200, 255));
#else
        TextRenderer::drawText(renderer, "SCORE", baseX + 20, 10, 1, 200, 200, 220);
        TextRenderer::drawText(renderer, std::to_string(m_runState->roundScore), baseX + 80, 8, 2, 255, 255, 255);
        TextRenderer::drawText(renderer, "/ " + std::to_string(m_runState->roundTarget), baseX + 140, 12, 1, 180, 180, 200);
#endif

        // ── Score progress bar ──
#ifdef N3DS
        C2D_DrawRectSolid(baseX + 20, 35, 0.5f, 280, 20, C2D_Color32(40, 40, 60, 255));
        int fillW = m_runState->roundTarget > 0 ? (m_runState->roundScore * 260) / m_runState->roundTarget : 0;
        if (fillW > 260) fillW = 260;
        u32 fillColor = m_runState->isRoundWon()
            ? C2D_Color32(80, 220, 80, 255)
            : C2D_Color32(80, 140, 255, 255);
        if (fillW > 0) C2D_DrawRectSolid(baseX + 30, 38, 0.5f, fillW, 14, fillColor);
#else
        SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
        SDL_Rect bgBar = { baseX + 20, 35, 280, 20 };
        SDL_RenderFillRect(renderer, &bgBar);

        int fillW = m_runState->roundTarget > 0 ? (m_runState->roundScore * 260) / m_runState->roundTarget : 0;
        if (fillW > 260) fillW = 260;
        if (m_runState->isRoundWon())
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
            HandResult preview = HandEvaluator::evaluate(
                selected,
                m_runState->jokers,
                m_runState->currentBossModifier,
                m_runState->currentBlockedSuit,
                m_runState.get());
            TextRenderer::drawText(handTypeName(preview.detectedHand), baseX + 20, 86, 0.45f, 0.45f,
                                   C2D_Color32(255, 255, 180, 255));
            std::string ps = formatScoreLine(
                preview.finalChips,
                preview.finalMult,
                preview.finalScore,
                preview.scoreEquationExact);
            TextRenderer::drawText(ps, baseX + 20, 102, 0.4f, 0.4f, C2D_Color32(200, 200, 255, 255));
        }
#else
        TextRenderer::drawText(renderer, selStr, baseX + 20, 70, 0, 200, 200, 220);
        if (numSelected >= 1 && numSelected <= 5) {
            HandResult preview = HandEvaluator::evaluate(
                selected,
                m_runState->jokers,
                m_runState->currentBossModifier,
                m_runState->currentBlockedSuit,
                m_runState.get());
            TextRenderer::drawText(renderer, handTypeName(preview.detectedHand), baseX + 20, 86, 1, 255, 255, 180);
            std::string ps = formatScoreLine(
                preview.finalChips,
                preview.finalMult,
                preview.finalScore,
                preview.scoreEquationExact);
            TextRenderer::drawText(renderer, ps, baseX + 20, 106, 0, 200, 200, 255);
        }
#endif

        // ── Controls ──
#ifdef N3DS
        // Play Button (Green)
        C2D_DrawRectSolid(baseX + 20, 160, 0.5f, 120, 50, C2D_Color32(80, 200, 80, 255));
        C2D_DrawRectSolid(baseX + 20, 160, 0.5f, 120, 2, C2D_Color32(120, 240, 120, 255)); // highlight
        TextRenderer::drawText("Play Hand", baseX + 35, 175, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255));
        
        // Discard Button (Orange)
        C2D_DrawRectSolid(baseX + 160, 160, 0.5f, 120, 50, C2D_Color32(200, 100, 40, 255));
        C2D_DrawRectSolid(baseX + 160, 160, 0.5f, 120, 2, C2D_Color32(240, 140, 80, 255)); // highlight
        TextRenderer::drawText("Discard", baseX + 185, 175, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255));
        
        // Hints
        TextRenderer::drawText("[X]", baseX + 75, 215, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
        TextRenderer::drawText("[Y]", baseX + 215, 215, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
#else
        // Play Button (Green)
        SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
        SDL_Rect playRect = { baseX + 20, 160, 120, 50 };
        SDL_RenderFillRect(renderer, &playRect);
        TextRenderer::drawText(renderer, "Play Hand", baseX + 35, 175, 1, 0, 0, 0);

        // Discard Button (Orange)
        SDL_SetRenderDrawColor(renderer, 200, 100, 40, 255);
        SDL_Rect discRect = { baseX + 160, 160, 120, 50 };
        SDL_RenderFillRect(renderer, &discRect);
        TextRenderer::drawText(renderer, "Discard", baseX + 185, 175, 1, 0, 0, 0);
        
        // Hints
        TextRenderer::drawText(renderer, "[Enter]", baseX + 65, 215, 0, 200, 200, 220);
        TextRenderer::drawText(renderer, "[D]", baseX + 210, 215, 0, 200, 200, 220);
#endif
    }
    else {
        // Non-playing phases: show ante info on bottom screen
#ifdef N3DS
        TextRenderer::drawText("Ante " + std::to_string(m_runState->ante) + " / " + std::to_string(RunState::kMaxAnte),
                               baseX + 80, 70, 0.5f, 0.5f, C2D_Color32(255, 200, 80, 255));
        TextRenderer::drawText(m_runState->currentBlindName(), baseX + 100, 95, 0.45f, 0.45f,
                               C2D_Color32(200, 200, 220, 255));
        TextRenderer::drawText("Target " + std::to_string(m_runState->roundTarget), baseX + 90, 110, 0.45f, 0.45f,
                               C2D_Color32(200, 200, 220, 255));
#else
        TextRenderer::drawText(renderer, "Ante " + std::to_string(m_runState->ante) + " / " + std::to_string(RunState::kMaxAnte),
                               baseX + 80, 70, 1, 255, 200, 80);
        TextRenderer::drawText(renderer, m_runState->currentBlindName(), baseX + 105, 95, 1, 200, 200, 220);
        TextRenderer::drawText(renderer, "Target " + std::to_string(m_runState->roundTarget), baseX + 90, 120, 1, 200, 200, 220);
#endif
    }
}

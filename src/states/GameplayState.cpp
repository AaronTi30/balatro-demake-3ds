#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
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

constexpr int kBottomScreenBaseXDesktop = 400;

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
    u32 kDown = hidKeysDown();
    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const auto bottomLayout = gameplay_state_helpers::compactBottomScreenLayout();
    
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
            const auto playButton = gameplay_state_helpers::bottomPlayButtonRect(bottomLayout);
            const auto discardButton = gameplay_state_helpers::bottomDiscardButtonRect(bottomLayout);

            if (gameplay_state_helpers::pointInRect(touch.px, touch.py, playButton)) {
                playHand();
            }
            else if (gameplay_state_helpers::pointInRect(touch.px, touch.py, discardButton)) {
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
    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const auto bottomLayout = gameplay_state_helpers::compactBottomScreenLayout();
    
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
                    int cardIndex = gameplay_state_helpers::gameplayHandIndexAtPoint(
                        mx, my, numCards, topLayout, hitLayout);
                    if (cardIndex >= 0 && cardIndex < numCards) {
                        m_cursorIndex = cardIndex;
                        m_hand.toggleSelect(m_cursorIndex);
                    }
                }
                const auto playButton = gameplay_state_helpers::bottomPlayButtonRect(bottomLayout, kBottomScreenBaseXDesktop);
                const auto discardButton = gameplay_state_helpers::bottomDiscardButtonRect(bottomLayout, kBottomScreenBaseXDesktop);

                if (gameplay_state_helpers::pointInRect(mx, my, playButton)) {
                    playHand();
                }
                else if (gameplay_state_helpers::pointInRect(mx, my, discardButton)) {
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

void GameplayState::renderTopScreen(Application* app, ScreenRenderer& r) {
    if (m_phase == RoundPhase::Playing) {
        const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();

        // ── HUD ──
        r.drawText("Ante " + std::to_string(m_runState->ante), topLayout.anteX, topLayout.anteY, 0.4f, 255, 200, 80);
        r.drawText(m_runState->currentBlindName(), topLayout.blindX, topLayout.blindY, 0.35f, 180, 180, 200);
        r.drawText("$" + std::to_string(m_runState->money), topLayout.moneyX, topLayout.moneyY, 0.5f, 255, 215, 0);

        // ── Hand result banner ──
        if (m_showResult) {
            r.fillRect(topLayout.resultBannerX, topLayout.resultBannerY,
                       topLayout.resultBannerW, topLayout.resultBannerH,
                       25, 25, 50, 230);
            r.drawText(handTypeName(m_lastHandType),
                       topLayout.resultBannerX + 10, topLayout.resultBannerY + 2,
                       0.35f, 255, 255, 255);
            r.drawText(formatScoreLine(m_lastChips, m_lastMult, m_lastScore,
                                       m_lastScore == m_lastChips * m_lastMult),
                       topLayout.resultBannerX + 10, topLayout.resultBannerY + 11,
                       0.38f, 255, 220, 80);
        }

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            r.drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->currentBossModifier)),
                       topLayout.blindX, topLayout.bossLabelY, 0.32f, 255, 170, 120);
        }

        // ── Stage area (idle) ──
        r.drawRectOutline(topLayout.idleStagePanelRect.x, topLayout.idleStagePanelRect.y,
                          topLayout.idleStagePanelRect.w, topLayout.idleStagePanelRect.h,
                          gameplay_state_helpers::kIdleStagePanelColorR,
                          gameplay_state_helpers::kIdleStagePanelColorG,
                          gameplay_state_helpers::kIdleStagePanelColorB);
        r.drawText("play a hand", topLayout.idleStagePromptX, topLayout.idleStagePromptY,
                   0.32f,
                   gameplay_state_helpers::kIdleStagePanelColorR,
                   gameplay_state_helpers::kIdleStagePanelColorG,
                   gameplay_state_helpers::kIdleStagePanelColorB);

        // ── Cards ──
        const auto layout = CardRenderer::gameplayHandLayout();
        CardRenderer::drawHand(app, m_hand, topLayout.handCenterX, topLayout.handY, m_cursorIndex, layout);

        // ── Jokers ──
        const int numJokers = static_cast<int>(m_runState->jokers.size());
        if (numJokers > 0) {
            const int jokerStartX = gameplay_state_helpers::jokerStripStartX(numJokers, topLayout);
            for (int i = 0; i < numJokers; ++i) {
                const int jx = jokerStartX + i * topLayout.jokerSpacing;
                const int jy = topLayout.jokerStripY;

                uint8_t jr = 100, jg = 100, jb = 100;
                if (m_runState->jokers[i].effectType == JokerEffectType::AddChips)  { jr =  80; jg = 120; jb = 220; }
                else if (m_runState->jokers[i].effectType == JokerEffectType::AddMult)  { jr = 220; jg =  60; jb =  60; }
                else if (m_runState->jokers[i].effectType == JokerEffectType::MulMult)  { jr = 180; jg =  60; jb = 220; }

                r.fillRect(jx, jy, topLayout.jokerBoxW, topLayout.jokerBoxH, jr, jg, jb);
                r.drawRectOutline(jx, jy, topLayout.jokerBoxW, topLayout.jokerBoxH, 255, 255, 255);
                r.drawText(gameplay_state_helpers::compactJokerLabel(m_runState->jokers[i].name),
                           jx + 2, jy + 12, 0.3f, 255, 255, 255);
            }
        }
    }
    else if (m_phase == RoundPhase::RoundWon) {
        // ── ROUND WON ──
        const BlindStage upcomingBlind = m_runState->nextBlindStage();
        const int upcomingAnte = m_runState->nextBlindAnte();
        const std::string nextTargetLabel = "Next: Ante " + std::to_string(upcomingAnte) +
            " " + std::string(RunState::blindStageName(upcomingBlind)) +
            " (" + std::to_string(RunState::targetForBlind(upcomingAnte, upcomingBlind)) + ")";
        const bool previewBoss = upcomingBlind == BlindStage::Boss &&
            m_runState->nextBossModifier != BossBlindModifier::None;
        const char* bossDescription = RunState::bossModifierDescription(
            m_runState->nextBossModifier, m_runState->nextBlockedSuit);

        r.drawText("BLIND CLEAR!", 100, 40, 0.7f, 80, 255, 120);
        r.drawText(std::string(m_runState->currentBlindName()) + " cleared", 100, 80, 0.5f, 255, 255, 255);
        r.drawText("Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
                   110, 110, 0.45f, 255, 220, 80);
        r.drawText("Reward: $" + std::to_string(m_runState->currentBlindReward()), 80, 136, 0.4f, 255, 215, 0);
        r.drawText(nextTargetLabel, 60, 154, 0.35f, 180, 180, 200);
        if (previewBoss) {
            r.drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                       70, 174, 0.35f, 255, 170, 120);
            r.drawText(bossDescription, 46, 190, 0.3f, 220, 220, 220);
        }
        r.drawText("Press A / Enter to enter Shop", 72, previewBoss ? 210 : 185, 0.38f, 200, 200, 220);
    }
    else if (m_phase == RoundPhase::GameOver) {
        // ── GAME OVER ──
        r.drawText("GAME OVER", 110, 50, 0.7f, 255, 80, 80);
        r.drawText("Reached Ante " + std::to_string(m_runState->ante) + " " + m_runState->currentBlindName(),
                   70, 100, 0.45f, 255, 255, 255);
        r.drawText("Score: " + std::to_string(m_runState->roundScore) + " / " + std::to_string(m_runState->roundTarget),
                   110, 130, 0.45f, 255, 180, 80);
        r.drawText("Press A / Enter to return", 88, 180, 0.4f, 200, 200, 220);
    }
    else if (m_phase == RoundPhase::GameWon) {
        // ── YOU WIN ──
        r.drawText("YOU WIN!", 120, 50, 0.8f, 255, 220, 80);
        r.drawText("All 8 antes cleared!", 100, 100, 0.5f, 100, 255, 140);
        r.drawText("Press A / Enter to return", 88, 170, 0.4f, 200, 200, 220);
    }
}

void GameplayState::renderBottomScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    if (m_phase == RoundPhase::Playing) {
        const auto bottomLayout = gameplay_state_helpers::compactBottomScreenLayout();

        // ── Score section ──
        r.drawText("SCORE", bottomLayout.scoreHeaderX, bottomLayout.scoreHeaderY, 0.45f, 200, 200, 220);
        r.drawText(std::to_string(m_runState->roundScore), bottomLayout.scoreValueX, bottomLayout.scoreValueY, 0.55f, 255, 255, 255);
        r.drawText("/ " + std::to_string(m_runState->roundTarget), bottomLayout.scoreTargetX, bottomLayout.scoreTargetY, 0.4f, 180, 180, 200);

        // ── Score progress bar ──
        r.fillRect(bottomLayout.progressBarX, bottomLayout.progressBarY,
                   bottomLayout.progressBarW, bottomLayout.progressBarH, 40, 40, 60);
        int fillW = m_runState->roundTarget > 0 ? (m_runState->roundScore * 260) / m_runState->roundTarget : 0;
        if (fillW > 260) fillW = 260;
        if (fillW > 0) {
            const bool won = m_runState->isRoundWon();
            r.fillRect(bottomLayout.progressBarX + 10, bottomLayout.progressBarY + 3,
                       fillW, 14,
                       won ? 80 : 80, won ? 220 : 140, won ? 80 : 255);
        }

        // ── Status and hand preview ──
        auto selected = m_hand.getSelected();
        const int numSelected = static_cast<int>(selected.size());

        r.drawText(gameplay_state_helpers::compactStatusLine(
                       m_runState->handsRemaining,
                       m_runState->discardsRemaining,
                       m_runState->roundDeck().remaining()),
                   bottomLayout.scoreHeaderX, bottomLayout.statusRowY, 0.34f, 200, 200, 220);
        r.drawText("Selected: " + std::to_string(numSelected) + "/5",
                   bottomLayout.scoreHeaderX, bottomLayout.previewLabelY, 0.4f, 200, 200, 220);

        if (numSelected >= 1 && numSelected <= 5) {
            HandResult preview = HandEvaluator::evaluate(
                selected,
                m_runState->jokers,
                m_runState->currentBossModifier,
                m_runState->currentBlockedSuit,
                m_runState.get());
            r.drawText(handTypeName(preview.detectedHand),
                       bottomLayout.scoreHeaderX, bottomLayout.previewTypeY, 0.45f, 255, 255, 180);
            r.drawText(formatScoreLine(preview.finalChips, preview.finalMult, preview.finalScore, preview.scoreEquationExact),
                       bottomLayout.scoreHeaderX, bottomLayout.previewScoreY, 0.4f, 200, 200, 255);
        }

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            r.drawText(RunState::bossModifierDescription(m_runState->currentBossModifier, m_runState->currentBlockedSuit),
                       bottomLayout.scoreHeaderX, bottomLayout.bossDescriptionY, 0.31f, 220, 220, 220);
        }

        // ── Play / Discard buttons ──
        r.fillRect(bottomLayout.buttonX, bottomLayout.buttonY, bottomLayout.buttonW, bottomLayout.buttonH, 80, 200, 80);
        r.fillRect(bottomLayout.buttonX, bottomLayout.buttonY, bottomLayout.buttonW, 2, 120, 240, 120);
        r.drawText("Play Hand", bottomLayout.buttonX + 15, bottomLayout.buttonY + 15, 0.5f, 0, 0, 0);

        const int discardX = bottomLayout.buttonX + bottomLayout.buttonW + bottomLayout.buttonGap;
        r.fillRect(discardX, bottomLayout.buttonY, bottomLayout.buttonW, bottomLayout.buttonH, 200, 100, 40);
        r.fillRect(discardX, bottomLayout.buttonY, bottomLayout.buttonW, 2, 240, 140, 80);
        r.drawText("Discard", discardX + 25, bottomLayout.buttonY + 15, 0.5f, 0, 0, 0);
    }
    else {
        // Non-playing phases: show ante info on bottom screen
        r.drawText("Ante " + std::to_string(m_runState->ante) + " / " + std::to_string(RunState::kMaxAnte),
                   80, 70, 0.5f, 255, 200, 80);
        r.drawText(m_runState->currentBlindName(), 100, 95, 0.45f, 200, 200, 220);
        r.drawText("Target " + std::to_string(m_runState->roundTarget), 90, 110, 0.45f, 200, 200, 220);
    }
}

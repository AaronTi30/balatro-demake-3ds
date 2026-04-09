#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
#include "../core/TextRenderer.h"
#include "../game/CardRenderer.h"
#include "../game/HandEvaluator.h"
#include "../states/GameplayLayout.h"
#include "../states/GameplayHudStyle.h"
#include "../game/ScoringAnimator.h"
#include "../states/TitleState.h"
#include "../states/ShopState.h"

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace {

constexpr int kBottomScreenBaseXDesktop = 400;
constexpr int kScoringStageY = 100;
constexpr int kHudOuterX = 12;
constexpr int kProgressFillInsetX = 2;
constexpr int kProgressFillInsetY = 1;
constexpr int kProgressFillHeight = 4;

struct CompactBottomScreenLayout {
    GameplayRect blindCell;
    GameplayRect roundScoreCell;
    GameplayRect targetCell;
    GameplayRect moneyCell;
    GameplayRect progressTrack;
    GameplayRect selectedCell;
    GameplayRect handTypeCell;
    GameplayRect chipsCell;
    GameplayRect multCell;
    GameplayRect handsCell;
    GameplayRect discardsCell;
    GameplayRect anteCell;
    GameplayRect roundCell;
    int bossDescriptionY;
};

CompactBottomScreenLayout compactBottomScreenLayout() {
    return {
        {12, 8, 106, 26},
        {124, 8, 58, 26},
        {188, 8, 66, 26},
        {260, 8, 48, 26},
        {12, 38, 296, 6},
        {12, 52, 72, 24},
        {90, 52, 218, 24},
        {12, 82, 144, 30},
        {164, 82, 144, 30},
        {12, 120, 68, 24},
        {88, 120, 68, 24},
        {164, 120, 68, 24},
        {240, 120, 68, 24},
        152
    };
}

std::string formatScoreLine(int chips, int mult, int score, bool scoreEquationExact) {
    if (!scoreEquationExact) {
        return "Score: " + std::to_string(score);
    }

    return std::to_string(chips) + " x " + std::to_string(mult) +
        " = " + std::to_string(score);
}

const char* gameplaySortModeLabel(GameplaySortMode mode) {
    switch (mode) {
        case GameplaySortMode::Suit:
            return "SUIT";
        case GameplaySortMode::Rank:
        default:
            return "RANK";
    }
}

int blindStageRoundNumber(BlindStage stage) {
    switch (stage) {
        case BlindStage::Big:
            return 2;
        case BlindStage::Boss:
            return 3;
        case BlindStage::Small:
        default:
            return 1;
    }
}

float hudValueScale(const std::string& value, int width) {
    if (width <= 58) {
        return value.size() > 2 ? 0.30f : 0.42f;
    }
    if (width <= 72) {
        return value.size() > 5 ? 0.28f : 0.40f;
    }
    if (width <= 110) {
        return value.size() > 10 ? 0.24f : 0.34f;
    }
    if (value.size() > 16) {
        return 0.24f;
    }
    if (value.size() > 10) {
        return 0.28f;
    }
    return 0.34f;
}

void drawHudCell(ScreenRenderer& r,
                 const GameplayRect& rect,
                 const std::string& label,
                 const std::string& value,
                 uint8_t valueR = 255,
                 uint8_t valueG = 255,
                 uint8_t valueB = 255) {
    const GameplayHudColor shell = gameplayHudNeutralShellColor();
    const GameplayHudColor inset = gameplayHudNeutralInsetColor();
    const GameplayHudColor outline = gameplayHudOutlineColor();
    // Outer dark shell
    r.fillRect(rect.x, rect.y, rect.w, rect.h, shell.r, shell.g, shell.b);
    if (rect.h >= 22) {
        // Taller cells: inset value region below a label strip
        const int insetTop = rect.y + (rect.h >= 24 ? 11 : 9);
        const int insetH = rect.y + rect.h - insetTop - 1;
        if (insetH > 0) {
            r.fillRect(rect.x + 1, insetTop, rect.w - 2, insetH, inset.r, inset.g, inset.b);
        }
        r.drawRectOutline(rect.x, rect.y, rect.w, rect.h, outline.r, outline.g, outline.b);
        r.drawText(label, rect.x + 4, rect.y + 2, 0.21f, outline.r, outline.g, outline.b);
        r.drawText(value, rect.x + 4, insetTop + 1, hudValueScale(value, rect.w), valueR, valueG, valueB);
    } else {
        // Short cells (20px): fontSmall is 10px — two stacked lines don't fit.
        // Use flat fill with the value centered; color carries the identification.
        r.drawRectOutline(rect.x, rect.y, rect.w, rect.h, outline.r, outline.g, outline.b);
        r.drawText(value, rect.x + 4, rect.y + 5, hudValueScale(value, rect.w), valueR, valueG, valueB);
    }
}

void drawPrimaryAction(ScreenRenderer& r,
                       const GameplayRect& rect,
                       GameplayHudAction action,
                       const char* label,
                       bool enabled) {
    const GameplayHudActionStyle style = gameplayPrimaryActionStyle(action, enabled);
    const GameplayHudColor shell = gameplayHudNeutralShellColor();
    // Shadow rect (offset 2px right + down for emboss feel)
    r.fillRect(rect.x + 2, rect.y + 2, rect.w, rect.h,
               style.shadow.r, style.shadow.g, style.shadow.b);
    // Button body
    r.fillRect(rect.x, rect.y, rect.w, rect.h,
               style.fill.r, style.fill.g, style.fill.b);
    // Dark shell outline
    r.drawRectOutline(rect.x, rect.y, rect.w, rect.h, shell.r, shell.g, shell.b);
    // Centered label
    const std::string labelText(label);
    const float scale = labelText.size() > 6 ? 0.36f : 0.42f;
    const int textW = static_cast<int>(labelText.size() * scale * 8.0f);
    r.drawText(labelText,
               rect.x + (rect.w - textW) / 2,
               rect.y + 10,
               scale,
               style.text.r, style.text.g, style.text.b);
}

void drawProgressTrack(ScreenRenderer& r,
                       const GameplayRect& track,
                       int score,
                       int target) {
    const GameplayHudColor shell = gameplayHudNeutralShellColor();
    const GameplayHudColor inset = gameplayHudNeutralInsetColor();
    const GameplayHudColor outline = gameplayHudOutlineColor();
    const GameplayHudColor chips = gameplayHudChipsColor();
    r.fillRect(track.x, track.y, track.w, track.h, shell.r, shell.g, shell.b);
    r.fillRect(track.x + 1, track.y + 1, track.w - 2, track.h - 2, inset.r, inset.g, inset.b);
    const int fillW = roundProgressFillWidth(score, target, track.w - kProgressFillInsetX * 2);
    if (fillW > 0) {
        r.fillRect(track.x + kProgressFillInsetX, track.y + kProgressFillInsetY,
                   fillW, kProgressFillHeight, chips.r, chips.g, chips.b);
    }
    r.drawRectOutline(track.x, track.y, track.w, track.h, outline.r, outline.g, outline.b);
}

// Balatro-style scoring value block: full accent-color fill with white label and value.
// Used for chips and mult cells so they dominate the scoring cluster visually.
void drawScoringValueCell(ScreenRenderer& r,
                          const GameplayRect& rect,
                          const std::string& label,
                          const std::string& value,
                          const GameplayHudColor& accent) {
    const GameplayHudColor shell = gameplayHudNeutralShellColor();
    const GameplayHudColor shadow{
        accent.r * 3 / 4,
        accent.g * 3 / 4,
        accent.b * 3 / 4
    };
    // Shadow for depth
    r.fillRect(rect.x + 2, rect.y + 2, rect.w, rect.h, shadow.r, shadow.g, shadow.b);
    // Saturated accent fill — the full block carries the color identity
    r.fillRect(rect.x, rect.y, rect.w, rect.h, accent.r, accent.g, accent.b);
    // Dark shell outline keeps the block from bleeding visually
    r.drawRectOutline(rect.x, rect.y, rect.w, rect.h, shell.r, shell.g, shell.b);
    // White label and value so they read clearly on the saturated background
    r.drawText(label, rect.x + 4, rect.y + 2, 0.21f, 255, 255, 255);
    r.drawText(value, rect.x + 4, rect.y + 12, hudValueScale(value, rect.w), 255, 255, 255);
}

// Balatro-style sort utility widget: dark shell + orange accent label + integrated mode text.
// Sort is a utility control, not a primary action, so it does not get a saturated fill.
void drawSortUtility(ScreenRenderer& r,
                     const GameplayRect& rect,
                     const char* modeLabel,
                     bool enabled) {
    const GameplayHudColor shell = gameplayHudNeutralShellColor();
    const GameplayHudColor inset = gameplayHudNeutralInsetColor();
    const GameplayHudColor outline = gameplayHudOutlineColor();
    const GameplayHudColor accent = enabled ? gameplayHudSortAccentColor() : outline;
    // Dark outer shell
    r.fillRect(rect.x, rect.y, rect.w, rect.h, shell.r, shell.g, shell.b);
    // Slightly lighter inset for the mode text area
    r.fillRect(rect.x + 1, rect.y + 18, rect.w - 2, rect.h - 19, inset.r, inset.g, inset.b);
    // Orange (or muted) outline signals utility interactivity
    r.drawRectOutline(rect.x, rect.y, rect.w, rect.h, accent.r, accent.g, accent.b);
    // "SORT" in accent color as the primary affordance label
    r.drawText("SORT", rect.x + 5, rect.y + 4, 0.28f, accent.r, accent.g, accent.b);
    // Mode text (RANK / SUIT) in lighter outline color below the divider
    r.drawText(modeLabel, rect.x + 5, rect.y + 20, 0.24f, outline.r, outline.g, outline.b);
}

void drawJokerStrip(ScreenRenderer& r,
                    const std::vector<Joker>& jokers,
                    const gameplay_state_helpers::CompactTopScreenLayout& topLayout,
                    int activeJokerIndex = -1) {
    const int numJokers = static_cast<int>(jokers.size());
    if (numJokers <= 0) {
        return;
    }

    const int jokerStartX = gameplay_state_helpers::jokerStripStartX(numJokers, topLayout);
    for (int i = 0; i < numJokers; ++i) {
        const int jx = jokerStartX + i * topLayout.jokerSpacing;
        const int jy = topLayout.jokerStripY;

        uint8_t jr = 100, jg = 100, jb = 100;
        if (jokers[i].effectType == JokerEffectType::AddChips)  { jr =  80; jg = 120; jb = 220; }
        else if (jokers[i].effectType == JokerEffectType::AddMult)  { jr = 220; jg =  60; jb =  60; }
        else if (jokers[i].effectType == JokerEffectType::MulMult)  { jr = 180; jg =  60; jb = 220; }

        r.fillRect(jx, jy, topLayout.jokerBoxW, topLayout.jokerBoxH, jr, jg, jb);
        if (i == activeJokerIndex) {
            r.drawRectOutline(jx - 1, jy - 1, topLayout.jokerBoxW + 2, topLayout.jokerBoxH + 2,
                              255, 220, 50);
        } else {
            r.drawRectOutline(jx, jy, topLayout.jokerBoxW, topLayout.jokerBoxH, 255, 255, 255);
        }
        r.drawText(gameplay_state_helpers::compactJokerLabel(jokers[i].name),
                   jx + 2, jy + 12, 0.3f, 255, 255, 255);
    }
}

void drawRemainingUnselectedHand(Application* app,
                                 ScreenRenderer& r,
                                 const Hand& hand,
                                 const gameplay_state_helpers::CompactTopScreenLayout& topLayout) {
    (void)r;

    const auto layout = CardRenderer::gameplayHandLayout();
    const int handSize = hand.size();
    for (int i = 0; i < handSize; ++i) {
        if (hand.isSelected(i)) {
            continue;
        }

        const int cardX = CardRenderer::handCardX(topLayout.handCenterX, handSize, i, layout);
        CardRenderer::drawCard(app, hand.at(i), cardX, topLayout.handY, false, layout);
    }
}

int handIndexForInstanceId(const Hand& hand, uint32_t instanceId) {
    for (int i = 0; i < hand.size(); ++i) {
        if (hand.at(i).instanceId == instanceId) {
            return i;
        }
    }
    return -1;
}

} // namespace

GameplayState::GameplayState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)), m_cursorIndex(0),
      m_sortMode(GameplaySortMode::Rank),
      m_phase(RoundPhase::Playing), m_phaseTimer(0.0f),
      m_lastHandType(HandType::HighCard), m_lastChips(0), m_lastMult(0),
      m_lastScore(0), m_showResult(false), m_resultTimer(0.0f), m_inputDelay(0.3f)
{
    m_stateMachine = machine;
}

void GameplayState::enter() {
    m_sortMode = GameplaySortMode::Rank;
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    startNewRound();
}

void GameplayState::exit() {}

void GameplayState::startNewRound() {
    m_runState->startRound();
    m_hand = Hand();
    m_cursorIndex = 0;
    m_sortMode = GameplaySortMode::Rank;
    m_showResult = false;
    m_resultTimer = 0.0f;
    m_scorer.reset();
    m_phase = RoundPhase::Playing;
    m_phaseTimer = 0.0f;
    drawToFull();
    if (!m_hand.empty()) {
        m_cursorIndex = 0;
    }
}

void GameplayState::drawToFull() {
    while (!m_hand.full() && !m_runState->roundDeck().empty()) {
        m_hand.addCard(m_runState->roundDeck().draw());
    }
    applyCurrentSort();
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

void GameplayState::handleHudAction(GameplayHudAction action) {
    switch (action) {
        case GameplayHudAction::Play:
            playHand();
            break;
        case GameplayHudAction::Discard:
            discardSelected();
            break;
        case GameplayHudAction::Sort:
            toggleSortMode();
            break;
        case GameplayHudAction::None:
            break;
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

    const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();
    const auto handLayout = CardRenderer::gameplayHandLayout();
    std::vector<std::pair<int, int>> startPositions;
    startPositions.reserve(selected.size());
    for (int i = 0; i < m_hand.size(); ++i) {
        if (!m_hand.isSelected(i)) {
            continue;
        }

        const int x = CardRenderer::handCardX(topLayout.handCenterX, m_hand.size(), i, handLayout);
        const int y = topLayout.handY - handLayout.selectOffset;
        startPositions.push_back({ x, y });
    }

    m_scorer = std::make_unique<ScoringAnimator>(
        selected,
        startPositions,
        m_runState->jokers,
        result,
        m_runState->roundScore,
        topLayout.handCenterX,
        kScoringStageY);
    m_phase = RoundPhase::Scoring;
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

void GameplayState::applyCurrentSort() {
    if (m_hand.empty()) {
        m_cursorIndex = 0;
        return;
    }

    const int previousCursor = std::clamp(m_cursorIndex, 0, m_hand.size() - 1);
    const uint32_t cursorInstanceId = m_hand.at(previousCursor).instanceId;

    if (m_sortMode == GameplaySortMode::Suit) {
        m_hand.sortBySuitThenRank();
    } else {
        m_hand.sortByRankDescending();
    }

    const int sortedIndex = handIndexForInstanceId(m_hand, cursorInstanceId);
    if (sortedIndex >= 0) {
        m_cursorIndex = sortedIndex;
        return;
    }

    m_cursorIndex = std::min(previousCursor, m_hand.size() - 1);
}

void GameplayState::toggleSortMode() {
    if (m_phase != RoundPhase::Playing) return;

    m_sortMode = (m_sortMode == GameplaySortMode::Rank)
        ? GameplaySortMode::Suit
        : GameplaySortMode::Rank;
    applyCurrentSort();
}

void GameplayState::handleInput() {
    if (m_inputDelay > 0.0f) return;

#ifdef N3DS
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

        if (kDown & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            handleHudAction(resolveGameplayHudAction(touch.px, touch.py));
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
    
    static int leftCD = 0, rightCD = 0, selectCD = 0, playCD = 0, discardCD = 0, sortCD = 0, confirmCD = 0;
    if (leftCD > 0) leftCD--;
    if (rightCD > 0) rightCD--;
    if (selectCD > 0) selectCD--;
    if (playCD > 0) playCD--;
    if (discardCD > 0) discardCD--;
    if (sortCD > 0) sortCD--;
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
        if (keys[SDL_SCANCODE_S] && sortCD == 0) {
            toggleSortMode();
            sortCD = 20;
        }

        int mx, my;
        uint32_t mouseState = SDL_GetMouseState(&mx, &my);
        static bool mousePressed = false;
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            if (!mousePressed) {
                mousePressed = true;

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
                handleHudAction(resolveGameplayHudAction(mx - kBottomScreenBaseXDesktop, my));
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

    if (m_phase == RoundPhase::Scoring && m_scorer) {
        m_scorer->update(dt);
        if (m_scorer->isDone()) {
            m_runState->addRoundScore(m_lastScore);
            m_runState->handsRemaining--;
            m_hand.removeSelected();
            drawToFull();
            m_showResult = true;
            m_resultTimer = 2.0f;
            m_scorer.reset();
            m_phase = RoundPhase::Playing;
            checkRoundEnd();
        }
        return;
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
    if (m_phase == RoundPhase::Scoring && m_scorer) {
        const auto topLayout = gameplay_state_helpers::compactTopScreenLayout();

        r.drawText("Ante " + std::to_string(m_runState->ante), topLayout.anteX, topLayout.anteY, 0.4f, 255, 200, 80);
        r.drawText(m_runState->currentBlindName(), topLayout.blindX, topLayout.blindY, 0.35f, 180, 180, 200);
        r.drawText("$" + std::to_string(m_runState->money), topLayout.moneyX, topLayout.moneyY, 0.5f, 255, 215, 0);

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            r.drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->currentBossModifier)),
                       topLayout.blindX, topLayout.bossLabelY, 0.32f, 255, 170, 120);
        }

        r.drawRectOutline(topLayout.idleStagePanelRect.x, topLayout.idleStagePanelRect.y,
                          topLayout.idleStagePanelRect.w, topLayout.idleStagePanelRect.h,
                          gameplay_state_helpers::kIdleStagePanelColorR,
                          gameplay_state_helpers::kIdleStagePanelColorG,
                          gameplay_state_helpers::kIdleStagePanelColorB);

        drawJokerStrip(r, m_runState->jokers, topLayout, m_scorer->activeJokerIndex());
        m_scorer->render(app, r);
        drawRemainingUnselectedHand(app, r, m_hand, topLayout);
    }
    else if (m_phase == RoundPhase::Playing) {
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
        drawJokerStrip(r, m_runState->jokers, topLayout);
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

    if (m_phase == RoundPhase::Scoring && m_scorer) {
        const auto bottomLayout = compactBottomScreenLayout();
        const int selectedCount = static_cast<int>(m_hand.getSelected().size());

        {
            const auto money = gameplayHudMoneyColor();
            const auto outline = gameplayHudOutlineColor();
            drawHudCell(r, bottomLayout.blindCell, "BLIND", m_runState->currentBlindName(), 255, 255, 255);
            drawHudCell(r, bottomLayout.roundScoreCell, "SCORE",
                        std::to_string(m_scorer->displayRoundScore()), 255, 255, 255);
            drawHudCell(r, bottomLayout.targetCell, "TARGET",
                        std::to_string(m_runState->roundTarget), outline.r, outline.g, outline.b);
            drawHudCell(r, bottomLayout.moneyCell, "MONEY",
                        "$" + std::to_string(m_runState->money), money.r, money.g, money.b);
        }

        drawProgressTrack(r, bottomLayout.progressTrack,
                          m_scorer->displayRoundScore(), m_runState->roundTarget);

        // Scoring cluster: hand name + selected (Band 2, top row)
        drawHudCell(r, bottomLayout.handTypeCell, "HAND",
                    handTypeName(m_scorer->handType()), 255, 255, 255);
        drawHudCell(r, bottomLayout.selectedCell, "SELECTED",
                    std::to_string(selectedCount) + "/5", 191, 199, 213);
        // Chips and mult as dominant Balatro-style accent blocks (Band 2, bottom row)
        drawScoringValueCell(r, bottomLayout.chipsCell, "CHIPS",
                             std::to_string(m_scorer->displayChips()),
                             gameplayHudChipsColor());
        drawScoringValueCell(r, bottomLayout.multCell, "MULT",
                             "x" + std::to_string(m_scorer->displayMult()),
                             gameplayHudMultColor());

        {
            const auto chips = gameplayHudChipsColor();
            const auto mult = gameplayHudMultColor();
            const auto money = gameplayHudMoneyColor();
            drawHudCell(r, bottomLayout.handsCell, "HANDS",
                        std::to_string(m_runState->handsRemaining), chips.r, chips.g, chips.b);
            drawHudCell(r, bottomLayout.discardsCell, "DISCARD",
                        std::to_string(m_runState->discardsRemaining), mult.r, mult.g, mult.b);
            drawHudCell(r, bottomLayout.anteCell, "ANTE",
                        std::to_string(m_runState->ante), money.r, money.g, money.b);
            drawHudCell(r, bottomLayout.roundCell, "ROUND",
                        std::to_string(blindStageRoundNumber(m_runState->blindStage)), 255, 255, 255);
        }

        const GameplayRect playButton = gameplayPlayButtonRect();
        const GameplayRect discardButton = gameplayDiscardButtonRect();
        const GameplayRect sortButton = gameplaySortButtonRect();

        drawPrimaryAction(r, playButton, GameplayHudAction::Play, "Play", false);
        drawPrimaryAction(r, discardButton, GameplayHudAction::Discard, "Discard", false);
        drawSortUtility(r, sortButton, gameplaySortModeLabel(m_sortMode), false);

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            r.drawText(RunState::bossModifierDescription(m_runState->currentBossModifier, m_runState->currentBlockedSuit),
                       kHudOuterX, bottomLayout.bossDescriptionY, 0.28f, 220, 220, 220);
        }
    }
    else if (m_phase == RoundPhase::Playing) {
        const auto bottomLayout = compactBottomScreenLayout();
        auto selected = m_hand.getSelected();
        const int numSelected = static_cast<int>(selected.size());
        std::string handTypeValue = "No hand";
        std::string chipsValue = "0";
        std::string multValue = "x0";

        if (numSelected >= 1 && numSelected <= 5) {
            HandResult preview = HandEvaluator::evaluate(
                selected,
                m_runState->jokers,
                m_runState->currentBossModifier,
                m_runState->currentBlockedSuit,
                m_runState.get());
            handTypeValue = handTypeName(preview.detectedHand);
            chipsValue = std::to_string(preview.finalChips);
            multValue = "x" + std::to_string(preview.finalMult);
        }

        {
            const auto money = gameplayHudMoneyColor();
            const auto outline = gameplayHudOutlineColor();
            drawHudCell(r, bottomLayout.blindCell, "BLIND", m_runState->currentBlindName(), 255, 255, 255);
            drawHudCell(r, bottomLayout.roundScoreCell, "SCORE",
                        std::to_string(m_runState->roundScore), 255, 255, 255);
            drawHudCell(r, bottomLayout.targetCell, "TARGET",
                        std::to_string(m_runState->roundTarget), outline.r, outline.g, outline.b);
            drawHudCell(r, bottomLayout.moneyCell, "MONEY",
                        "$" + std::to_string(m_runState->money), money.r, money.g, money.b);
        }

        drawProgressTrack(r, bottomLayout.progressTrack,
                          m_runState->roundScore, m_runState->roundTarget);

        // Scoring cluster: hand name + selected (Band 2, top row)
        drawHudCell(r, bottomLayout.handTypeCell, "HAND",
                    handTypeValue, 255, 255, 255);
        drawHudCell(r, bottomLayout.selectedCell, "SELECTED",
                    std::to_string(numSelected) + "/5", 191, 199, 213);
        // Chips and mult as dominant Balatro-style accent blocks (Band 2, bottom row)
        drawScoringValueCell(r, bottomLayout.chipsCell, "CHIPS",
                             chipsValue, gameplayHudChipsColor());
        drawScoringValueCell(r, bottomLayout.multCell, "MULT",
                             multValue, gameplayHudMultColor());

        {
            const auto chips = gameplayHudChipsColor();
            const auto mult = gameplayHudMultColor();
            const auto money = gameplayHudMoneyColor();
            drawHudCell(r, bottomLayout.handsCell, "HANDS",
                        std::to_string(m_runState->handsRemaining), chips.r, chips.g, chips.b);
            drawHudCell(r, bottomLayout.discardsCell, "DISCARD",
                        std::to_string(m_runState->discardsRemaining), mult.r, mult.g, mult.b);
            drawHudCell(r, bottomLayout.anteCell, "ANTE",
                        std::to_string(m_runState->ante), money.r, money.g, money.b);
            drawHudCell(r, bottomLayout.roundCell, "ROUND",
                        std::to_string(blindStageRoundNumber(m_runState->blindStage)), 255, 255, 255);
        }

        if (m_runState->isBossBlind() && m_runState->currentBossModifier != BossBlindModifier::None) {
            r.drawText(RunState::bossModifierDescription(m_runState->currentBossModifier, m_runState->currentBlockedSuit),
                       kHudOuterX, bottomLayout.bossDescriptionY, 0.28f, 220, 220, 220);
        }

        const GameplayRect playButton = gameplayPlayButtonRect();
        const GameplayRect discardButton = gameplayDiscardButtonRect();
        const GameplayRect sortButton = gameplaySortButtonRect();

        drawPrimaryAction(r, playButton, GameplayHudAction::Play, "Play", true);
        drawPrimaryAction(r, discardButton, GameplayHudAction::Discard, "Discard", true);
        drawSortUtility(r, sortButton, gameplaySortModeLabel(m_sortMode), true);
    }
    else {
        // Non-playing phases: show ante info on bottom screen
        r.drawText("Ante " + std::to_string(m_runState->ante) + " / " + std::to_string(RunState::kMaxAnte),
                   80, 70, 0.5f, 255, 200, 80);
        r.drawText(m_runState->currentBlindName(), 100, 95, 0.45f, 200, 200, 220);
        r.drawText("Target " + std::to_string(m_runState->roundTarget), 90, 110, 0.45f, 200, 200, 220);
    }
}

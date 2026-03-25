#include "ShopState.h"
#include "BlindSelectState.h"
#include "GameplayState.h"
#include "states/ShopLayout.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include <random>
#include <utility>

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

namespace {

void advanceToNextBlindAndResume(StateMachine* machine, const std::shared_ptr<RunState>& runState) {
    machine->changeState(std::make_shared<BlindSelectState>(machine, runState));
}

#ifdef N3DS
u32 toC2DColor(const ShopColor& color) {
    return C2D_Color32(color.r, color.g, color.b, color.a);
}
#else
SDL_Rect toSDLRect(const ShopRect& rect) {
    return SDL_Rect{ rect.x, rect.y, rect.w, rect.h };
}

void fillRect(SDL_Renderer* renderer, const ShopRect& rect, const ShopColor& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_Rect sdlRect = toSDLRect(rect);
    SDL_RenderFillRect(renderer, &sdlRect);
}

void drawRect(SDL_Renderer* renderer, const ShopRect& rect, const ShopColor& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_Rect sdlRect = toSDLRect(rect);
    SDL_RenderDrawRect(renderer, &sdlRect);
}
#endif

const ShopColor kWhite{ 255, 255, 255, 255 };
const ShopColor kSelectedBorder{ 255, 255, 100, 255 };
const ShopColor kEmptySlot{ 30, 30, 40, 255 };
const ShopColor kDimBorder{ 80, 80, 100, 255 };
const ShopColor kSoldFill{ 45, 45, 55, 255 };
const ShopColor kSoldBorder{ 100, 100, 120, 255 };
const ShopColor kUnavailableFill{ 55, 42, 32, 255 };
const ShopColor kUnavailableBorder{ 165, 140, 105, 255 };

} // namespace

ShopState::ShopState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)),
      m_rng(std::random_device{}()) {
    m_stateMachine = machine;
}

void ShopState::enter() {
    m_heldInspectIndex = -1;
    generateItems();
}

void ShopState::exit() {}

void ShopState::update(float dt) {
    if (m_inputDelay > 0.0f) {
        m_inputDelay -= dt;
    }
}

void ShopState::generateItems() {
    m_slots = generateShopItems(m_rng, *m_runState);
    for (int i = 0; i < kVisibleShopSlots; ++i) {
        m_slots[i].sold = false;
    }
    m_cursorIndex = nextSelectableShopSlot(0, +1, disabledMask());
}

void ShopState::clearHeldInspect() {
    m_heldInspectIndex = -1;
}

std::array<bool, kVisibleShopSlots> ShopState::disabledMask() const {
    return shop_state_helpers::disabledShopSlots(m_slots);
}

bool ShopState::tryBuySelectedItem() {
    if (!isSelectableShopSlot(m_cursorIndex, disabledMask())) return false;
    if (m_runState->money < m_slots[m_cursorIndex].item.price) return false;
    if (m_runState->jokers.size() >= static_cast<size_t>(m_runState->jokerLimit)) return false;

    const int purchasedSlot = m_cursorIndex;
    clearHeldInspect();
    m_cursorIndex = shop_state_helpers::purchaseShopSlotAndAdvanceCursor(*m_runState, m_slots, purchasedSlot);
    return true;
}

bool ShopState::trySellHeldJoker() {
    if (m_heldInspectIndex < 0) return false;

    const int idx = m_heldInspectIndex;
    if (idx >= static_cast<int>(m_runState->jokers.size())) return false;

    const Joker& sold = m_runState->jokers[idx];
    m_runState->money += sold.sellValue;
    m_runState->markJokerReturnedToShopPool(Joker::idFor(sold));
    m_runState->jokers.erase(m_runState->jokers.begin() + idx);
    clearHeldInspect();
    return true;
}

bool ShopState::tryReroll() {
    if (m_runState->money < m_runState->rerollCost) return false;

    m_runState->money -= m_runState->rerollCost;
    ++m_runState->rerollCost;
    generateItems();
    clearHeldInspect();
    return true;
}

void ShopState::handleInput() {
    if (m_inputDelay > 0.0f) return;

    auto moveCursor = [&](int direction) {
        const std::array<bool, kVisibleShopSlots> disabled = disabledMask();
        const int candidate = m_cursorIndex + direction;
        if (isSelectableShopSlot(candidate, disabled)) {
            m_cursorIndex = candidate;
            clearHeldInspect();
        }
    };

#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_DLEFT) {
        moveCursor(-1);
    } else if (kDown & KEY_DRIGHT) {
        moveCursor(+1);
    }

    if (kDown & KEY_A) {
        if (m_heldInspectIndex >= 0)
            trySellHeldJoker();
        else
            tryBuySelectedItem();
    }

    if (kDown & KEY_START || kDown & KEY_X) {
        // Next Round
        advanceToNextBlindAndResume(m_stateMachine, m_runState);
    }

    // Bottom Screen Touch
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);

        // Held joker inspect
        {
            int heldHit = hitHeldJoker(ShopPlatform::ThreeDS, static_cast<int>(m_runState->jokers.size()), touch.px, touch.py);
            if (heldHit >= 0) {
                m_heldInspectIndex = heldHit;
            }
        }

        if (hitBuyButton(ShopPlatform::ThreeDS, touch.px, touch.py)) {
            if (m_heldInspectIndex >= 0)
                trySellHeldJoker();
            else
                tryBuySelectedItem();
        }
        else if (hitRerollButton(ShopPlatform::ThreeDS, touch.px, touch.py)) {
            tryReroll();
        }
        else if (hitNextBlindButton(ShopPlatform::ThreeDS, touch.px, touch.py)) {
            advanceToNextBlindAndResume(m_stateMachine, m_runState);
        }
    }
    
#else
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    static bool leftPressed = false, rightPressed = false, spacePressed = false, enterPressed = false,
                rerollPressed = false;

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!leftPressed) {
            moveCursor(-1);
            leftPressed = true;
        }
    } else { leftPressed = false; }

    if (keys[SDL_SCANCODE_RIGHT]) {
        if (!rightPressed) {
            moveCursor(+1);
            rightPressed = true;
        }
    } else { rightPressed = false; }

    if (keys[SDL_SCANCODE_SPACE]) {
        if (!spacePressed) {
            spacePressed = true;
            if (m_heldInspectIndex >= 0)
                trySellHeldJoker();
            else
                tryBuySelectedItem();
        }
    } else { spacePressed = false; }

    if (keys[SDL_SCANCODE_R]) {
        if (!rerollPressed) {
            rerollPressed = true;
            tryReroll();
        }
    } else { rerollPressed = false; }

    if (keys[SDL_SCANCODE_RETURN]) {
        if (!enterPressed) {
            enterPressed = true;
            advanceToNextBlindAndResume(m_stateMachine, m_runState);
        }
    } else { enterPressed = false; }

    // Mouse Input
    int mx, my;
    uint32_t mouseState = SDL_GetMouseState(&mx, &my);

    // Hover — update cursor only when mouse moves so keyboard nav wins when mouse is stationary
    if (mx != m_lastMouseX || my != m_lastMouseY) {
        if (mx < 400) {
            const int hit = hitShopCard(ShopPlatform::SDL, kVisibleShopSlots, mx, my);
            if (isSelectableShopSlot(hit, disabledMask())) {
                m_cursorIndex = hit;
                clearHeldInspect();
            }
        }
    }
    m_lastMouseX = mx;
    m_lastMouseY = my;

    static bool mousePressed = false;
    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        if (!mousePressed) {
            mousePressed = true;

            // Top Screen (card selection via click)
            if (mx < 400) {
                const int hit = hitShopCard(ShopPlatform::SDL, kVisibleShopSlots, mx, my);
                if (isSelectableShopSlot(hit, disabledMask())) {
                    m_cursorIndex = hit;
                    clearHeldInspect();
                }
            }
            // Bottom screen
            else if (mx >= 400) {
                // Held joker inspect
                int heldHit = hitHeldJoker(ShopPlatform::SDL, static_cast<int>(m_runState->jokers.size()), mx, my);
                if (heldHit >= 0) m_heldInspectIndex = heldHit;
                if (hitBuyButton(ShopPlatform::SDL, mx, my)) {
                    if (m_heldInspectIndex >= 0)
                        trySellHeldJoker();
                    else
                        tryBuySelectedItem();
                }
                else if (hitRerollButton(ShopPlatform::SDL, mx, my)) {
                    tryReroll();
                }
                else if (hitNextBlindButton(ShopPlatform::SDL, mx, my)) {
                    advanceToNextBlindAndResume(m_stateMachine, m_runState);
                }
            }
        }
    } else {
        mousePressed = false;
    }
#endif
}

void ShopState::renderTopScreen(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
#endif

    // ── HUD ──
#ifdef N3DS
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, C2D_Color32(20, 30, 40, 255)); // Background
    TextRenderer::drawText("SHOP", 12, 14, 0.75f, 0.75f, C2D_Color32(255, 180, 80, 255));
    TextRenderer::drawText("Money: $" + std::to_string(m_runState->money), 12, 39, 0.55f, 0.55f, C2D_Color32(255, 215, 0, 255));
    TextRenderer::drawText("Next: Ante " + std::to_string(m_runState->nextBlindAnte()) + " " + RunState::blindStageName(m_runState->nextBlindStage()),
                           12, 59, 0.38f, 0.38f, C2D_Color32(200, 200, 220, 255));
    TextRenderer::drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                           12, 78, 0.34f, 0.34f, C2D_Color32(255, 170, 120, 255));
    TextRenderer::drawText(RunState::bossModifierDescription(m_runState->nextBossModifier, m_runState->nextBlockedSuit),
                           12, 96, 0.3f, 0.3f, C2D_Color32(220, 220, 220, 255));
#else
    SDL_SetRenderDrawColor(renderer, 20, 30, 40, 255);
    SDL_Rect bgTop = { 0, 0, 400, 240 };
    SDL_RenderFillRect(renderer, &bgTop);
    
    TextRenderer::drawText(renderer, "SHOP", 16, 14, 2, 255, 180, 80);
    TextRenderer::drawText(renderer, "Money: $" + std::to_string(m_runState->money), 16, 42, 1, 255, 215, 0);
    TextRenderer::drawText(renderer, "Next: Ante " + std::to_string(m_runState->nextBlindAnte()) + " " + RunState::blindStageName(m_runState->nextBlindStage()),
                           16, 64, 0, 200, 200, 220);
    TextRenderer::drawText(renderer, "Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                           16, 84, 0, 255, 170, 120);
    TextRenderer::drawText(renderer,
                           RunState::bossModifierDescription(m_runState->nextBossModifier, m_runState->nextBlockedSuit),
                           16, 102, 0, 220, 220, 220);
#endif

    // ── Items for Sale ──
    const ShopPlatform platform =
#ifdef N3DS
        ShopPlatform::ThreeDS;
#else
        ShopPlatform::SDL;
#endif
    const std::array<bool, kVisibleShopSlots> disabled = disabledMask();

    for (int i = 0; i < kVisibleShopSlots; ++i) {
        const ShopRect body = shopCardBodyRect(platform, kVisibleShopSlots, i);
        const ShopRect highlight = shopCardHighlightRect(platform, kVisibleShopSlots, i);
        const bool selectable = isSelectableShopSlot(i, disabled);
        const bool unavailable = m_slots[i].unavailable;
        const ShopColor fillColor = selectable
            ? jokerEffectColor(m_slots[i].item.joker.effectType)
            : (unavailable ? kUnavailableFill : kSoldFill);
        const ShopColor borderColor = selectable
            ? kWhite
            : (unavailable ? kUnavailableBorder : kSoldBorder);

#ifdef N3DS
        if (selectable && i == m_cursorIndex) {
            C2D_DrawRectSolid(highlight.x, highlight.y, 0.5f, highlight.w, highlight.h, toC2DColor(kSelectedBorder));
        }

        C2D_DrawRectSolid(body.x, body.y, 0.5f, body.w, body.h, toC2DColor(fillColor));
        C2D_DrawRectSolid(body.x, body.y, 0.5f, body.w, 1, toC2DColor(borderColor));
        C2D_DrawRectSolid(body.x, body.y + body.h - 1, 0.5f, body.w, 1, toC2DColor(borderColor));
        C2D_DrawRectSolid(body.x, body.y, 0.5f, 1, body.h, toC2DColor(borderColor));
        C2D_DrawRectSolid(body.x + body.w - 1, body.y, 0.5f, 1, body.h, toC2DColor(borderColor));

        if (selectable) {
            TextRenderer::drawText(m_slots[i].item.joker.name, body.x + 5, body.y + 10, 0.45f, 0.45f, toC2DColor(kWhite));
            TextRenderer::drawText("$" + std::to_string(m_slots[i].item.price), body.x + 30, body.y + 50, 0.5f, 0.5f, C2D_Color32(255, 215, 0, 255));
        } else {
            TextRenderer::drawText(shop_state_helpers::blockedShopSlotLabel(m_slots[i]),
                                   body.x + (unavailable ? 6 : body.w / 2 - 8),
                                   body.y + body.h / 2 - 5,
                                   unavailable ? 0.3f : 0.5f,
                                   unavailable ? 0.3f : 0.5f,
                                   toC2DColor(borderColor));
        }
#else
        if (selectable && i == m_cursorIndex) {
            fillRect(renderer, highlight, kSelectedBorder);
        }

        fillRect(renderer, body, fillColor);
        drawRect(renderer, body, borderColor);

        if (selectable) {
            TextRenderer::drawText(renderer, m_slots[i].item.joker.name, body.x + 5, body.y + 10, 0, 255, 255, 255);
            TextRenderer::drawText(renderer, "$" + std::to_string(m_slots[i].item.price), body.x + 35, body.y + 50, 1, 255, 215, 0);
        } else {
            TextRenderer::drawText(renderer,
                                   shop_state_helpers::blockedShopSlotLabel(m_slots[i]),
                                   body.x + (unavailable ? 8 : body.w / 2 - 15),
                                   body.y + body.h / 2 - 5,
                                   0,
                                   borderColor.r,
                                   borderColor.g,
                                   borderColor.b);
        }
#endif
    }
}

void ShopState::renderBottomScreen(Application* app) {
#ifdef N3DS
    int baseX = 0;
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 240, C2D_Color32(15, 20, 30, 255)); // Darker background
    
    // Inspect panel
    const InspectSelection sel = resolveInspectSelection(
        m_heldInspectIndex,
        static_cast<int>(m_runState->jokers.size()),
        isSelectableShopSlot(m_cursorIndex, disabledMask()) ? m_cursorIndex : -1,
        kVisibleShopSlots
    );
    if (sel.source == InspectSource::HeldJoker) {
        TextRenderer::drawText(m_runState->jokers[sel.index].name, 10, 20, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText(m_runState->jokers[sel.index].description, 10, 45, 0.4f, 0.4f, C2D_Color32(180, 180, 180, 255));
    } else if (sel.source == InspectSource::ShopItem) {
        TextRenderer::drawText(m_slots[sel.index].item.joker.name, 10, 20, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText(m_slots[sel.index].item.joker.description, 10, 45, 0.4f, 0.4f, C2D_Color32(180, 180, 180, 255));
    } else {
        TextRenderer::drawText("Use D-pad to inspect", 10, 20, 0.4f, 0.4f, C2D_Color32(150, 150, 150, 255));
    }

    TextRenderer::drawText("Your Jokers: " + std::to_string(m_runState->jokers.size()) + "/" + std::to_string(m_runState->jokerLimit),
                           12, 72, 0.35f, 0.35f, C2D_Color32(200, 200, 200, 255));

    for (int i = 0; i < m_runState->jokerLimit; ++i) {
        const ShopRect slot = heldJokerSlotRect(ShopPlatform::ThreeDS, i);
        const bool filled = i < static_cast<int>(m_runState->jokers.size());
        const ShopColor slotColor = filled ? jokerEffectColor(m_runState->jokers[i].effectType) : kEmptySlot;
        const ShopColor borderColor = filled ? kWhite : kDimBorder;

        C2D_DrawRectSolid(slot.x, slot.y, 0.5f, slot.w, slot.h, toC2DColor(slotColor));
        C2D_DrawRectSolid(slot.x, slot.y, 0.5f, slot.w, 1, toC2DColor(borderColor));
        C2D_DrawRectSolid(slot.x, slot.y + slot.h - 1, 0.5f, slot.w, 1, toC2DColor(borderColor));
        C2D_DrawRectSolid(slot.x, slot.y, 0.5f, 1, slot.h, toC2DColor(borderColor));
        C2D_DrawRectSolid(slot.x + slot.w - 1, slot.y, 0.5f, 1, slot.h, toC2DColor(borderColor));

        if (filled) {
            TextRenderer::drawText(m_runState->jokers[i].name, slot.x + 4, slot.y + 8, 0.35f, 0.35f, toC2DColor(kWhite));
        }

        if (m_heldInspectIndex == i && filled) {
            C2D_DrawRectSolid(slot.x, slot.y, 0.5f, slot.w, 2, toC2DColor(kSelectedBorder));
            C2D_DrawRectSolid(slot.x, slot.y + slot.h - 2, 0.5f, slot.w, 2, toC2DColor(kSelectedBorder));
            C2D_DrawRectSolid(slot.x, slot.y, 0.5f, 2, slot.h, toC2DColor(kSelectedBorder));
            C2D_DrawRectSolid(slot.x + slot.w - 2, slot.y, 0.5f, 2, slot.h, toC2DColor(kSelectedBorder));
        }
    }
    
    // Action button (Buy or Sell)
    {
        const ShopRect actionRect = buyButtonRect(ShopPlatform::ThreeDS);
        const bool isSell = m_heldInspectIndex >= 0 &&
                            m_heldInspectIndex < static_cast<int>(m_runState->jokers.size());
        const u32 fillColor = isSell ? C2D_Color32(100, 50, 20, 255) : C2D_Color32(20, 60, 20, 255);
        const u32 borderColor = C2D_Color32(200, 200, 80, 255);

        C2D_DrawRectSolid(actionRect.x, actionRect.y, 0.5f, actionRect.w, actionRect.h, fillColor);
        C2D_DrawRectSolid(actionRect.x, actionRect.y, 0.5f, actionRect.w, 2, borderColor);
        C2D_DrawRectSolid(actionRect.x, actionRect.y + actionRect.h - 2, 0.5f, actionRect.w, 2, borderColor);
        C2D_DrawRectSolid(actionRect.x, actionRect.y, 0.5f, 2, actionRect.h, borderColor);
        C2D_DrawRectSolid(actionRect.x + actionRect.w - 2, actionRect.y, 0.5f, 2, actionRect.h, borderColor);

        if (isSell) {
            const int sellAmt = m_runState->jokers[m_heldInspectIndex].sellValue;
            TextRenderer::drawText("SELL $" + std::to_string(sellAmt),
                                   actionRect.x + 6, actionRect.y + 18,
                                   0.38f, 0.38f, C2D_Color32(255, 215, 0, 255));
        } else {
            TextRenderer::drawText("BUY",
                                   actionRect.x + 36, actionRect.y + 18,
                                   0.45f, 0.45f, C2D_Color32(255, 255, 255, 255));
        }
    }

    // Reroll button
    {
        const ShopRect rRect = rerollButtonRect(ShopPlatform::ThreeDS);
        const bool canAfford = m_runState->money >= m_runState->rerollCost;
        const u32 fillColor = canAfford
            ? C2D_Color32(30, 30, 70, 255) : C2D_Color32(30, 30, 30, 255);
        const u32 borderColor = canAfford
            ? C2D_Color32(120, 120, 220, 255) : C2D_Color32(60, 60, 60, 255);
        const u32 textColor = canAfford
            ? C2D_Color32(200, 200, 255, 255) : C2D_Color32(80, 80, 80, 255);

        C2D_DrawRectSolid(rRect.x, rRect.y, 0.5f, rRect.w, rRect.h, fillColor);
        C2D_DrawRectSolid(rRect.x, rRect.y, 0.5f, rRect.w, 2, borderColor);
        C2D_DrawRectSolid(rRect.x, rRect.y + rRect.h - 2, 0.5f, rRect.w, 2, borderColor);
        C2D_DrawRectSolid(rRect.x, rRect.y, 0.5f, 2, rRect.h, borderColor);
        C2D_DrawRectSolid(rRect.x + rRect.w - 2, rRect.y, 0.5f, 2, rRect.h, borderColor);
        TextRenderer::drawText("REROLL",
                               rRect.x + 4, rRect.y + 8, 0.30f, 0.30f, textColor);
        TextRenderer::drawText("$" + std::to_string(m_runState->rerollCost),
                               rRect.x + 16, rRect.y + 26, 0.35f, 0.35f, textColor);
    }

    // Next Blind button
    {
        const ShopRect nRect = nextBlindButtonRect(ShopPlatform::ThreeDS);
        C2D_DrawRectSolid(nRect.x, nRect.y, 0.5f, nRect.w, nRect.h, C2D_Color32(80, 140, 255, 255));
        C2D_DrawRectSolid(nRect.x, nRect.y, 0.5f, nRect.w, 2, C2D_Color32(120, 180, 255, 255));
        TextRenderer::drawText("Next Blind", nRect.x + 8, nRect.y + 18,
                               0.42f, 0.42f, C2D_Color32(0, 0, 0, 255));
    }
    
    // Hints
    TextRenderer::drawText("[A]", baseX + 70, 215, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
    TextRenderer::drawText("[Start]", baseX + 200, 215, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    int baseX = 400;
    
    SDL_SetRenderDrawColor(renderer, 15, 20, 30, 255);
    SDL_Rect bgBot = { 400, 0, 400, 240 };
    SDL_RenderFillRect(renderer, &bgBot);

    // Inspect panel
    const InspectSelection sel = resolveInspectSelection(
        m_heldInspectIndex,
        static_cast<int>(m_runState->jokers.size()),
        isSelectableShopSlot(m_cursorIndex, disabledMask()) ? m_cursorIndex : -1,
        kVisibleShopSlots
    );
    if (sel.source == InspectSource::HeldJoker) {
        TextRenderer::drawText(renderer, m_runState->jokers[sel.index].name, baseX + 10, 20, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, m_runState->jokers[sel.index].description, baseX + 10, 45, 0, 180, 180, 180);
    } else if (sel.source == InspectSource::ShopItem) {
        TextRenderer::drawText(renderer, m_slots[sel.index].item.joker.name, baseX + 10, 20, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, m_slots[sel.index].item.joker.description, baseX + 10, 45, 0, 180, 180, 180);
    } else {
        TextRenderer::drawText(renderer, "Hover a card to inspect", baseX + 10, 20, 0, 150, 150, 150);
    }

    TextRenderer::drawText(renderer,
                           "Your Jokers: " + std::to_string(m_runState->jokers.size()) + "/" + std::to_string(m_runState->jokerLimit),
                           baseX + 10, 72, 0, 200, 200, 200);

    for (int i = 0; i < m_runState->jokerLimit; ++i) {
        const ShopRect slot = heldJokerSlotRect(ShopPlatform::SDL, i);
        const bool filled = i < static_cast<int>(m_runState->jokers.size());
        const ShopColor slotColor = filled ? jokerEffectColor(m_runState->jokers[i].effectType) : kEmptySlot;
        const ShopColor borderColor = filled ? kWhite : kDimBorder;

        fillRect(renderer, slot, slotColor);
        drawRect(renderer, slot, borderColor);

        if (filled) {
            const SDL_Rect clipRect = toSDLRect(slot);
            SDL_RenderSetClipRect(renderer, &clipRect);
            TextRenderer::drawText(renderer, m_runState->jokers[i].name, slot.x + 4, slot.y + 8, 0, 255, 255, 255);
            SDL_RenderSetClipRect(renderer, nullptr);
        }

        if (m_heldInspectIndex == i && filled) {
            drawRect(renderer, slot, kSelectedBorder);
        }
    }
    
    // Action button (Buy or Sell)
    {
        const ShopRect actionRect = buyButtonRect(ShopPlatform::SDL);
        const bool isSell = m_heldInspectIndex >= 0 &&
                            m_heldInspectIndex < static_cast<int>(m_runState->jokers.size());
        SDL_Rect fill = { actionRect.x, actionRect.y, actionRect.w, actionRect.h };
        SDL_SetRenderDrawColor(renderer, isSell ? 100 : 20, isSell ? 50 : 60, isSell ? 20 : 20, 255);
        SDL_RenderFillRect(renderer, &fill);
        SDL_SetRenderDrawColor(renderer, 200, 200, 80, 255);
        SDL_RenderDrawRect(renderer, &fill);

        if (isSell) {
            const int sellAmt = m_runState->jokers[m_heldInspectIndex].sellValue;
            TextRenderer::drawText(renderer,
                                   "SELL $" + std::to_string(sellAmt),
                                   actionRect.x + 6, actionRect.y + 16,
                                   0, 255, 215, 0);
        } else {
            TextRenderer::drawText(renderer, "BUY",
                                   actionRect.x + 34, actionRect.y + 16,
                                   0, 255, 255, 255);
        }
    }

    // Reroll button
    {
        const ShopRect rRect = rerollButtonRect(ShopPlatform::SDL);
        const bool canAfford = m_runState->money >= m_runState->rerollCost;
        SDL_Rect fill = { rRect.x, rRect.y, rRect.w, rRect.h };
        SDL_SetRenderDrawColor(renderer, 30, 30, canAfford ? 70 : 30, 255);
        SDL_RenderFillRect(renderer, &fill);
        SDL_SetRenderDrawColor(renderer,
                               canAfford ? 120 : 60,
                               canAfford ? 120 : 60,
                               canAfford ? 220 : 60, 255);
        SDL_RenderDrawRect(renderer, &fill);
        TextRenderer::drawText(renderer, "REROLL",
                               rRect.x + 4, rRect.y + 8,
                               0, canAfford ? 200 : 80, canAfford ? 200 : 80, canAfford ? 255 : 80);
        TextRenderer::drawText(renderer, "$" + std::to_string(m_runState->rerollCost),
                               rRect.x + 18, rRect.y + 26,
                               0, canAfford ? 200 : 80, canAfford ? 200 : 80, canAfford ? 255 : 80);
    }

    // Next Blind button
    {
        const ShopRect nRect = nextBlindButtonRect(ShopPlatform::SDL);
        SDL_Rect fill = { nRect.x, nRect.y, nRect.w, nRect.h };
        SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
        SDL_RenderFillRect(renderer, &fill);
        TextRenderer::drawText(renderer, "Next Blind", nRect.x + 8, nRect.y + 18, 0, 0, 0, 0);
    }
    
    // Hints
    TextRenderer::drawText(renderer, "[Space]", baseX + 60, 215, 0, 200, 200, 220);
    TextRenderer::drawText(renderer, "[R]", baseX + 145, 215, 0, 200, 200, 220);
    TextRenderer::drawText(renderer, "[Enter]", baseX + 220, 215, 0, 200, 200, 220);
#endif
}

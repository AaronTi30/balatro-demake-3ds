#include "ShopState.h"
#include "BlindSelectState.h"
#include "states/ShopLayout.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/ScreenRenderer.h"
#include <random>
#include <utility>

#ifdef N3DS
#include <3ds.h>
#else
#include <SDL.h>
#endif

namespace {

void advanceToNextBlindAndResume(StateMachine* machine, const std::shared_ptr<RunState>& runState) {
    machine->changeState(std::make_shared<BlindSelectState>(machine, runState));
}

bool leaveShopForNextBlind(StateMachine* machine, const std::shared_ptr<RunState>& runState, bool& exitQueued) {
    if (exitQueued) {
        return false;
    }

    exitQueued = true;
    runState->awardInterest();
    advanceToNextBlindAndResume(machine, runState);
    return true;
}

const ShopColor kWhite{ 255, 255, 255, 255 };
const ShopColor kSelectedBorder{ 255, 255, 100, 255 };
const ShopColor kEmptySlot{ 30, 30, 40, 255 };
const ShopColor kDimBorder{ 80, 80, 100, 255 };
const ShopColor kSoldFill{ 45, 45, 55, 255 };
const ShopColor kSoldBorder{ 100, 100, 120, 255 };
const ShopColor kUnavailableFill{ 55, 42, 32, 255 };
const ShopColor kUnavailableBorder{ 165, 140, 105, 255 };
const ShopColor kDeckCardFill{ 70, 110, 170, 255 };
const ShopColor kDeckCardBorder{ 170, 210, 255, 255 };
const ShopColor kHandUpgradeFill{ 90, 70, 45, 255 };
const ShopColor kHandUpgradeBorder{ 220, 190, 120, 255 };

ShopColor shopOfferFillColor(const ShopOffer& offer) {
    switch (offer.kind) {
        case ShopOfferKind::Joker:
            return jokerEffectColor(offer.joker.effectType);
        case ShopOfferKind::DeckCard:
            return kDeckCardFill;
        case ShopOfferKind::HandUpgrade:
            return kHandUpgradeFill;
        default:
            return kSoldFill;
    }
}

ShopColor shopOfferBorderColor(const ShopOffer& offer) {
    switch (offer.kind) {
        case ShopOfferKind::Joker:
            return kWhite;
        case ShopOfferKind::DeckCard:
            return kDeckCardBorder;
        case ShopOfferKind::HandUpgrade:
            return kHandUpgradeBorder;
        default:
            return kWhite;
    }
}

} // namespace

ShopState::ShopState(StateMachine* machine, std::shared_ptr<RunState> runState)
    : m_runState(std::move(runState)),
      m_rng(std::random_device{}()) {
    m_stateMachine = machine;
}

std::array<ShopSlot, kVisibleShopSlots> ShopState::generateShopItems(std::mt19937& rng, const RunState& runState) {
    return generateShopOffers(rng, runState);
}

void ShopState::enter() {
    m_exitQueued = false;
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
    if (m_runState->money < m_slots[m_cursorIndex].offer.price) return false;
    const int purchasedSlot = m_cursorIndex;
    clearHeldInspect();
    if (!applyShopOfferPurchase(*m_runState, m_slots[purchasedSlot])) return false;
    m_cursorIndex = shop_state_helpers::markShopSlotSoldAndAdvanceCursor(m_slots, purchasedSlot);
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
        leaveShopForNextBlind(m_stateMachine, m_runState, m_exitQueued);
        return;
    }

    // Bottom Screen Touch
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        const int localX = touch.px;
        const int localY = touch.py;

        // Held joker inspect
        {
            int heldHit = hitHeldJoker(static_cast<int>(m_runState->jokers.size()), localX, localY);
            if (heldHit >= 0) {
                m_heldInspectIndex = heldHit;
            }
        }

        if (hitBuyButton(localX, localY)) {
            if (m_heldInspectIndex >= 0)
                trySellHeldJoker();
            else
                tryBuySelectedItem();
        }
        else if (hitRerollButton(localX, localY)) {
            tryReroll();
        }
        else if (hitNextBlindButton(localX, localY)) {
            leaveShopForNextBlind(m_stateMachine, m_runState, m_exitQueued);
            return;
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
            leaveShopForNextBlind(m_stateMachine, m_runState, m_exitQueued);
            return;
        }
    } else { enterPressed = false; }

    // Mouse Input
    int mx, my;
    uint32_t mouseState = SDL_GetMouseState(&mx, &my);

    // Hover — update cursor only when mouse moves so keyboard nav wins when mouse is stationary
    if (mx != m_lastMouseX || my != m_lastMouseY) {
        if (mx < 400) {
            const int hit = hitShopCard(kVisibleShopSlots, mx, my);
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
                const int hit = hitShopCard(kVisibleShopSlots, mx, my);
                if (isSelectableShopSlot(hit, disabledMask())) {
                    m_cursorIndex = hit;
                    clearHeldInspect();
                }
            }
            // Bottom screen
            else if (mx >= 400) {
                const int localX = mx - 400;
                const int localY = my;

                // Held joker inspect
                int heldHit = hitHeldJoker(static_cast<int>(m_runState->jokers.size()), localX, localY);
                if (heldHit >= 0) m_heldInspectIndex = heldHit;
                if (hitBuyButton(localX, localY)) {
                    if (m_heldInspectIndex >= 0)
                        trySellHeldJoker();
                    else
                        tryBuySelectedItem();
                }
                else if (hitRerollButton(localX, localY)) {
                    tryReroll();
                }
                else if (hitNextBlindButton(localX, localY)) {
                    leaveShopForNextBlind(m_stateMachine, m_runState, m_exitQueued);
                    return;
                }
            }
        }
    } else {
        mousePressed = false;
    }
#endif
}

void ShopState::renderTopScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    r.fillRect(0, 0, 400, 240, 20, 30, 40);
    r.drawText("SHOP", 12.0f, 14.0f, 0.75f, 255, 180, 80);
    r.drawText("Money: $" + std::to_string(m_runState->money), 12.0f, 39.0f, 0.55f, 255, 215, 0);
    r.drawText("Interest: +$" + std::to_string(m_runState->interestPayout()),
               12.0f, 58.0f, 0.42f, 180, 220, 180);
    r.drawText("Next: Ante " + std::to_string(m_runState->nextBlindAnte()) + " " +
               RunState::blindStageName(m_runState->nextBlindStage()),
               12.0f, 77.0f, 0.38f, 200, 200, 220);
    r.drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
               12.0f, 96.0f, 0.34f, 255, 170, 120);
    r.drawText(RunState::bossModifierDescription(m_runState->nextBossModifier, m_runState->nextBlockedSuit),
               12.0f, 114.0f, 0.30f, 220, 220, 220);

    const std::array<bool, kVisibleShopSlots> disabled = disabledMask();

    for (int i = 0; i < kVisibleShopSlots; ++i) {
        const ShopRect body = shopCardBodyRect(kVisibleShopSlots, i);
        const ShopRect highlight = shopCardHighlightRect(kVisibleShopSlots, i);
        const bool selectable = isSelectableShopSlot(i, disabled);
        const bool unavailable = m_slots[i].unavailable;
        const ShopColor fillColor = selectable
            ? shopOfferFillColor(m_slots[i].offer)
            : (unavailable ? kUnavailableFill : kSoldFill);
        const ShopColor borderColor = selectable
            ? shopOfferBorderColor(m_slots[i].offer)
            : (unavailable ? kUnavailableBorder : kSoldBorder);

        if (selectable && i == m_cursorIndex) {
            r.fillRect(highlight.x, highlight.y, highlight.w, highlight.h,
                       kSelectedBorder.r, kSelectedBorder.g, kSelectedBorder.b, kSelectedBorder.a);
        }

        r.fillRect(body.x, body.y, body.w, body.h,
                   fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        r.drawRectOutline(body.x, body.y, body.w, body.h,
                          borderColor.r, borderColor.g, borderColor.b, borderColor.a);

        if (selectable) {
            r.drawText(shopOfferTitle(m_slots[i].offer), body.x + 4.0f, body.y + 10.0f, 0.40f,
                       kWhite.r, kWhite.g, kWhite.b, kWhite.a);
            r.drawText("$" + std::to_string(m_slots[i].offer.price), body.x + 22.0f, body.y + 50.0f, 0.50f,
                       255, 215, 0);
        } else {
            r.drawText(shop_state_helpers::blockedShopSlotLabel(m_slots[i]),
                       body.x + (unavailable ? 6.0f : body.w / 2.0f - 8.0f),
                       body.y + body.h / 2.0f - 5.0f,
                       unavailable ? 0.30f : 0.50f,
                       borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        }
    }
}

void ShopState::renderBottomScreen(Application* app, ScreenRenderer& r) {
    (void)app;

    r.fillRect(0, 0, 320, 240, 15, 20, 30);

    const InspectSelection sel = resolveInspectSelection(
        m_heldInspectIndex,
        static_cast<int>(m_runState->jokers.size()),
        isSelectableShopSlot(m_cursorIndex, disabledMask()) ? m_cursorIndex : -1,
        kVisibleShopSlots
    );
    if (sel.source == InspectSource::HeldJoker) {
        r.drawText(m_runState->jokers[sel.index].name, 10.0f, 20.0f, 0.50f, 255, 255, 255);
        r.drawText(m_runState->jokers[sel.index].description, 10.0f, 45.0f, 0.40f, 180, 180, 180);
    } else if (sel.source == InspectSource::ShopItem) {
        r.drawText(shopOfferTitle(m_slots[sel.index].offer), 10.0f, 20.0f, 0.50f, 255, 255, 255);
        r.drawText(shopOfferDescription(m_slots[sel.index].offer), 10.0f, 45.0f, 0.40f, 180, 180, 180);
    } else {
        r.drawText("Select a card or joker to inspect", 10.0f, 20.0f, 0.40f, 150, 150, 150);
    }

    r.drawText("Your Jokers: " + std::to_string(m_runState->jokers.size()) + "/" +
               std::to_string(m_runState->jokerLimit),
               12.0f, 72.0f, 0.35f, 200, 200, 200);

    for (int i = 0; i < m_runState->jokerLimit; ++i) {
        const ShopRect slot = heldJokerSlotRect(i);
        const bool filled = i < static_cast<int>(m_runState->jokers.size());
        const ShopColor slotColor = filled ? jokerEffectColor(m_runState->jokers[i].effectType) : kEmptySlot;
        const ShopColor borderColor = filled ? kWhite : kDimBorder;

        r.fillRect(slot.x, slot.y, slot.w, slot.h,
                   slotColor.r, slotColor.g, slotColor.b, slotColor.a);
        r.drawRectOutline(slot.x, slot.y, slot.w, slot.h,
                          borderColor.r, borderColor.g, borderColor.b, borderColor.a);

        if (filled) {
            r.drawText(m_runState->jokers[i].name, slot.x + 4.0f, slot.y + 8.0f, 0.35f,
                       kWhite.r, kWhite.g, kWhite.b, kWhite.a);
        }

        if (m_heldInspectIndex == i && filled) {
            r.drawRectOutline(slot.x, slot.y, slot.w, slot.h,
                              kSelectedBorder.r, kSelectedBorder.g, kSelectedBorder.b, kSelectedBorder.a);
        }
    }

    {
        const ShopRect actionRect = buyButtonRect();
        const bool isSell = m_heldInspectIndex >= 0 &&
                            m_heldInspectIndex < static_cast<int>(m_runState->jokers.size());
        const ShopColor fillColor = isSell ? ShopColor{100, 50, 20, 255} : ShopColor{20, 60, 20, 255};
        const ShopColor borderColor = {200, 200, 80, 255};

        r.fillRect(actionRect.x, actionRect.y, actionRect.w, actionRect.h,
                   fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        r.drawRectOutline(actionRect.x, actionRect.y, actionRect.w, actionRect.h,
                          borderColor.r, borderColor.g, borderColor.b, borderColor.a);

        if (isSell) {
            const int sellAmt = m_runState->jokers[m_heldInspectIndex].sellValue;
            r.drawText("SELL $" + std::to_string(sellAmt),
                       actionRect.x + 6.0f, actionRect.y + 18.0f,
                       0.38f, 255, 215, 0);
        } else {
            r.drawText("BUY",
                       actionRect.x + 36.0f, actionRect.y + 18.0f,
                       0.45f, 255, 255, 255);
        }
    }

    {
        const ShopRect rRect = rerollButtonRect();
        const bool canAfford = m_runState->money >= m_runState->rerollCost;
        const ShopColor fillColor = canAfford
            ? ShopColor{30, 30, 70, 255} : ShopColor{30, 30, 30, 255};
        const ShopColor borderColor = canAfford
            ? ShopColor{120, 120, 220, 255} : ShopColor{60, 60, 60, 255};
        const ShopColor textColor = canAfford
            ? ShopColor{200, 200, 255, 255} : ShopColor{80, 80, 80, 255};

        r.fillRect(rRect.x, rRect.y, rRect.w, rRect.h,
                   fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        r.drawRectOutline(rRect.x, rRect.y, rRect.w, rRect.h,
                          borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        r.drawText("REROLL", rRect.x + 4.0f, rRect.y + 8.0f, 0.30f,
                   textColor.r, textColor.g, textColor.b, textColor.a);
        r.drawText("$" + std::to_string(m_runState->rerollCost),
                   rRect.x + 16.0f, rRect.y + 26.0f, 0.35f,
                   textColor.r, textColor.g, textColor.b, textColor.a);
    }

    {
        const ShopRect nRect = nextBlindButtonRect();
        r.fillRect(nRect.x, nRect.y, nRect.w, nRect.h, 80, 140, 255);
        r.drawRectOutline(nRect.x, nRect.y, nRect.w, nRect.h, 120, 180, 255);
        r.drawText("Next Blind", nRect.x + 8.0f, nRect.y + 18.0f, 0.42f, 0, 0, 0);
    }

    r.drawText("Inspect", 34.0f, 215.0f, 0.40f, 200, 200, 220);
    r.drawText("Buy / Sell", 120.0f, 215.0f, 0.40f, 200, 200, 220);
    r.drawText("Reroll", 220.0f, 215.0f, 0.40f, 200, 200, 220);
    r.drawText("Next Blind", 274.0f, 215.0f, 0.40f, 200, 200, 220);
}

#include "ShopState.h"
#include "GameplayState.h"
#include "states/ShopLayout.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include <algorithm>
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
    runState->advanceBlind();
    machine->changeState(std::make_shared<GameplayState>(machine, runState));
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
    m_items = generateShopItems(m_rng);
}

void ShopState::clearHeldInspect() {
    m_heldInspectIndex = -1;
}

bool ShopState::tryBuySelectedItem() {
    if (m_cursorIndex < 0 || m_cursorIndex >= static_cast<int>(m_items.size())) return false;
    if (m_runState->money < m_items[m_cursorIndex].price) return false;
    if (m_runState->jokers.size() >= static_cast<size_t>(m_runState->jokerLimit)) return false;

    clearHeldInspect();
    m_runState->money -= m_items[m_cursorIndex].price;
    m_runState->jokers.push_back(m_items[m_cursorIndex].joker);
    m_items.erase(m_items.begin() + m_cursorIndex);
    if (m_cursorIndex >= static_cast<int>(m_items.size()) && m_cursorIndex > 0) m_cursorIndex--;
    return true;
}

void ShopState::handleInput() {
    if (m_inputDelay > 0.0f) return;

#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_DLEFT) {
        m_cursorIndex--;
        if (m_cursorIndex < 0) m_cursorIndex = 0;
        clearHeldInspect();
    } else if (kDown & KEY_DRIGHT) {
        m_cursorIndex++;
        if (m_cursorIndex >= static_cast<int>(m_items.size())) m_cursorIndex = static_cast<int>(m_items.size()) - 1;
        clearHeldInspect();
    }

    if (kDown & KEY_A) {
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

        // Held joker slots: check first
        {
            int heldHit = hitHeldJoker(ShopPlatform::ThreeDS, static_cast<int>(m_runState->jokers.size()), touch.px, touch.py);
            if (heldHit >= 0) {
                m_heldInspectIndex = heldHit;
            }
        }

        // Buy Button: x: 20 -> 140, y: 160 -> 210
        if (touch.px >= 20 && touch.px <= 140 && touch.py >= 160 && touch.py <= 210) {
            tryBuySelectedItem();
        }
        // Next Ante Button: x: 160 -> 280, y: 160 -> 210
        else if (touch.px >= 160 && touch.px <= 280 && touch.py >= 160 && touch.py <= 210) {
            advanceToNextBlindAndResume(m_stateMachine, m_runState);
        }
    }
    
#else
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    static bool leftPressed = false, rightPressed = false, spacePressed = false, enterPressed = false;

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!leftPressed) {
            m_cursorIndex = std::max(0, m_cursorIndex - 1);
            clearHeldInspect();
            leftPressed = true;
        }
    } else { leftPressed = false; }

    if (keys[SDL_SCANCODE_RIGHT]) {
        if (!rightPressed) {
            m_cursorIndex = std::min(static_cast<int>(m_items.size()) - 1, m_cursorIndex + 1);
            clearHeldInspect();
            rightPressed = true;
        }
    } else { rightPressed = false; }

    if (keys[SDL_SCANCODE_SPACE]) {
        if (!spacePressed) {
            spacePressed = true;
            tryBuySelectedItem();
        }
    } else { spacePressed = false; }

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
            clearHeldInspect();
            if (!m_items.empty()) {
                int hit = hitShopCard(ShopPlatform::SDL, static_cast<int>(m_items.size()), mx, my);
                if (hit >= 0) m_cursorIndex = hit;
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
            if (mx < 400 && !m_items.empty()) {
                int hit = hitShopCard(ShopPlatform::SDL, static_cast<int>(m_items.size()), mx, my);
                if (hit >= 0) m_cursorIndex = hit;
            }
            // Bottom screen
            else if (mx >= 400) {
                // Held joker inspect: check before Buy/Next Blind
                int heldHit = hitHeldJoker(ShopPlatform::SDL, static_cast<int>(m_runState->jokers.size()), mx, my);
                if (heldHit >= 0) m_heldInspectIndex = heldHit;
                // Buy Button: x: 420 -> 540, y: 160 -> 210
                if (mx >= 420 && mx <= 540 && my >= 160 && my <= 210) {
                    tryBuySelectedItem();
                }
                // Next Ante Button: x: 560 -> 680, y: 160 -> 210
                else if (mx >= 560 && mx <= 680 && my >= 160 && my <= 210) {
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
    TextRenderer::drawText("SHOP", 140, 15, 0.7f, 0.7f, C2D_Color32(255, 180, 80, 255));
    TextRenderer::drawText("Money: $" + std::to_string(m_runState->money), 130, 45, 0.6f, 0.6f, C2D_Color32(255, 215, 0, 255));
    TextRenderer::drawText("Next: Ante " + std::to_string(m_runState->nextBlindAnte()) + " " + RunState::blindStageName(m_runState->nextBlindStage()),
                           70, 65, 0.4f, 0.4f, C2D_Color32(200, 200, 220, 255));
    TextRenderer::drawText("Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                           85, 83, 0.35f, 0.35f, C2D_Color32(255, 170, 120, 255));
    TextRenderer::drawText(RunState::bossModifierDescription(m_runState->nextBossModifier, m_runState->nextBlockedSuit),
                           46, 99, 0.3f, 0.3f, C2D_Color32(220, 220, 220, 255));
#else
    SDL_SetRenderDrawColor(renderer, 20, 30, 40, 255);
    SDL_Rect bgTop = { 0, 0, 400, 240 };
    SDL_RenderFillRect(renderer, &bgTop);
    
    TextRenderer::drawText(renderer, "SHOP", 160, 15, 2, 255, 180, 80);
    TextRenderer::drawText(renderer, "Money: $" + std::to_string(m_runState->money), 140, 45, 1, 255, 215, 0);
    TextRenderer::drawText(renderer, "Next: Ante " + std::to_string(m_runState->nextBlindAnte()) + " " + RunState::blindStageName(m_runState->nextBlindStage()),
                           90, 70, 0, 200, 200, 220);
    TextRenderer::drawText(renderer, "Boss: " + std::string(RunState::bossModifierName(m_runState->nextBossModifier)),
                           110, 88, 0, 255, 170, 120);
    TextRenderer::drawText(renderer,
                           RunState::bossModifierDescription(m_runState->nextBossModifier, m_runState->nextBlockedSuit),
                           75, 106, 0, 220, 220, 220);
#endif

    // ── Items for Sale ──
    const ShopPlatform platform =
#ifdef N3DS
        ShopPlatform::ThreeDS;
#else
        ShopPlatform::SDL;
#endif
    const int itemCount = static_cast<int>(m_items.size());

    for (size_t i = 0; i < m_items.size(); ++i) {
        const int index = static_cast<int>(i);
        const ShopRect body = shopCardBodyRect(platform, itemCount, index);
        const ShopRect highlight = shopCardHighlightRect(platform, itemCount, index);
        const ShopColor fillColor = jokerEffectColor(m_items[i].joker.effectType);

#ifdef N3DS
        if (i == m_cursorIndex) {
            C2D_DrawRectSolid(highlight.x, highlight.y, 0.5f, highlight.w, highlight.h, toC2DColor(kSelectedBorder));
        }

        C2D_DrawRectSolid(body.x, body.y, 0.5f, body.w, body.h, toC2DColor(fillColor));
        C2D_DrawRectSolid(body.x, body.y, 0.5f, body.w, 1, toC2DColor(kWhite));
        C2D_DrawRectSolid(body.x, body.y + body.h - 1, 0.5f, body.w, 1, toC2DColor(kWhite));
        C2D_DrawRectSolid(body.x, body.y, 0.5f, 1, body.h, toC2DColor(kWhite));
        C2D_DrawRectSolid(body.x + body.w - 1, body.y, 0.5f, 1, body.h, toC2DColor(kWhite));

        TextRenderer::drawText(m_items[i].joker.name, body.x + 5, body.y + 10, 0.45f, 0.45f, toC2DColor(kWhite));
        TextRenderer::drawText("$" + std::to_string(m_items[i].price), body.x + 30, body.y + 50, 0.5f, 0.5f, C2D_Color32(255, 215, 0, 255));
#else
        if (i == m_cursorIndex) {
            fillRect(renderer, highlight, kSelectedBorder);
        }

        fillRect(renderer, body, fillColor);
        drawRect(renderer, body, kWhite);

        TextRenderer::drawText(renderer, m_items[i].joker.name, body.x + 5, body.y + 10, 0, 255, 255, 255);
        TextRenderer::drawText(renderer, "$" + std::to_string(m_items[i].price), body.x + 35, body.y + 50, 1, 255, 215, 0);
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
        m_cursorIndex,
        static_cast<int>(m_items.size())
    );
    if (sel.source == InspectSource::HeldJoker) {
        TextRenderer::drawText(m_runState->jokers[sel.index].name, 10, 20, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText(m_runState->jokers[sel.index].description, 10, 45, 0.4f, 0.4f, C2D_Color32(180, 180, 180, 255));
    } else if (sel.source == InspectSource::ShopItem) {
        TextRenderer::drawText(m_items[sel.index].joker.name, 10, 20, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText(m_items[sel.index].joker.description, 10, 45, 0.4f, 0.4f, C2D_Color32(180, 180, 180, 255));
    } else {
        TextRenderer::drawText("Use D-pad to inspect", 10, 20, 0.4f, 0.4f, C2D_Color32(150, 150, 150, 255));
    }

    TextRenderer::drawText("Your Jokers: " + std::to_string(m_runState->jokers.size()) + "/" + std::to_string(m_runState->jokerLimit),
                           75, 80, 0.5f, 0.5f, C2D_Color32(200, 200, 200, 255));

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
    
    // Buy Button (Green)
    C2D_DrawRectSolid(baseX + 20, 160, 0.5f, 120, 50, C2D_Color32(80, 200, 80, 255));
    C2D_DrawRectSolid(baseX + 20, 160, 0.5f, 120, 2, C2D_Color32(120, 240, 120, 255)); // highlight
    
    std::string buyText = "Buy Item";
    if (m_cursorIndex >= 0 && m_cursorIndex < m_items.size()) {
        buyText = "Buy ($" + std::to_string(m_items[m_cursorIndex].price) + ")";
    }
    TextRenderer::drawText(buyText, baseX + 35, 175, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255));
    
    // Next Blind Button (Blue)
    C2D_DrawRectSolid(baseX + 160, 160, 0.5f, 120, 50, C2D_Color32(80, 140, 255, 255));
    C2D_DrawRectSolid(baseX + 160, 160, 0.5f, 120, 2, C2D_Color32(120, 180, 255, 255)); // highlight
    TextRenderer::drawText("Next Blind", baseX + 170, 175, 0.5f, 0.5f, C2D_Color32(0, 0, 0, 255));
    
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
        m_cursorIndex,
        static_cast<int>(m_items.size())
    );
    if (sel.source == InspectSource::HeldJoker) {
        TextRenderer::drawText(renderer, m_runState->jokers[sel.index].name, baseX + 10, 20, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, m_runState->jokers[sel.index].description, baseX + 10, 45, 0, 180, 180, 180);
    } else if (sel.source == InspectSource::ShopItem) {
        TextRenderer::drawText(renderer, m_items[sel.index].joker.name, baseX + 10, 20, 1, 255, 255, 255);
        TextRenderer::drawText(renderer, m_items[sel.index].joker.description, baseX + 10, 45, 0, 180, 180, 180);
    } else {
        TextRenderer::drawText(renderer, "Hover a card to inspect", baseX + 10, 20, 0, 150, 150, 150);
    }

    TextRenderer::drawText(renderer,
                           "Your Jokers: " + std::to_string(m_runState->jokers.size()) + "/" + std::to_string(m_runState->jokerLimit),
                           baseX + 80, 80, 1, 200, 200, 200);

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
    
    // Buy Button (Green)
    SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
    SDL_Rect buyRect = { baseX + 20, 160, 120, 50 };
    SDL_RenderFillRect(renderer, &buyRect);
    
    std::string buyText = "Buy Item";
    if (m_cursorIndex >= 0 && m_cursorIndex < m_items.size()) {
        buyText = "Buy ($" + std::to_string(m_items[m_cursorIndex].price) + ")";
    }
    TextRenderer::drawText(renderer, buyText, baseX + 35, 175, 1, 0, 0, 0);

    // Next Blind Button (Blue)
    SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
    SDL_Rect nextRect = { baseX + 160, 160, 120, 50 };
    SDL_RenderFillRect(renderer, &nextRect);
    TextRenderer::drawText(renderer, "Next Blind", baseX + 170, 175, 1, 0, 0, 0);
    
    // Hints
    TextRenderer::drawText(renderer, "[Space]", baseX + 60, 215, 0, 200, 200, 220);
    TextRenderer::drawText(renderer, "[Enter]", baseX + 200, 215, 0, 200, 200, 220);
#endif
}

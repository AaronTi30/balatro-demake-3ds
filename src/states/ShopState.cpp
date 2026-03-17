#include "ShopState.h"
#include "GameplayState.h"
#include "../core/StateMachine.h"
#include "../core/Application.h"
#include "../core/TextRenderer.h"
#include <algorithm>
#include <random>

#ifdef N3DS
#include <3ds.h>
#include <citro2d.h>
#else
#include <SDL.h>
#endif

ShopState::ShopState(StateMachine* machine, int nextAnte, int currentMoney, const std::vector<Joker>& currentJokers)
    : m_nextAnte(nextAnte), m_money(currentMoney), m_jokers(currentJokers), m_cursorIndex(0) {
    m_stateMachine = machine;
}

void ShopState::enter() {
    generateItems();
}

void ShopState::exit() {}

void ShopState::update(float dt) {}

void ShopState::generateItems() {
    m_items.clear();
    // Generate 2 random jokers for the shop
    for (int i = 0; i < 2; ++i) {
        ShopItem item;
        item.joker = Joker::getRandom();
        // Base price calculation (very rough for now)
        int basePrice = 4;
        if (item.joker.effectType == JokerEffectType::MulMult) basePrice += 2;
        if (item.joker.effectValue > 10) basePrice += 1;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(-1, 2);
        
        item.price = basePrice + dis(gen);
        m_items.push_back(item);
    }
}

void ShopState::handleInput() {
#ifdef N3DS
    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_DLEFT) {
        m_cursorIndex--;
        if (m_cursorIndex < 0) m_cursorIndex = 0;
    } else if (kDown & KEY_DRIGHT) {
        m_cursorIndex++;
        if (m_cursorIndex >= m_items.size()) m_cursorIndex = m_items.size() - 1;
    }
    
    if (kDown & KEY_A) {
        // Buy item
        if (m_cursorIndex >= 0 && m_cursorIndex < m_items.size()) {
            if (m_money >= m_items[m_cursorIndex].price && m_jokers.size() < 5) {
                m_money -= m_items[m_cursorIndex].price;
                m_jokers.push_back(m_items[m_cursorIndex].joker);
                m_items.erase(m_items.begin() + m_cursorIndex);
                if (m_cursorIndex >= m_items.size() && m_cursorIndex > 0) {
                    m_cursorIndex--;
                }
            }
        }
    }
    
    if (kDown & KEY_START || kDown & KEY_X) {
        // Next Round
        m_stateMachine->changeState(std::make_shared<GameplayState>(m_stateMachine, m_nextAnte, m_money, m_jokers));
    }
    
#else
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    static bool leftPressed = false, rightPressed = false, spacePressed = false, enterPressed = false;

    if (keys[SDL_SCANCODE_LEFT]) {
        if (!leftPressed) { m_cursorIndex = std::max(0, m_cursorIndex - 1); leftPressed = true; }
    } else { leftPressed = false; }

    if (keys[SDL_SCANCODE_RIGHT]) {
        if (!rightPressed) { m_cursorIndex = std::min(static_cast<int>(m_items.size()) - 1, m_cursorIndex + 1); rightPressed = true; }
    } else { rightPressed = false; }

    if (keys[SDL_SCANCODE_SPACE]) {
        if (!spacePressed) {
            spacePressed = true;
            if (m_cursorIndex >= 0 && m_cursorIndex < m_items.size()) {
                if (m_money >= m_items[m_cursorIndex].price && m_jokers.size() < 5) {
                    m_money -= m_items[m_cursorIndex].price;
                    m_jokers.push_back(m_items[m_cursorIndex].joker);
                    m_items.erase(m_items.begin() + m_cursorIndex);
                    if (m_cursorIndex >= m_items.size() && m_cursorIndex > 0) {
                        m_cursorIndex--;
                    }
                }
            }
        }
    } else { spacePressed = false; }

    if (keys[SDL_SCANCODE_RETURN]) {
        if (!enterPressed) {
            enterPressed = true;
            m_stateMachine->changeState(std::make_shared<GameplayState>(m_stateMachine, m_nextAnte, m_money, m_jokers));
        }
    } else { enterPressed = false; }
#endif
}

void ShopState::renderTopScreen(Application* app) {
#ifndef N3DS
    SDL_Renderer* renderer = app->getRenderer();
#endif

    // ── HUD ──
#ifdef N3DS
    TextRenderer::drawText("SHOP", 160, 20, 0.7f, 0.7f, C2D_Color32(255, 180, 80, 255));
    TextRenderer::drawText("Money: $" + std::to_string(m_money), 150, 60, 0.5f, 0.5f, C2D_Color32(255, 215, 0, 255));
#else
    TextRenderer::drawText(renderer, "SHOP", 180, 20, 2, 255, 180, 80);
    TextRenderer::drawText(renderer, "Money: $" + std::to_string(m_money), 180, 60, 1, 255, 215, 0);
#endif

    // ── Items for Sale ──
    int startX = 60;
    
    for (size_t i = 0; i < m_items.size(); ++i) {
        int x = startX + i * 140;
        int y = 100;
        
#ifdef N3DS
        u32 color = C2D_Color32(100, 100, 100, 255);
        if (m_items[i].joker.effectType == JokerEffectType::AddChips) color = C2D_Color32(80, 120, 220, 255);
        else if (m_items[i].joker.effectType == JokerEffectType::AddMult) color = C2D_Color32(220, 60, 60, 255);
        else if (m_items[i].joker.effectType == JokerEffectType::MulMult) color = C2D_Color32(180, 60, 220, 255);

        // Selection highlight
        if (i == m_cursorIndex) {
            C2D_DrawRectSolid(x - 5, y - 5, 0.5f, 100, 120, C2D_Color32(255, 255, 100, 255));
        }

        // Card body
        C2D_DrawRectSolid(x, y, 0.5f, 90, 110, color);
        C2D_DrawRectSolid(x, y, 0.5f, 90, 1, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(x, y + 109, 0.5f, 90, 1, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(x, y, 0.5f, 1, 110, C2D_Color32(255, 255, 255, 255));
        C2D_DrawRectSolid(x + 89, y, 0.5f, 1, 110, C2D_Color32(255, 255, 255, 255));
        
        // Text
        TextRenderer::drawText(m_items[i].joker.name, x + 5, y + 10, 0.45f, 0.45f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText(m_items[i].joker.description, x + 10, y + 50, 0.4f, 0.4f, C2D_Color32(255, 255, 255, 255));
        TextRenderer::drawText("$" + std::to_string(m_items[i].price), x + 30, y + 90, 0.5f, 0.5f, C2D_Color32(255, 215, 0, 255));
#else
        int r = 100, g = 100, b = 100;
        if (m_items[i].joker.effectType == JokerEffectType::AddChips) { r = 80; g = 120; b = 220; }
        else if (m_items[i].joker.effectType == JokerEffectType::AddMult) { r = 220; g = 60; b = 60; }
        else if (m_items[i].joker.effectType == JokerEffectType::MulMult) { r = 180; g = 60; b = 220; }

        if (i == m_cursorIndex) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
            SDL_Rect hbox = { x - 5, y - 5, 110, 130 };
            SDL_RenderFillRect(renderer, &hbox);
        }

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect jbox = { x, y, 100, 120 };
        SDL_RenderFillRect(renderer, &jbox);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &jbox);

        TextRenderer::drawText(renderer, m_items[i].joker.name, x + 5, y + 10, 0, 255, 255, 255);
        TextRenderer::drawText(renderer, m_items[i].joker.description, x + 10, y + 50, 0, 255, 255, 255);
        TextRenderer::drawText(renderer, "$" + std::to_string(m_items[i].price), x + 35, y + 95, 1, 255, 215, 0);
#endif
    }
}

void ShopState::renderBottomScreen(Application* app) {
#ifdef N3DS
    TextRenderer::drawText("SHOP CONTROLS", 100, 30, 0.5f, 0.5f, C2D_Color32(200, 200, 220, 255));
    TextRenderer::drawText("[D-Pad] Select Item", 20, 80, 0.45f, 0.45f, C2D_Color32(255, 255, 255, 255));
    TextRenderer::drawText("[A] Buy selected", 20, 110, 0.45f, 0.45f, C2D_Color32(100, 220, 100, 255));
    TextRenderer::drawText("[Start] Next Ante", 20, 140, 0.45f, 0.45f, C2D_Color32(255, 180, 80, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    TextRenderer::drawText(renderer, "SHOP CONTROLS", 480, 30, 1, 200, 200, 220);
    TextRenderer::drawText(renderer, "[Arrows] Select Item", 420, 80, 0, 255, 255, 255);
    TextRenderer::drawText(renderer, "[Space] Buy selected", 420, 110, 0, 100, 220, 100);
    TextRenderer::drawText(renderer, "[Enter] Next Ante", 420, 140, 0, 255, 180, 80);
#endif
}

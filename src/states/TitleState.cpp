#include "TitleState.h"

#include "GameplayState.h"
#include "../core/Application.h"
#include "../core/AssetPath.h"
#include "../core/StateMachine.h"
#include "../core/TextRenderer.h"
#include "../game/CardRenderer.h"
#include "../game/RunState.h"

#include <memory>

#ifdef N3DS
#include <citro2d.h>
#else
#include <SDL.h>
#endif

namespace {

constexpr int kBackgroundR = 0x1a;
constexpr int kBackgroundG = 0x0a;
constexpr int kBackgroundB = 0x14;

#ifndef N3DS
struct TitleRect {
    int x;
    int y;
    int w;
    int h;
};

enum class TitleButton {
    Collection,
    Options,
    Quit,
    Play
};

struct ButtonStyle {
    SDL_Color shadow;
    SDL_Color body;
    SDL_Color highlight;
    SDL_Color text;
    int textSize;
};

constexpr TitleRect kTopScreenRect{0, 0, 400, 240};
constexpr TitleRect kBottomScreenRect{400, 0, 320, 240};
constexpr TitleRect kLogoRect{80, 4, 240, 156};
constexpr TitleRect kAceRect{160, 124, 80, 112};
constexpr TitleRect kPlayRect{480, 74, 160, 44};
constexpr TitleRect kOptionsRect{422, 134, 74, 34};
constexpr TitleRect kQuitRect{604, 134, 74, 34};
constexpr TitleRect kCollectionRect{480, 184, 160, 44};

TitleRect buttonRect(TitleButton button) {
    switch (button) {
        case TitleButton::Collection: return kCollectionRect;
        case TitleButton::Options: return kOptionsRect;
        case TitleButton::Quit: return kQuitRect;
        case TitleButton::Play: return kPlayRect;
        default: return kPlayRect;
    }
}

ButtonStyle buttonStyle(TitleButton button) {
    switch (button) {
        case TitleButton::Collection:
            return {
                {14, 52, 28, 255},
                {0x1E, 0x6B, 0x3A, 255},
                {62, 144, 85, 255},
                {255, 255, 255, 255},
                1
            };
        case TitleButton::Options:
            return {
                {78, 35, 12, 255},
                {0x9B, 0x4A, 0x1A, 255},
                {197, 111, 56, 255},
                {255, 255, 255, 255},
                1
            };
        case TitleButton::Quit:
            return {
                {61, 12, 12, 255},
                {0x8B, 0x20, 0x20, 255},
                {183, 76, 76, 255},
                {255, 245, 245, 255},
                1
            };
        case TitleButton::Play:
            return {
                {22, 49, 81, 255},
                {0x2B, 0x5F, 0x9E, 255},
                {91, 138, 196, 255},
                {255, 255, 255, 255},
                1
            };
        default:
            return {
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {0, 0, 0, 255},
                {255, 255, 255, 255},
                1
            };
    }
}

bool pointInRect(const TitleRect& rect, int px, int py) {
    return px >= rect.x && px < rect.x + rect.w && py >= rect.y && py < rect.y + rect.h;
}

void fillRect(SDL_Renderer* renderer, const TitleRect& rect, const SDL_Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect sdlRect = { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(renderer, &sdlRect);
}

void drawButton(SDL_Renderer* renderer,
                const TitleRect& rect,
                const char* label,
                const ButtonStyle& style) {
    const TitleRect shadow = { rect.x + 3, rect.y + 4, rect.w, rect.h };
    fillRect(renderer, shadow, style.shadow);
    fillRect(renderer, rect, style.body);

    const TitleRect highlight = { rect.x + 3, rect.y + 3, rect.w - 6, rect.h - 10 };
    fillRect(renderer, highlight, style.highlight);

    const int approxCharWidth = (style.textSize == 0) ? 4 : 5;
    const int labelLength = static_cast<int>(SDL_strlen(label));
    const int textX = rect.x + (rect.w / 2) - (labelLength * approxCharWidth);
    const int textY = rect.y + ((style.textSize == 0) ? 15 : 13);
    TextRenderer::drawText(renderer,
                           label,
                           textX,
                           textY,
                           style.textSize,
                           style.text.r,
                           style.text.g,
                           style.text.b);
}
#endif

} // namespace

TitleState::TitleState(StateMachine* machine) {
    m_stateMachine = machine;
#ifndef N3DS
    m_logoTex = nullptr;
    m_mouseWasPressed = false;
#endif
}

TitleState::~TitleState() {
#ifndef N3DS
    destroyDesktopAssets();
#endif
}

void TitleState::enter() {
#ifndef N3DS
    m_mouseWasPressed = false;
#endif
}

void TitleState::exit() {
#ifndef N3DS
    destroyDesktopAssets();
#endif
}

void TitleState::startRun() {
    auto runState = std::make_shared<RunState>();
    runState->startNewRun();
    m_stateMachine->changeState(
        std::make_shared<GameplayState>(m_stateMachine, runState));
}

#ifndef N3DS
void TitleState::ensureDesktopAssets(Application* app) {
    if (!m_logoTex) {
        m_logoTex = sdlLoadTexture(app->getRenderer(), "assets/textures/balatro.png");
    }
}

void TitleState::destroyDesktopAssets() {
    if (m_logoTex && SDL_WasInit(SDL_INIT_VIDEO) != 0) {
        SDL_DestroyTexture(m_logoTex);
    }
    m_logoTex = nullptr;
}
#endif

void TitleState::handleInput() {
#ifdef N3DS
    hidScanInput();
    const u32 kDown = hidKeysDown();
    if (kDown & KEY_A) {
        startRun();
    }
#else
    int mx = 0;
    int my = 0;
    const Uint32 buttons = SDL_GetMouseState(&mx, &my);
    const bool isPressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (isPressed && !m_mouseWasPressed) {
        if (pointInRect(buttonRect(TitleButton::Collection), mx, my) ||
            pointInRect(buttonRect(TitleButton::Options), mx, my)) {
            m_mouseWasPressed = isPressed;
            return;
        }

        if (pointInRect(buttonRect(TitleButton::Play), mx, my)) {
            startRun();
            m_mouseWasPressed = isPressed;
            return;
        }

        if (pointInRect(buttonRect(TitleButton::Quit), mx, my)) {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }
    }

    m_mouseWasPressed = isPressed;
#endif
}

void TitleState::update(float dt) {
    (void)dt;
}

void TitleState::renderTopScreen(Application* app) {
#ifdef N3DS
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, C2D_Color32(kBackgroundR, kBackgroundG, kBackgroundB, 255));
    TextRenderer::drawText("BALATRO", 118, 70, 0.75f, 0.75f, C2D_Color32(240, 220, 180, 255));
    TextRenderer::drawText("DEMAKE", 142, 104, 0.55f, 0.55f, C2D_Color32(215, 215, 230, 255));
    TextRenderer::drawText("Press A to start", 120, 162, 0.42f, 0.42f, C2D_Color32(200, 200, 210, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    fillRect(renderer, kTopScreenRect, SDL_Color{ kBackgroundR, kBackgroundG, kBackgroundB, 255 });

    ensureDesktopAssets(app);
    if (m_logoTex) {
        SDL_Rect logoDst = { kLogoRect.x, kLogoRect.y, kLogoRect.w, kLogoRect.h };
        SDL_RenderCopy(renderer, m_logoTex, nullptr, &logoDst);
    }

    CardRenderer::init(app);
    const auto renderPlan = CardRenderer::desktopRenderPlan({ Suit::Spades, Rank::Ace });
    SDL_Texture* cardBaseTexture = CardRenderer::getCardBaseTexture();
    SDL_Texture* cardsTexture = CardRenderer::getCardsTexture();
    const SDL_Rect dstRect = { kAceRect.x, kAceRect.y, kAceRect.w, kAceRect.h };

    if (cardBaseTexture) {
        SDL_Rect baseSrcRect = {
            renderPlan.baseSource.x,
            renderPlan.baseSource.y,
            renderPlan.baseSource.w,
            renderPlan.baseSource.h
        };
        SDL_RenderCopy(renderer, cardBaseTexture, &baseSrcRect, &dstRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
        SDL_RenderFillRect(renderer, &dstRect);
        SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
        SDL_RenderDrawRect(renderer, &dstRect);
    }

    if (cardsTexture) {
        const auto source = renderPlan.overlaySource;
        SDL_Rect srcRect = { source.x, source.y, source.w, source.h };
        SDL_RenderCopy(renderer, cardsTexture, &srcRect, &dstRect);
    }
#endif
}

void TitleState::renderBottomScreen(Application* app) {
#ifdef N3DS
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 240, C2D_Color32(kBackgroundR, kBackgroundG, kBackgroundB, 255));
    C2D_DrawRectSolid(70, 156, 0.5f, 180, 44, C2D_Color32(54, 144, 74, 255));
    C2D_DrawRectSolid(73, 159, 0.5f, 174, 34, C2D_Color32(104, 208, 128, 255));
    TextRenderer::drawText("PLAY", 136, 170, 0.55f, 0.55f, C2D_Color32(255, 255, 255, 255));
    TextRenderer::drawText("Press A to start a run", 78, 98, 0.38f, 0.38f, C2D_Color32(210, 210, 220, 255));
#else
    SDL_Renderer* renderer = app->getRenderer();
    fillRect(renderer, kBottomScreenRect, SDL_Color{ kBackgroundR, kBackgroundG, kBackgroundB, 255 });

    drawButton(renderer, buttonRect(TitleButton::Collection), "COLLECTION", buttonStyle(TitleButton::Collection));
    drawButton(renderer, buttonRect(TitleButton::Options), "OPTIONS", buttonStyle(TitleButton::Options));
    drawButton(renderer, buttonRect(TitleButton::Quit), "QUIT", buttonStyle(TitleButton::Quit));
    drawButton(renderer, buttonRect(TitleButton::Play), "PLAY", buttonStyle(TitleButton::Play));
#endif
}

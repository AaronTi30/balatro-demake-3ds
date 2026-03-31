#include "core/TextRenderer.h"

#include <SDL.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

struct RenderStats {
    int width = 0;
    int height = 0;
    int drawnPixels = 0;
    int minX = 0;
    int maxX = -1;
    int minY = 0;
    int maxY = -1;
    int maxAlpha = 0;
};

int drawnHeight(const RenderStats& stats) {
    if (stats.maxY < stats.minY) {
        return 0;
    }
    return stats.maxY - stats.minY + 1;
}

RenderStats captureRenderStats(SDL_Renderer* renderer, SDL_Surface* surface) {
    RenderStats stats;
    stats.width = surface->w;
    stats.height = surface->h;

    std::vector<Uint8> pixels(static_cast<size_t>(surface->pitch) * static_cast<size_t>(surface->h));
    expectEqual(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, pixels.data(), surface->pitch),
                0,
                "read rendered pixels");

    const Uint32* rows = reinterpret_cast<const Uint32*>(pixels.data());
    for (int y = 0; y < surface->h; ++y) {
        const Uint32* row = reinterpret_cast<const Uint32*>(reinterpret_cast<const Uint8*>(rows) + static_cast<size_t>(y) * surface->pitch);
        for (int x = 0; x < surface->w; ++x) {
            const Uint32 pixel = row[x];
            const Uint8 alpha = static_cast<Uint8>((pixel >> 24) & 0xFFu);
            if (alpha == 0) {
                continue;
            }

            ++stats.drawnPixels;
            if (stats.maxX < stats.minX) {
                stats.minX = x;
                stats.maxX = x;
                stats.minY = y;
                stats.maxY = y;
            } else {
                if (x < stats.minX) stats.minX = x;
                if (x > stats.maxX) stats.maxX = x;
                if (y < stats.minY) stats.minY = y;
                if (y > stats.maxY) stats.maxY = y;
            }
            if (alpha > stats.maxAlpha) {
                stats.maxAlpha = alpha;
            }
        }
    }

    return stats;
}

void clearRenderer(SDL_Renderer* renderer) {
    expectEqual(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0), 0, "clear color setup");
    expectEqual(SDL_RenderClear(renderer), 0, "renderer clear");
}

void testDesktopScaleMapping() {
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.44f), 0, "small scale threshold");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.45f), 1, "medium lower bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.62f), 1, "medium upper bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.63f), 2, "large scale threshold");
}

void testSharedDesktopDrawTextUsesThresholdMapping() {
    expect(SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy"), "SDL dummy video driver hint");
    expectEqual(SDL_Init(SDL_INIT_VIDEO), 0, "SDL init");
    expect(TextRenderer::init(), "TextRenderer init");

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 96, 48, 32, SDL_PIXELFORMAT_ARGB8888);
    expect(surface != nullptr, "software render target surface");

    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    expect(renderer != nullptr, "software renderer");

    TextRenderer::setRenderer(renderer);

    clearRenderer(renderer);
    TextRenderer::drawText("H", 4.0f, 4.0f, 0.44f, 255, 255, 255);
    const RenderStats small = captureRenderStats(renderer, surface);

    clearRenderer(renderer);
    TextRenderer::drawText("H", 4.0f, 4.0f, 0.45f, 255, 255, 255);
    const RenderStats mediumLower = captureRenderStats(renderer, surface);

    clearRenderer(renderer);
    TextRenderer::drawText("H", 4.0f, 4.0f, 0.62f, 255, 255, 255);
    const RenderStats mediumUpper = captureRenderStats(renderer, surface);

    clearRenderer(renderer);
    TextRenderer::drawText("H", 4.0f, 4.0f, 0.63f, 255, 255, 255);
    const RenderStats large = captureRenderStats(renderer, surface);

    expect(small.drawnPixels > 0, "small shared drawText should render");
    expect(mediumLower.drawnPixels > 0, "medium lower bound shared drawText should render");
    expect(mediumUpper.drawnPixels > 0, "medium upper bound shared drawText should render");
    expect(large.drawnPixels > 0, "large shared drawText should render");
    expect(drawnHeight(small) < drawnHeight(mediumLower), "0.44f should map to the smaller font tier");
    expect(drawnHeight(mediumLower) == drawnHeight(mediumUpper), "0.45f and 0.62f should map to the same medium tier");
    expect(drawnHeight(mediumUpper) < drawnHeight(large), "0.63f should map to the larger font tier");

    TextRenderer::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
}

void testSharedDesktopDrawTextPreservesAlpha() {
    expect(SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy"), "SDL dummy video driver hint");
    expectEqual(SDL_Init(SDL_INIT_VIDEO), 0, "SDL init");
    expect(TextRenderer::init(), "TextRenderer init");

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 96, 48, 32, SDL_PIXELFORMAT_ARGB8888);
    expect(surface != nullptr, "software render target surface");

    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    expect(renderer != nullptr, "software renderer");

    TextRenderer::setRenderer(renderer);

    clearRenderer(renderer);
    TextRenderer::drawText("A", 4.0f, 4.0f, 0.63f, 255, 255, 255, 255);
    const RenderStats opaque = captureRenderStats(renderer, surface);

    clearRenderer(renderer);
    TextRenderer::drawText("A", 4.0f, 4.0f, 0.63f, 255, 255, 255, 64);
    const RenderStats translucent = captureRenderStats(renderer, surface);

    expect(opaque.maxAlpha > translucent.maxAlpha, "alpha should affect shared desktop output");
    expect(translucent.maxAlpha > 0, "translucent draw should still render visible pixels");

    TextRenderer::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
}

} // namespace

int main() {
    testDesktopScaleMapping();
    testSharedDesktopDrawTextUsesThresholdMapping();
    testSharedDesktopDrawTextPreservesAlpha();
    return 0;
}

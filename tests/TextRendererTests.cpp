#include "core/TextRenderer.h"

#include <SDL.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

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

void testDesktopScaleMapping() {
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.44f), 0, "small scale threshold");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.45f), 1, "medium lower bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.62f), 1, "medium upper bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.63f), 2, "large scale threshold");
}

void testSharedDesktopDrawTextRendersWithBoundRenderer() {
    expect(SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy"), "SDL dummy video driver hint");
    expectEqual(SDL_Init(SDL_INIT_VIDEO), 0, "SDL init");
    expect(TextRenderer::init(), "TextRenderer init");

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 96, 48, 32, SDL_PIXELFORMAT_ARGB8888);
    expect(surface != nullptr, "software render target surface");

    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    expect(renderer != nullptr, "software renderer");

    TextRenderer::setRenderer(renderer);

    expectEqual(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0), 0, "clear color setup");
    expectEqual(SDL_RenderClear(renderer), 0, "renderer clear");

    TextRenderer::drawText("A", 4.0f, 4.0f, 0.63f, 255, 255, 255);
    SDL_RenderPresent(renderer);

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    const int pixelCount = surface->w * surface->h;
    bool foundNonZeroPixel = false;
    for (int i = 0; i < pixelCount; ++i) {
        if (pixels[i] != 0) {
            foundNonZeroPixel = true;
            break;
        }
    }
    expect(foundNonZeroPixel, "shared desktop drawText should render non-empty output");

    TextRenderer::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
}

} // namespace

int main() {
    testDesktopScaleMapping();
    testSharedDesktopDrawTextRendersWithBoundRenderer();
    return 0;
}

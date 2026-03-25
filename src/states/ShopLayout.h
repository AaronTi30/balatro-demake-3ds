#pragma once

#include "../game/Joker.h"

enum class ShopPlatform { SDL, ThreeDS };

struct ShopRect { int x, y, w, h; };
struct ShopColor { int r, g, b, a; };

// Stride between shop card slots (body + gap). Fixed at 140px on both platforms.
static constexpr int kShopCardStride = 140;

struct ShopCardLayout {
    int startX, y, bodyWidth, bodyHeight, highlightPad, priceOffsetY;
};

struct HeldJokerRowLayout {
    // startX is an absolute pixel coordinate on both platforms.
    // SDL: 412 (= bottom-screen base 400 + centered offset 12). Use directly — do NOT add baseX again.
    // 3DS: 12 (centered in 320px bottom screen).
    int startX, y, cardWidth, cardHeight, stride;
};

enum class InspectSource { Placeholder, ShopItem, HeldJoker };

struct InspectSelection { InspectSource source; int index; };

ShopCardLayout shopCardLayout(ShopPlatform platform, int itemCount);
ShopRect shopCardBodyRect(ShopPlatform platform, int itemCount, int index);
ShopRect shopCardHighlightRect(ShopPlatform platform, int itemCount, int index);

HeldJokerRowLayout heldJokerRowLayout(ShopPlatform platform);
ShopRect heldJokerSlotRect(ShopPlatform platform, int index);
ShopRect buyButtonRect(ShopPlatform platform);
ShopRect nextBlindButtonRect(ShopPlatform platform);
bool hitBuyButton(ShopPlatform platform, int px, int py);
bool hitNextBlindButton(ShopPlatform platform, int px, int py);
ShopColor jokerEffectColor(JokerEffectType effectType);

int hitShopCard(ShopPlatform platform, int itemCount, int px, int py);
int hitHeldJoker(ShopPlatform platform, int jokerCount, int px, int py);

InspectSelection resolveInspectSelection(int heldInspectIndex, int heldCount,
                                         int cursorIndex, int itemCount);

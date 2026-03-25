#pragma once

enum class ShopPlatform { SDL, N3DS };

struct ShopRect { int x, y, w, h; };

struct ShopCardLayout {
    int startX, y, bodyWidth, bodyHeight, highlightPad, priceOffsetY;
};

struct HeldJokerRowLayout {
    int startX, y, cardWidth, cardHeight, stride;
};

enum class InspectSource { Placeholder, ShopItem, HeldJoker };

struct InspectSelection { InspectSource source; int index; };

ShopCardLayout shopCardLayout(ShopPlatform platform, int itemCount);
ShopRect shopCardBodyRect(ShopPlatform platform, int itemCount, int index);
ShopRect shopCardHighlightRect(ShopPlatform platform, int itemCount, int index);

HeldJokerRowLayout heldJokerRowLayout(ShopPlatform platform);
ShopRect heldJokerSlotRect(ShopPlatform platform, int index);

int hitShopCard(ShopPlatform platform, int itemCount, int px, int py);
int hitHeldJoker(ShopPlatform platform, int jokerCount, int px, int py);

InspectSelection resolveInspectSelection(int heldInspectIndex, int heldCount,
                                         int cursorIndex, int itemCount);

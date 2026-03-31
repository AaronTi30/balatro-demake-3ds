#pragma once

#include <array>

#include "../game/ShopOffer.h"

struct ShopRect { int x, y, w, h; };
struct ShopColor { int r, g, b, a; };

static constexpr int kVisibleShopSlots = 3;

struct ShopCardLayout {
    int startX, y, bodyWidth, bodyHeight, highlightPad, priceOffsetY, stride;
};

struct HeldJokerRowLayout {
    // startX is an absolute 3DS-native pixel coordinate on the bottom screen.
    int startX, y, cardWidth, cardHeight, stride;
};

enum class InspectSource { Placeholder, ShopItem, HeldJoker };

struct InspectSelection { InspectSource source; int index; };

ShopCardLayout shopCardLayout(int itemCount);
ShopRect shopCardBodyRect(int itemCount, int index);
ShopRect shopCardHighlightRect(int itemCount, int index);

HeldJokerRowLayout heldJokerRowLayout();
ShopRect heldJokerSlotRect(int index);
ShopRect buyButtonRect();
ShopRect nextBlindButtonRect();
bool hitBuyButton(int px, int py);
bool hitNextBlindButton(int px, int py);
ShopRect rerollButtonRect();
bool hitRerollButton(int px, int py);
ShopColor jokerEffectColor(JokerEffectType effectType);

int hitShopCard(int itemCount, int px, int py);
int hitHeldJoker(int jokerCount, int px, int py);

bool isSelectableShopSlot(int slotIndex, const std::array<bool, kVisibleShopSlots>& soldSlots);
int nextSelectableShopSlot(int currentSlot, int direction,
                           const std::array<bool, kVisibleShopSlots>& soldSlots);

InspectSelection resolveInspectSelection(int heldInspectIndex, int heldCount,
                                         int cursorIndex, int itemCount);

namespace shop_layout_helpers {
bool isSlotDisabled(const ShopSlot& slot);
const char* blockedShopSlotLabel(const ShopSlot& slot);
std::array<bool, kVisibleShopSlots> disabledShopSlots(const std::array<ShopSlot, kVisibleShopSlots>& slots);
int markShopSlotSoldAndAdvanceCursor(std::array<ShopSlot, kVisibleShopSlots>& slots, int purchasedSlot);
} // namespace shop_layout_helpers

#include "states/ShopLayout.h"

namespace {

bool pointInRect(const ShopRect& rect, int px, int py) {
    return px >= rect.x && px < rect.x + rect.w &&
           py >= rect.y && py < rect.y + rect.h;
}

} // namespace

ShopCardLayout shopCardLayout(int itemCount) {
    (void)itemCount;

    return {15, 125, 80, 70, 5, 50, 100};
}

ShopRect shopCardBodyRect(int itemCount, int index) {
    const ShopCardLayout layout = shopCardLayout(itemCount);
    return {
        layout.startX + index * layout.stride,
        layout.y,
        layout.bodyWidth,
        layout.bodyHeight
    };
}

ShopRect shopCardHighlightRect(int itemCount, int index) {
    const ShopRect body = shopCardBodyRect(itemCount, index);
    const int pad = shopCardLayout(itemCount).highlightPad;
    return {
        body.x - pad,
        body.y - pad,
        body.w + pad * 2,
        body.h + pad * 2
    };
}

HeldJokerRowLayout heldJokerRowLayout() {
    return {
        12,
        90,
        55,
        60,
        60
    };
}

ShopRect heldJokerSlotRect(int index) {
    const HeldJokerRowLayout layout = heldJokerRowLayout();
    return {
        layout.startX + index * layout.stride,
        layout.y,
        layout.cardWidth,
        layout.cardHeight
    };
}

ShopRect buyButtonRect() {
    return {
        20,
        160,
        100,
        50
    };
}

ShopRect nextBlindButtonRect() {
    return {
        215,
        160,
        95,
        50
    };
}

bool hitBuyButton(int px, int py) {
    return pointInRect(buyButtonRect(), px, py);
}

bool hitNextBlindButton(int px, int py) {
    return pointInRect(nextBlindButtonRect(), px, py);
}

ShopRect rerollButtonRect() {
    return {
        130,
        160,
        75,
        50
    };
}

bool hitRerollButton(int px, int py) {
    return pointInRect(rerollButtonRect(), px, py);
}

ShopColor jokerEffectColor(JokerEffectType effectType) {
    switch (effectType) {
        case JokerEffectType::AddChips:
            return {80, 120, 220, 255};
        case JokerEffectType::AddMult:
            return {220, 60, 60, 255};
        case JokerEffectType::MulMult:
            return {180, 60, 220, 255};
    }

    return {100, 100, 100, 255};
}

bool isSelectableShopSlot(int slotIndex, const std::array<bool, kVisibleShopSlots>& soldSlots) {
    return slotIndex >= 0 &&
           slotIndex < kVisibleShopSlots &&
           !soldSlots[slotIndex];
}

int nextSelectableShopSlot(int currentSlot, int direction,
                           const std::array<bool, kVisibleShopSlots>& soldSlots) {
    for (int step = 0; step < kVisibleShopSlots; ++step) {
        const int candidate = currentSlot + direction * step;
        if (isSelectableShopSlot(candidate, soldSlots)) {
            return candidate;
        }
    }

    for (int candidate = 0; candidate < kVisibleShopSlots; ++candidate) {
        if (isSelectableShopSlot(candidate, soldSlots)) {
            return candidate;
        }
    }

    return -1;
}

int hitShopCard(int itemCount, int px, int py) {
    const ShopCardLayout layout = shopCardLayout(itemCount);
    if (py < layout.y || py >= layout.y + layout.bodyHeight) {
        return -1;
    }
    for (int i = 0; i < itemCount; ++i) {
        const int cardX = layout.startX + i * layout.stride;
        if (px >= cardX && px < cardX + layout.bodyWidth) {
            return i;
        }
    }
    return -1;
}

int hitHeldJoker(int jokerCount, int px, int py) {
    const HeldJokerRowLayout layout = heldJokerRowLayout();
    if (py < layout.y || py >= layout.y + layout.cardHeight) {
        return -1;
    }
    for (int i = 0; i < jokerCount; ++i) {
        const int slotX = layout.startX + i * layout.stride;
        if (px >= slotX && px < slotX + layout.cardWidth) {
            return i;
        }
    }
    return -1;
}

InspectSelection resolveInspectSelection(int heldInspectIndex, int heldCount,
                                          int cursorIndex, int itemCount) {
    if (heldInspectIndex >= 0 && heldInspectIndex < heldCount) {
        return {InspectSource::HeldJoker, heldInspectIndex};
    }
    if (cursorIndex >= 0 && cursorIndex < itemCount) {
        return {InspectSource::ShopItem, cursorIndex};
    }
    return {InspectSource::Placeholder, -1};
}

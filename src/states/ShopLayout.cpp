#include "states/ShopLayout.h"

ShopCardLayout shopCardLayout(ShopPlatform platform, int itemCount) {
    const int screenWidth = (platform == ShopPlatform::N3DS) ? 320 : 400;
    return {
        (screenWidth - itemCount * kShopCardStride) / 2 + 20,
        125,
        (platform == ShopPlatform::N3DS) ? 90 : 100,
        70,
        5,
        50
    };
}

ShopRect shopCardBodyRect(ShopPlatform platform, int itemCount, int index) {
    const ShopCardLayout layout = shopCardLayout(platform, itemCount);
    return {
        layout.startX + index * kShopCardStride,
        layout.y,
        layout.bodyWidth,
        layout.bodyHeight
    };
}

ShopRect shopCardHighlightRect(ShopPlatform platform, int itemCount, int index) {
    const ShopRect body = shopCardBodyRect(platform, itemCount, index);
    const int pad = shopCardLayout(platform, itemCount).highlightPad;
    return {
        body.x - pad,
        body.y - pad,
        body.w + pad * 2,
        body.h + pad * 2
    };
}

HeldJokerRowLayout heldJokerRowLayout(ShopPlatform platform) {
    return {
        (platform == ShopPlatform::N3DS) ? 12 : 412,
        90,
        55,
        60,
        60
    };
}

ShopRect heldJokerSlotRect(ShopPlatform platform, int index) {
    const HeldJokerRowLayout layout = heldJokerRowLayout(platform);
    return {
        layout.startX + index * layout.stride,
        layout.y,
        layout.cardWidth,
        layout.cardHeight
    };
}

int hitShopCard(ShopPlatform platform, int itemCount, int px, int py) {
    const ShopCardLayout layout = shopCardLayout(platform, itemCount);
    if (py < layout.y || py >= layout.y + layout.bodyHeight) {
        return -1;
    }
    for (int i = 0; i < itemCount; ++i) {
        const int cardX = layout.startX + i * kShopCardStride;
        if (px >= cardX && px < cardX + layout.bodyWidth) {
            return i;
        }
    }
    return -1;
}

int hitHeldJoker(ShopPlatform platform, int jokerCount, int px, int py) {
    const HeldJokerRowLayout layout = heldJokerRowLayout(platform);
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

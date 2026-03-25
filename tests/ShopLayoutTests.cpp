#include "states/ShopLayout.h"
#include "states/ShopState.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        fail(label);
    }
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expectEqual(const std::string& actual, const std::string& expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void expectRectEqual(const ShopRect& actual, const ShopRect& expected, const std::string& label) {
    if (actual.x != expected.x || actual.y != expected.y ||
        actual.w != expected.w || actual.h != expected.h) {
        std::ostringstream oss;
        oss << label << ": expected {" << expected.x << ", " << expected.y
            << ", " << expected.w << ", " << expected.h << "}, got {"
            << actual.x << ", " << actual.y << ", " << actual.w << ", " << actual.h << "}";
        fail(oss.str());
    }
}

void expectColorEqual(const ShopColor& actual, const ShopColor& expected, const std::string& label) {
    if (actual.r != expected.r || actual.g != expected.g ||
        actual.b != expected.b || actual.a != expected.a) {
        std::ostringstream oss;
        oss << label << ": expected {" << expected.r << ", " << expected.g
            << ", " << expected.b << ", " << expected.a << "}, got {"
            << actual.r << ", " << actual.g << ", " << actual.b << ", " << actual.a << "}";
        fail(oss.str());
    }
}

void testShopCardBodyRect() {
    expectRectEqual(shopCardBodyRect(ShopPlatform::SDL, 3, 0),
                    ShopRect{35, 125, 90, 70},
                    "SDL slot 0 body rect");
    expectRectEqual(shopCardBodyRect(ShopPlatform::SDL, 3, 2),
                    ShopRect{255, 125, 90, 70},
                    "SDL slot 2 body rect");

    expectRectEqual(shopCardBodyRect(ShopPlatform::ThreeDS, 3, 0),
                    ShopRect{15, 125, 80, 70},
                    "3DS slot 0 body rect");
    expectRectEqual(shopCardBodyRect(ShopPlatform::ThreeDS, 3, 2),
                    ShopRect{215, 125, 80, 70},
                    "3DS slot 2 body rect");
}

void testShopCardHighlightRect() {
    expectRectEqual(shopCardHighlightRect(ShopPlatform::ThreeDS, 3, 1),
                    ShopRect{110, 120, 90, 80},
                    "3DS highlight rect keeps 5px padding");
}

void testHeldJokerSlotRect() {
    // SDL: startX=412, index=0: x=412+0*60=412, y=90, w=55, h=60
    expectRectEqual(heldJokerSlotRect(ShopPlatform::SDL, 0),
                    ShopRect{412, 90, 55, 60},
                    "SDL held joker row starts at x=412");
}

void testBuyButtonRect() {
    expectRectEqual(buyButtonRect(ShopPlatform::SDL),
                    ShopRect{420, 160, 100, 50},
                    "SDL buy button rect");
    expectRectEqual(buyButtonRect(ShopPlatform::ThreeDS),
                    ShopRect{20, 160, 100, 50},
                    "3DS buy button rect");
}

void testNextBlindButtonRect() {
    expectRectEqual(nextBlindButtonRect(ShopPlatform::SDL),
                    ShopRect{615, 160, 120, 50},
                    "SDL next blind button rect");
    expectRectEqual(nextBlindButtonRect(ShopPlatform::ThreeDS),
                    ShopRect{215, 160, 95, 50},
                    "3DS next blind button rect");
}

void testHitBuyButton() {
    expect(hitBuyButton(ShopPlatform::SDL, 420, 160),
           "SDL buy button should include top-left corner");
    expect(hitBuyButton(ShopPlatform::SDL, 519, 209),
           "SDL buy button should include the last pixel inside the rect");
    expect(!hitBuyButton(ShopPlatform::SDL, 520, 160),
           "SDL buy button should exclude the right edge");
    expect(!hitBuyButton(ShopPlatform::SDL, 420, 210),
           "SDL buy button should exclude the bottom edge");

    expect(hitBuyButton(ShopPlatform::ThreeDS, 20, 160),
           "3DS buy button should include top-left corner");
    expect(!hitBuyButton(ShopPlatform::ThreeDS, 120, 160),
           "3DS buy button should exclude the right edge");
}

void testHitNextBlindButton() {
    expect(hitNextBlindButton(ShopPlatform::SDL, 615, 160),
           "SDL next blind button should include top-left corner");
    expect(hitNextBlindButton(ShopPlatform::SDL, 734, 209),
           "SDL next blind button should include the last pixel inside the rect");
    expect(!hitNextBlindButton(ShopPlatform::SDL, 735, 160),
           "SDL next blind button should exclude the right edge");
    expect(!hitNextBlindButton(ShopPlatform::SDL, 615, 210),
           "SDL next blind button should exclude the bottom edge");

    expect(hitNextBlindButton(ShopPlatform::ThreeDS, 215, 160),
           "3DS next blind button should include top-left corner");
    expect(!hitNextBlindButton(ShopPlatform::ThreeDS, 310, 160),
           "3DS next blind button should exclude the right edge");
}

void testRerollButtonRects() {
    expectRectEqual(rerollButtonRect(ShopPlatform::ThreeDS),
                    ShopRect{130, 160, 75, 50},
                    "3DS reroll button rect");
    expectRectEqual(rerollButtonRect(ShopPlatform::SDL),
                    ShopRect{530, 160, 75, 50},
                    "SDL reroll button rect");
}

void testHitRerollButton() {
    expect(hitRerollButton(ShopPlatform::ThreeDS, 167, 185),
           "3DS: center should hit reroll");
    expect(!hitRerollButton(ShopPlatform::ThreeDS, 129, 185),
           "3DS: one pixel left of reroll should miss");

    expect(hitRerollButton(ShopPlatform::SDL, 567, 185),
           "SDL: center should hit reroll");
    expect(!hitRerollButton(ShopPlatform::SDL, 529, 185),
           "SDL: one pixel left of reroll should miss");
}

void testResolveInspectSelection() {
    const InspectSelection sel = resolveInspectSelection(2, 3, 0, 3);
    expect(sel.source == InspectSource::HeldJoker,
           "valid held inspect should override shop cursor");
    expect(sel.index == 2, "held inspect index should be 2");
}

void testResolveInspectSelectionShopItem() {
    const InspectSelection sel = resolveInspectSelection(-1, 3, 1, 3);
    expect(sel.source == InspectSource::ShopItem,
           "invalid held inspect falls through to shop item");
    expect(sel.index == 1, "shop item index should be 1");
}

void testResolveInspectSelectionPlaceholder() {
    const InspectSelection sel = resolveInspectSelection(-1, 3, -1, 3);
    expect(sel.source == InspectSource::Placeholder,
           "no valid selection yields placeholder");
    expect(sel.index == -1, "placeholder index should be -1");
}

void testHitShopCard() {
    const int hit0 = hitShopCard(ShopPlatform::SDL, 3, 100, 150);
    std::ostringstream oss0;
    oss0 << "hit inside card 0: expected 0, got " << hit0;
    expect(hit0 == 0, oss0.str());

    const int hit1 = hitShopCard(ShopPlatform::SDL, 3, 150, 150);
    std::ostringstream oss1;
    oss1 << "hit inside card 1: expected 1, got " << hit1;
    expect(hit1 == 1, oss1.str());

    expectEqual(hitShopCard(ShopPlatform::SDL, 3, 280, 150), 2,
                "third SDL shop slot should be hittable");

    const int miss = hitShopCard(ShopPlatform::SDL, 3, 100, 100);
    expect(miss == -1, "click above card y range should miss");

    const int rightEdgeMiss = hitShopCard(ShopPlatform::SDL, 3, 125, 150);
    expect(rightEdgeMiss == -1, "shop card hit should exclude the body right edge");
}

void testHitHeldJoker() {
    // SDL, 2 jokers: startX=412, stride=60, w=55, y=90, h=60
    // slot 0: x=412..467, slot 1: x=472..527
    const int hit0 = hitHeldJoker(ShopPlatform::SDL, 2, 430, 110);
    std::ostringstream oss0;
    oss0 << "hit held joker 0: expected 0, got " << hit0;
    expect(hit0 == 0, oss0.str());

    // slot 2 would be at x=532 but jokerCount=2 so index 2 is out of range
    const int miss = hitHeldJoker(ShopPlatform::SDL, 2, 540, 110);
    expect(miss == -1, "hit beyond jokerCount should miss");

    const int bottomEdgeMiss = hitHeldJoker(ShopPlatform::SDL, 2, 430, 150);
    expect(bottomEdgeMiss == -1, "held joker hit should exclude the body bottom edge");
}

void testTask2Assertions() {
    expectEqual(hitShopCard(ShopPlatform::SDL, 3, 90, 140), 0,
                "SDL hover should use y=125..195");
    expectEqual(hitShopCard(ShopPlatform::SDL, 3, 90, 110), -1,
                "old y=105 card band should no longer hit");

    expectEqual(hitHeldJoker(ShopPlatform::ThreeDS, 3, 12, 90), 0,
                "3DS first held joker slot");
    // N3DS: startX=12, stride=60 -> slot 2 body is x=132..187; use 150 (mid-slot)
    expectEqual(hitHeldJoker(ShopPlatform::ThreeDS, 3, 150, 100), 2,
                "3DS third held joker slot uses 60px stride");

    expect(resolveInspectSelection(-1, 3, -1, 0).source == InspectSource::Placeholder,
           "placeholder should show only when neither held nor shop selection is valid");
}

void testJokerEffectColors() {
    expectColorEqual(jokerEffectColor(JokerEffectType::AddChips),
                    ShopColor{80, 120, 220, 255},
                    "AddChips color should match shop-card blue");
    expectColorEqual(jokerEffectColor(JokerEffectType::AddMult),
                    ShopColor{220, 60, 60, 255},
                    "AddMult color should match shop-card red");
    expectColorEqual(jokerEffectColor(JokerEffectType::MulMult),
                    ShopColor{180, 60, 220, 255},
                    "MulMult color should match shop-card purple");
}

void testFixedShopSlotNavigation() {
    expectEqual(nextSelectableShopSlot(0, +1, std::array<bool, 3>{true, false, false}), 1,
                "cursor should skip from sold left slot to next live slot");
    expectEqual(nextSelectableShopSlot(1, +1, std::array<bool, 3>{true, true, false}), 2,
                "cursor should advance to the third live slot");
    expectEqual(nextSelectableShopSlot(0, +1, std::array<bool, 3>{true, true, true}), -1,
                "cursor should become invalid when every slot is sold");
}

void testShopSlotSaleKeepsItemAndAdvancesCursor() {
    std::array<ShopSlot, kVisibleShopSlots> slots{};
    slots[0].offer.price = 4;
    slots[0].offer.joker.name = "Alpha";
    slots[1].offer.price = 6;
    slots[1].offer.joker.name = "Beta";
    slots[2].offer.price = 8;
    slots[2].offer.joker.name = "Gamma";

    int cursor = shop_state_helpers::markShopSlotSoldAndAdvanceCursor(slots, 0);
    expect(slots[0].sold, "sold slot should be marked sold");
    expectEqual(slots[0].offer.price, 4, "sold slot should keep its item data");
    expect(!isSelectableShopSlot(0, std::array<bool, kVisibleShopSlots>{slots[0].sold, slots[1].sold, slots[2].sold}),
           "sold slot should no longer be selectable");
    expectEqual(cursor, 1, "cursor should retarget to the remaining live slot");

    cursor = shop_state_helpers::markShopSlotSoldAndAdvanceCursor(slots, cursor);
    expect(slots[1].sold, "second sold slot should be marked sold");
    expectEqual(slots[1].offer.price, 6, "second sold slot should keep its item data");
    expectEqual(cursor, 2, "cursor should retarget to the final live slot");

    cursor = shop_state_helpers::markShopSlotSoldAndAdvanceCursor(slots, cursor);
    expect(slots[2].sold, "third sold slot should be marked sold");
    expectEqual(slots[2].offer.price, 8, "third sold slot should keep its item data");
    expectEqual(cursor, -1, "cursor should become invalid after all slots are sold");
}

void testUnavailableShopSlotBlocksSelectionAndUsesDedicatedLabel() {
    std::array<ShopSlot, kVisibleShopSlots> slots{};
    slots[0].unavailable = true;
    slots[1].sold = true;

    const std::array<bool, kVisibleShopSlots> disabled = shop_state_helpers::disabledShopSlots(slots);
    expect(!isSelectableShopSlot(0, disabled),
           "unavailable slot should not be selectable");
    expect(!isSelectableShopSlot(1, disabled),
           "sold slot should remain non-selectable");
    expect(isSelectableShopSlot(2, disabled),
           "remaining live slot should stay selectable");
    expectEqual(shop_state_helpers::blockedShopSlotLabel(slots[0]), std::string("UNAVAILABLE"),
                "unavailable slot should use dedicated label text");
    expectEqual(shop_state_helpers::blockedShopSlotLabel(slots[1]), std::string("SOLD"),
                "sold slot should keep sold label text");
}

void testUnavailableShopSlotNavigationSkipsBlockedOffer() {
    std::array<ShopSlot, kVisibleShopSlots> slots{};
    slots[0].unavailable = true;
    slots[1].sold = true;

    const std::array<bool, kVisibleShopSlots> disabled = shop_state_helpers::disabledShopSlots(slots);
    expectEqual(nextSelectableShopSlot(0, +1, disabled), 2,
                "cursor should skip blocked slots when advancing");
}

} // namespace

int main() {
    testShopCardBodyRect();
    testShopCardHighlightRect();
    testHeldJokerSlotRect();
    testBuyButtonRect();
    testNextBlindButtonRect();
    testHitBuyButton();
    testHitNextBlindButton();
    testRerollButtonRects();
    testHitRerollButton();
    testResolveInspectSelection();
    testResolveInspectSelectionShopItem();
    testResolveInspectSelectionPlaceholder();
    testHitShopCard();
    testHitHeldJoker();
    testTask2Assertions();
    testJokerEffectColors();
    testFixedShopSlotNavigation();
    testShopSlotSaleKeepsItemAndAdvancesCursor();
    testUnavailableShopSlotBlocksSelectionAndUsesDedicatedLabel();
    testUnavailableShopSlotNavigationSkipsBlockedOffer();

    std::cout << "ShopLayout tests passed\n";
    return 0;
}

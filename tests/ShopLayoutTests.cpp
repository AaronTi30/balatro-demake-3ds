#include "states/ShopLayout.h"

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
    expectRectEqual(shopCardBodyRect(3, 0),
                    ShopRect{15, 125, 80, 70},
                    "slot 0 body rect");
    expectRectEqual(shopCardBodyRect(3, 2),
                    ShopRect{215, 125, 80, 70},
                    "slot 2 body rect");
}

void testShopCardHighlightRect() {
    expectRectEqual(shopCardHighlightRect(3, 1),
                    ShopRect{110, 120, 90, 80},
                    "highlight rect keeps 5px padding");
}

void testHeldJokerSlotRect() {
    expectRectEqual(heldJokerSlotRect(0),
                    ShopRect{12, 90, 55, 60},
                    "held joker row starts at x=12");
}

void testBuyButtonRect() {
    expectRectEqual(buyButtonRect(),
                    ShopRect{20, 160, 100, 50},
                    "buy button rect");
}

void testNextBlindButtonRect() {
    expectRectEqual(nextBlindButtonRect(),
                    ShopRect{215, 160, 95, 50},
                    "next blind button rect");
}

void testHitBuyButton() {
    expect(hitBuyButton(20, 160),
           "buy button should include top-left corner");
    expect(hitBuyButton(119, 209),
           "buy button should include the last pixel inside the rect");
    expect(!hitBuyButton(120, 160),
           "buy button should exclude the right edge");
    expect(!hitBuyButton(20, 210),
           "buy button should exclude the bottom edge");
}

void testHitNextBlindButton() {
    expect(hitNextBlindButton(215, 160),
           "next blind button should include top-left corner");
    expect(hitNextBlindButton(309, 209),
           "next blind button should include the last pixel inside the rect");
    expect(!hitNextBlindButton(310, 160),
           "next blind button should exclude the right edge");
    expect(!hitNextBlindButton(215, 210),
           "next blind button should exclude the bottom edge");
}

void testRerollButtonRects() {
    expectRectEqual(rerollButtonRect(),
                    ShopRect{130, 160, 75, 50},
                    "reroll button rect");
}

void testHitRerollButton() {
    expect(hitRerollButton(167, 185),
           "center should hit reroll");
    expect(!hitRerollButton(129, 185),
           "one pixel left of reroll should miss");
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
    const int hit0 = hitShopCard(3, 50, 150);
    std::ostringstream oss0;
    oss0 << "hit inside card 0: expected 0, got " << hit0;
    expect(hit0 == 0, oss0.str());

    const int hit1 = hitShopCard(3, 150, 150);
    std::ostringstream oss1;
    oss1 << "hit inside card 1: expected 1, got " << hit1;
    expect(hit1 == 1, oss1.str());

    expectEqual(hitShopCard(3, 250, 150), 2,
                "third shop slot should be hittable");

    const int miss = hitShopCard(3, 50, 100);
    expect(miss == -1, "click above card y range should miss");

    const int rightEdgeMiss = hitShopCard(3, 95, 150);
    expect(rightEdgeMiss == -1, "shop card hit should exclude the body right edge");
}

void testHitHeldJoker() {
    const int hit0 = hitHeldJoker(2, 30, 110);
    std::ostringstream oss0;
    oss0 << "hit held joker 0: expected 0, got " << hit0;
    expect(hit0 == 0, oss0.str());

    const int miss = hitHeldJoker(2, 140, 110);
    expect(miss == -1, "hit beyond jokerCount should miss");

    const int bottomEdgeMiss = hitHeldJoker(2, 30, 150);
    expect(bottomEdgeMiss == -1, "held joker hit should exclude the body bottom edge");
}

void testTask2Assertions() {
    expectEqual(hitShopCard(3, 50, 140), 0,
                "hover should use y=125..195");
    expectEqual(hitShopCard(3, 50, 110), -1,
                "old y=105 card band should no longer hit");

    expectEqual(hitHeldJoker(3, 12, 90), 0,
                "first held joker slot");
    expectEqual(hitHeldJoker(3, 150, 100), 2,
                "third held joker slot uses 60px stride");

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

    int cursor = shop_layout_helpers::markShopSlotSoldAndAdvanceCursor(slots, 0);
    expect(slots[0].sold, "sold slot should be marked sold");
    expectEqual(slots[0].offer.price, 4, "sold slot should keep its item data");
    expect(!isSelectableShopSlot(0, std::array<bool, kVisibleShopSlots>{slots[0].sold, slots[1].sold, slots[2].sold}),
           "sold slot should no longer be selectable");
    expectEqual(cursor, 1, "cursor should retarget to the remaining live slot");

    cursor = shop_layout_helpers::markShopSlotSoldAndAdvanceCursor(slots, cursor);
    expect(slots[1].sold, "second sold slot should be marked sold");
    expectEqual(slots[1].offer.price, 6, "second sold slot should keep its item data");
    expectEqual(cursor, 2, "cursor should retarget to the final live slot");

    cursor = shop_layout_helpers::markShopSlotSoldAndAdvanceCursor(slots, cursor);
    expect(slots[2].sold, "third sold slot should be marked sold");
    expectEqual(slots[2].offer.price, 8, "third sold slot should keep its item data");
    expectEqual(cursor, -1, "cursor should become invalid after all slots are sold");
}

void testUnavailableShopSlotBlocksSelectionAndUsesDedicatedLabel() {
    std::array<ShopSlot, kVisibleShopSlots> slots{};
    slots[0].unavailable = true;
    slots[1].sold = true;

    const std::array<bool, kVisibleShopSlots> disabled = shop_layout_helpers::disabledShopSlots(slots);
    expect(!isSelectableShopSlot(0, disabled),
           "unavailable slot should not be selectable");
    expect(!isSelectableShopSlot(1, disabled),
           "sold slot should remain non-selectable");
    expect(isSelectableShopSlot(2, disabled),
           "remaining live slot should stay selectable");
    expectEqual(shop_layout_helpers::blockedShopSlotLabel(slots[0]), std::string("UNAVAILABLE"),
                "unavailable slot should use dedicated label text");
    expectEqual(shop_layout_helpers::blockedShopSlotLabel(slots[1]), std::string("SOLD"),
                "sold slot should keep sold label text");
}

void testUnavailableShopSlotNavigationSkipsBlockedOffer() {
    std::array<ShopSlot, kVisibleShopSlots> slots{};
    slots[0].unavailable = true;
    slots[1].sold = true;

    const std::array<bool, kVisibleShopSlots> disabled = shop_layout_helpers::disabledShopSlots(slots);
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

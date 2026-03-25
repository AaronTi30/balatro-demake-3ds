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

void testShopCardBodyRect() {
    // SDL: startX = (400 - 2*140)/2 + 20 = (400-280)/2 + 20 = 60+20 = 80
    // index 0: x=80, y=125, w=100, h=70
    expectRectEqual(shopCardBodyRect(ShopPlatform::SDL, 2, 0),
                    ShopRect{80, 125, 100, 70},
                    "SDL shop card body rect");
}

void testShopCardHighlightRect() {
    // 3DS: startX = (320 - 2*140)/2 + 20 = (320-280)/2 + 20 = 20+20 = 40
    // index 1: bodyX = 40 + 1*140 = 180, bodyY=125, bodyW=90, bodyH=70
    // highlight: x=180-5=175, y=125-5=120, w=90+10=100, h=70+10=80
    expectRectEqual(shopCardHighlightRect(ShopPlatform::N3DS, 2, 1),
                    ShopRect{175, 120, 100, 80},
                    "3DS highlight rect keeps 5px padding");
}

void testHeldJokerSlotRect() {
    // SDL: startX=412, index=0: x=412+0*60=412, y=90, w=55, h=60
    expectRectEqual(heldJokerSlotRect(ShopPlatform::SDL, 0),
                    ShopRect{412, 90, 55, 60},
                    "SDL held joker row starts at x=412");
}

void testResolveInspectSelection() {
    // heldInspectIndex=2, heldCount=3 -> valid held joker (index 2 < 3)
    // cursorIndex=0, itemCount=2 -> would be ShopItem but HeldJoker takes precedence
    const InspectSelection sel = resolveInspectSelection(2, 3, 0, 2);
    expect(sel.source == InspectSource::HeldJoker,
           "valid held inspect should override shop cursor");
    expect(sel.index == 2, "held inspect index should be 2");
}

void testResolveInspectSelectionShopItem() {
    // heldInspectIndex=-1 -> invalid, cursorIndex=1, itemCount=2 -> ShopItem
    const InspectSelection sel = resolveInspectSelection(-1, 3, 1, 2);
    expect(sel.source == InspectSource::ShopItem,
           "invalid held inspect falls through to shop item");
    expect(sel.index == 1, "shop item index should be 1");
}

void testResolveInspectSelectionPlaceholder() {
    // heldInspectIndex=-1, cursorIndex=-1 -> Placeholder
    const InspectSelection sel = resolveInspectSelection(-1, 3, -1, 2);
    expect(sel.source == InspectSource::Placeholder,
           "no valid selection yields placeholder");
    expect(sel.index == -1, "placeholder index should be -1");
}

void testHitShopCard() {
    // SDL, 2 items: startX=80, stride=140
    // card 0 body: x=80..180, y=125..195
    // click inside card 0
    const int hit0 = hitShopCard(ShopPlatform::SDL, 2, 100, 150);
    std::ostringstream oss0;
    oss0 << "hit inside card 0: expected 0, got " << hit0;
    expect(hit0 == 0, oss0.str());

    // click inside card 1: x=80+140=220..320
    const int hit1 = hitShopCard(ShopPlatform::SDL, 2, 250, 150);
    std::ostringstream oss1;
    oss1 << "hit inside card 1: expected 1, got " << hit1;
    expect(hit1 == 1, oss1.str());

    // miss (above card y range)
    const int miss = hitShopCard(ShopPlatform::SDL, 2, 100, 100);
    expect(miss == -1, "click above card y range should miss");
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
}

} // namespace

int main() {
    testShopCardBodyRect();
    testShopCardHighlightRect();
    testHeldJokerSlotRect();
    testResolveInspectSelection();
    testResolveInspectSelectionShopItem();
    testResolveInspectSelectionPlaceholder();
    testHitShopCard();
    testHitHeldJoker();

    std::cout << "ShopLayout tests passed\n";
    return 0;
}

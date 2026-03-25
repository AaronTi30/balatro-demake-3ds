#pragma once

#include "../core/State.h"
#include "../game/ShopOffer.h"
#include "../game/RunState.h"
#include "states/ShopLayout.h"
#include <array>
#include <memory>
#include <random>
#include <vector>

namespace shop_state_helpers {
inline bool isSlotDisabled(const ShopSlot& slot) {
    return slot.sold || slot.unavailable;
}

inline const char* blockedShopSlotLabel(const ShopSlot& slot) {
    return slot.unavailable ? "UNAVAILABLE" : "SOLD";
}

inline std::array<bool, kVisibleShopSlots> disabledShopSlots(const std::array<ShopSlot, kVisibleShopSlots>& slots) {
    std::array<bool, kVisibleShopSlots> disabled{};
    for (std::size_t i = 0; i < slots.size(); ++i) {
        disabled[i] = isSlotDisabled(slots[i]);
    }
    return disabled;
}

inline int markShopSlotSoldAndAdvanceCursor(std::array<ShopSlot, kVisibleShopSlots>& slots, int purchasedSlot) {
    slots[purchasedSlot].sold = true;
    return nextSelectableShopSlot(purchasedSlot, +1, disabledShopSlots(slots));
}
} // namespace shop_state_helpers

class ShopState : public State {
public:
    ShopState(StateMachine* machine, std::shared_ptr<RunState> runState);
    static std::array<ShopSlot, kVisibleShopSlots> generateShopItems(std::mt19937& rng,
                                                                     const RunState& runState);

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void handleInput() override;
    void renderTopScreen(Application* app) override;
    void renderBottomScreen(Application* app) override;

private:
    void generateItems();
    void clearHeldInspect();
    bool tryBuySelectedItem();
    bool trySellHeldJoker();
    bool tryReroll();
    std::array<bool, kVisibleShopSlots> disabledMask() const;

    std::shared_ptr<RunState> m_runState;
    std::array<ShopSlot, kVisibleShopSlots> m_slots{};
    int m_cursorIndex = 0;
    int m_heldInspectIndex = -1;
    float m_inputDelay = 0.3f;
    std::mt19937 m_rng;
#ifndef N3DS
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;
#endif
};

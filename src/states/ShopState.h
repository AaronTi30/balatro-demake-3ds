#pragma once

#include "../core/State.h"
#include "../game/Joker.h"
#include "../game/RunState.h"
#include "states/ShopLayout.h"
#include <array>
#include <memory>
#include <random>
#include <vector>

struct ShopItem {
    Joker joker;
    int price;
};

struct ShopSlot {
    ShopItem item;
    bool sold = false;
    bool unavailable = false;
};

namespace shop_state_helpers {
inline bool isSlotDisabled(const ShopSlot& slot) {
    return slot.sold || slot.unavailable;
}

inline const char* blockedShopSlotLabel(const ShopSlot& slot) {
    return slot.unavailable ? "UNAVAILABLE" : "SOLD";
}

inline std::array<bool, kVisibleShopSlots> disabledShopSlots(const std::array<ShopSlot, kVisibleShopSlots>& slots) {
    return { isSlotDisabled(slots[0]), isSlotDisabled(slots[1]) };
}

inline int markShopSlotSoldAndAdvanceCursor(std::array<ShopSlot, kVisibleShopSlots>& slots, int purchasedSlot) {
    slots[purchasedSlot].sold = true;
    return nextSelectableShopSlot(purchasedSlot, +1, disabledShopSlots(slots));
}

inline int purchaseShopSlotAndAdvanceCursor(RunState& runState,
                                            std::array<ShopSlot, kVisibleShopSlots>& slots,
                                            int purchasedSlot) {
    runState.money -= slots[purchasedSlot].item.price;
    runState.jokers.push_back(slots[purchasedSlot].item.joker);
    runState.markJokerRemovedFromShopPool(Joker::idFor(slots[purchasedSlot].item.joker));
    return markShopSlotSoldAndAdvanceCursor(slots, purchasedSlot);
}
} // namespace shop_state_helpers

class ShopState : public State {
public:
    ShopState(StateMachine* machine, std::shared_ptr<RunState> runState);
    static std::array<ShopSlot, kVisibleShopSlots> generateShopItems(std::mt19937& rng, const RunState& runState);

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

inline std::vector<Joker> filterAvailableShopCandidates(const std::vector<Joker>& candidates, const RunState& runState) {
    std::vector<Joker> available;
    available.reserve(candidates.size());
    for (const Joker& joker : candidates) {
        if (runState.isJokerShopAvailable(Joker::idFor(joker))) {
            available.push_back(joker);
        }
    }

    return available;
}

inline ShopSlot makeUnavailableShopSlot() {
    ShopSlot slot;
    slot.item.joker = Joker::plainJoker();
    slot.item.joker.name.clear();
    slot.item.joker.description.clear();
    slot.item.price = 0;
    slot.unavailable = true;
    return slot;
}

inline ShopSlot drawShopSlotFromCandidates(const std::vector<Joker>& candidates, std::mt19937& rng) {
    if (candidates.empty()) {
        return makeUnavailableShopSlot();
    }

    ShopSlot slot;
    slot.item.joker = Joker::drawFromCandidates(candidates, rng);
    slot.item.price = std::uniform_int_distribution<int>(
        slot.item.joker.shopPriceRange.min,
        slot.item.joker.shopPriceRange.max
    )(rng);
    return slot;
}

inline std::array<ShopSlot, kVisibleShopSlots> ShopState::generateShopItems(std::mt19937& rng, const RunState& runState) {
    std::unordered_set<std::string> excludedIds = runState.currentOwnedJokerIds();
    std::array<ShopSlot, kVisibleShopSlots> slots{};

    const std::vector<Joker> slot0Candidates = filterAvailableShopCandidates(
        Joker::weakOrMediumPoolFiltered(excludedIds),
        runState
    );
    slots[0] = drawShopSlotFromCandidates(slot0Candidates, rng);
    if (!slots[0].unavailable) {
        excludedIds.insert(Joker::idFor(slots[0].item.joker));
    }

    const JokerTier slot1Tier = Joker::tierForWeightedRoll(std::uniform_int_distribution<int>(1, 100)(rng));
    std::vector<Joker> slot1Pool;
    if (slot1Tier == JokerTier::Weak) {
        slot1Pool = Joker::weakPoolFiltered(excludedIds);
    } else if (slot1Tier == JokerTier::Medium) {
        slot1Pool = Joker::mediumPoolFiltered(excludedIds);
    } else {
        slot1Pool = Joker::strongPoolFiltered(excludedIds);
    }

    slots[1] = drawShopSlotFromCandidates(filterAvailableShopCandidates(slot1Pool, runState), rng);
    return slots;
}

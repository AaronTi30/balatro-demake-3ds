#pragma once

#include "Card.h"
#include "HandType.h"
#include "Joker.h"

#include <array>
#include <random>
#include <string>

class RunState;

enum class ShopOfferKind {
    Joker,
    DeckCard,
    HandUpgrade
};

struct ShopOffer {
    ShopOfferKind kind = ShopOfferKind::Joker;
    Joker joker{};
    Card card{Suit::Clubs, Rank::Ace};
    HandType handType = HandType::HighCard;
    int price = 0;
};

struct ShopSlot {
    ShopOffer offer{};
    bool sold = false;
    bool unavailable = false;
};

inline constexpr std::size_t kShopOfferCount = 3;

std::array<ShopSlot, kShopOfferCount> generateShopOffers(std::mt19937& rng, const RunState& runState);
bool applyShopOfferPurchase(RunState& runState, ShopSlot& slot);
std::string shopOfferTitle(const ShopOffer& offer);
std::string shopOfferDescription(const ShopOffer& offer);

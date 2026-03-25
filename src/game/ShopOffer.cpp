#include "ShopOffer.h"

#include "Joker.h"

#include <array>
#include <random>
#include <sstream>
#include <unordered_set>

namespace {

constexpr int kDeckCardOfferPrice = 4;
constexpr int kHandUpgradeOfferPrice = 6;

const char* fullSuitName(Suit suit) {
    switch (suit) {
        case Suit::Hearts:
            return "Hearts";
        case Suit::Diamonds:
            return "Diamonds";
        case Suit::Clubs:
            return "Clubs";
        case Suit::Spades:
            return "Spades";
        default:
            return "Unknown Suit";
    }
}

const char* fullRankName(Rank rank) {
    switch (rank) {
        case Rank::Ace:
            return "Ace";
        case Rank::Two:
            return "Two";
        case Rank::Three:
            return "Three";
        case Rank::Four:
            return "Four";
        case Rank::Five:
            return "Five";
        case Rank::Six:
            return "Six";
        case Rank::Seven:
            return "Seven";
        case Rank::Eight:
            return "Eight";
        case Rank::Nine:
            return "Nine";
        case Rank::Ten:
            return "Ten";
        case Rank::Jack:
            return "Jack";
        case Rank::Queen:
            return "Queen";
        case Rank::King:
            return "King";
        default:
            return "Unknown Rank";
    }
}

ShopSlot makeUnavailableSlot() {
    ShopSlot slot;
    slot.unavailable = true;
    slot.offer.price = 0;
    return slot;
}

ShopSlot makeJokerSlot(std::mt19937& rng, const RunState& runState) {
    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::Joker;

    std::unordered_set<std::string> excludedIds = runState.currentOwnedJokerIds();
    const std::vector<Joker> candidates = Joker::weakOrMediumPoolFiltered(excludedIds);
    std::vector<Joker> availableCandidates;
    availableCandidates.reserve(candidates.size());
    for (const Joker& joker : candidates) {
        if (runState.isJokerShopAvailable(Joker::idFor(joker))) {
            availableCandidates.push_back(joker);
        }
    }

    if (availableCandidates.empty()) {
        return makeUnavailableSlot();
    }

    slot.offer.joker = Joker::drawFromCandidates(availableCandidates, rng);
    std::uniform_int_distribution<int> priceDist(
        slot.offer.joker.shopPriceRange.min,
        slot.offer.joker.shopPriceRange.max
    );
    slot.offer.price = priceDist(rng);
    return slot;
}

ShopSlot makeDeckCardSlot(std::mt19937& rng) {
    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::DeckCard;
    std::uniform_int_distribution<int> suitDist(0, 3);
    std::uniform_int_distribution<int> rankDist(1, 13);
    slot.offer.card = Card{
        static_cast<Suit>(suitDist(rng)),
        static_cast<Rank>(rankDist(rng))
    };
    slot.offer.price = kDeckCardOfferPrice;
    return slot;
}

ShopSlot makeHandUpgradeSlot(std::mt19937& rng) {
    ShopSlot slot;
    slot.offer.kind = ShopOfferKind::HandUpgrade;
    std::uniform_int_distribution<int> handTypeDist(0, static_cast<int>(kHandTypeCount) - 1);
    slot.offer.handType = static_cast<HandType>(handTypeDist(rng));
    slot.offer.price = kHandUpgradeOfferPrice;
    return slot;
}

} // namespace

std::array<ShopSlot, kShopOfferCount> generateShopOffers(std::mt19937& rng, const RunState& runState) {
    return {
        makeJokerSlot(rng, runState),
        makeDeckCardSlot(rng),
        makeHandUpgradeSlot(rng)
    };
}

bool applyShopOfferPurchase(RunState& runState, ShopSlot& slot) {
    if (slot.sold || slot.unavailable || runState.money < slot.offer.price) {
        return false;
    }

    runState.money -= slot.offer.price;

    switch (slot.offer.kind) {
        case ShopOfferKind::Joker:
            runState.jokers.push_back(slot.offer.joker);
            runState.markJokerRemovedFromShopPool(Joker::idFor(slot.offer.joker));
            break;
        case ShopOfferKind::DeckCard:
            runState.addCardToRunDeck(slot.offer.card.suit, slot.offer.card.rank);
            break;
        case ShopOfferKind::HandUpgrade:
            runState.levelUpHand(slot.offer.handType);
            break;
        default:
            return false;
    }

    slot.sold = true;
    return true;
}

std::string shopOfferTitle(const ShopOffer& offer) {
    switch (offer.kind) {
        case ShopOfferKind::Joker:
            return offer.joker.name;
        case ShopOfferKind::DeckCard: {
            std::ostringstream oss;
            oss << "Add " << rankToString(offer.card.rank) << suitToString(offer.card.suit);
            return oss.str();
        }
        case ShopOfferKind::HandUpgrade: {
            std::ostringstream oss;
            oss << "Level " << handTypeName(offer.handType);
            return oss.str();
        }
        default:
            return "";
    }
}

std::string shopOfferDescription(const ShopOffer& offer) {
    switch (offer.kind) {
        case ShopOfferKind::Joker:
            return offer.joker.description;
        case ShopOfferKind::DeckCard: {
            std::ostringstream oss;
            oss << "Add " << fullRankName(offer.card.rank) << " of " << fullSuitName(offer.card.suit)
                << " to your deck";
            return oss.str();
        }
        case ShopOfferKind::HandUpgrade: {
            std::ostringstream oss;
            oss << handTypeName(offer.handType) << " gains +10 chips and +1 mult";
            return oss.str();
        }
        default:
            return "";
    }
}

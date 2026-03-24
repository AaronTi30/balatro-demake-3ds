# Balance Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebalance early progression so ante 1 stays survivable, shops are less automatically positive, and boss blinds occasionally disrupt dominant hand patterns.

**Architecture:** Keep the current run-state/shop/gameplay split. Move balance tables and boss-blind state into `RunState`, expand `Joker` into a small metadata-backed catalog that `ShopState` can price and draw from by tier, and let `HandEvaluator` apply an optional boss modifier after normal scoring and joker effects.

**Tech Stack:** C++17, CMake, SDL2, citro2d/citro3d, existing custom unit-test executables

---

## File Map

- Modify: `src/game/RunState.h`
- Modify: `src/game/RunState.cpp`
- Modify: `src/game/Joker.h`
- Modify: `src/game/Joker.cpp`
- Modify: `src/game/HandEvaluator.h`
- Modify: `src/game/HandEvaluator.cpp`
- Modify: `src/states/ShopState.h`
- Modify: `src/states/ShopState.cpp`
- Modify: `src/states/GameplayState.cpp`
- Modify: `tests/RunStateTests.cpp`
- Modify: `tests/HandEvaluatorTests.cpp`
- Create: `tests/JokerTests.cpp`
- Modify: `CMakeLists.txt`

## Planned Data Boundaries

- `RunState` owns the rebalance tables:

```cpp
struct BlindConfig {
    int reward;
    int target;
};

enum class BossBlindModifier {
    None,
    PairTax,
    SmallHandPunish,
    SuitLock,
    FaceTax,
    HighCardWall
};
```

- `Joker` becomes catalog-backed instead of "anonymous callback only":

```cpp
enum class JokerTier {
    Weak,
    Medium,
    Strong
};

struct PriceRange {
    int min;
    int max;
};

struct Joker {
    std::string name;
    std::string description;
    JokerEffectType effectType;
    JokerTier tier;
    PriceRange shopPriceRange;
    std::function<void(HandEvalContext&)> evaluate;

    static Joker createPlainJoker();
    static Joker drawShopSlotOne(std::mt19937& rng);
    static Joker drawWeightedShopSlot(std::mt19937& rng);
};
```

- `HandEvaluator` accepts boss pressure as an explicit finalization input:

```cpp
static HandResult evaluate(std::vector<Card> cards,
                           const std::vector<Joker>& jokers = {},
                           BossBlindModifier bossModifier = BossBlindModifier::None,
                           Suit blockedSuit = Suit::Clubs);
```

The blocked suit argument only matters for `SuitLock`; store it in `RunState` alongside the active boss modifier so UI and scoring use the same value.

### Task 1: Retune Economy and Blind Targets

**Files:**
- Modify: `src/game/RunState.h`
- Modify: `src/game/RunState.cpp`
- Modify: `tests/RunStateTests.cpp`

- [ ] **Step 1: Add explicit reward and target tables to `RunState`**

Replace the current base-target-plus-stage-multiplier logic with an explicit blind table so ante 1-3 match the spec exactly and ante 4-8 keep a sharp ramp.

```cpp
struct BlindTargets {
    int small;
    int big;
    int boss;
};

constexpr BlindTargets kBlindTargets[] = {
    {  300,   450,   600 },
    {  900,  1350,  1800 },
    { 2400,  3600,  4800 },
    { 5000,  7500, 10000 },
    {11000, 16500, 22000 },
    {20000, 30000, 40000 },
    {35000, 52500, 70000 },
    {50000, 75000,100000 }
};

constexpr int kBlindRewards[] = { 2, 3, 4 };
```

- [ ] **Step 2: Update round setup and reward helpers**

Keep `startNewRun()` at `$4`, but make `startRound()`, `currentBlindReward()`, and `targetForBlind()` all read from the explicit tables so gameplay and UI stay consistent.

- [ ] **Step 3: Write or update unit tests for the new curve**

Extend `tests/RunStateTests.cpp` so it asserts:

```cpp
expectEqual(run.money, 4, "new run starts with four dollars");
expectEqual(run.currentBlindReward(), 2, "small blind reward");
expectEqual(RunState::targetForBlind(1, BlindStage::Big), 450, "ante 1 big blind target");
expectEqual(RunState::targetForBlind(3, BlindStage::Boss), 4800, "ante 3 boss blind target");
```

Also update the payout accumulation test from `7 / 11 / 16` to `6 / 9 / 13`.

- [ ] **Step 4: Build the focused test target**

Run: `cmake --build build --target run_state_tests`
Expected: successful build of `run_state_tests`.

- [ ] **Step 5: Run the `RunState` tests**

Run: `ctest --test-dir build -R run_state_tests --output-on-failure`
Expected: PASS with updated reward and target expectations.

- [ ] **Step 6: Commit**

```bash
git add src/game/RunState.h src/game/RunState.cpp tests/RunStateTests.cpp
git commit -m "balance: retune blind rewards and targets"
```

### Task 2: Add Tiered Joker Pool and Weighted Shop Generation

**Files:**
- Modify: `src/game/Joker.h`
- Modify: `src/game/Joker.cpp`
- Modify: `src/states/ShopState.h`
- Modify: `src/states/ShopState.cpp`
- Create: `tests/JokerTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Expand `Joker` metadata to support balance-driven generation**

Add tier and shop price metadata, then replace the current flat `getRandom()` pool with named factory helpers for the full nine-joker set:

```cpp
// weak
Plain Joker    // +2 Mult
Greedy Joker   // +20 Chips if Pair
Suit Joker     // +2 Mult per scoring Spade

// medium
Focused Joker  // +8 Mult if <= 3 cards played
Flush Joker    // +40 Chips if played hand is Flush
Straight Joker // +40 Chips if played hand is Straight

// strong
Heavy Joker     // +5 Mult
Aggro Joker     // +60 Chips if Pair or better
Precision Joker // +12 Mult if exactly 1 card scores
```

- [ ] **Step 2: Add catalog and weighted draw helpers in `Joker.cpp`**

Implement one pure catalog source plus two draw helpers:

```cpp
static const std::vector<Joker>& weakPool();
static const std::vector<Joker>& mediumPool();
static const std::vector<Joker>& strongPool();

static Joker drawFromPool(const std::vector<Joker>& pool, std::mt19937& rng);
static Joker drawWeakOrMedium(std::mt19937& rng);
static Joker drawWeightedFullPool(std::mt19937& rng); // 50 / 35 / 15
```

Do not preserve the old `getRandom()` behavior. Replace callers so shop generation is deterministic about slot rules.

- [ ] **Step 3: Make `ShopState` generate one constrained slot and one weighted slot**

In `ShopState::generateItems()`, keep two offers but assign them like this:

```cpp
item0.joker = Joker::drawWeakOrMedium(rng);
item1.joker = Joker::drawWeightedFullPool(rng);
item0.price = randomInRange(item0.joker.shopPriceRange, rng);
item1.price = randomInRange(item1.joker.shopPriceRange, rng);
```

Preserve the existing buy flow, money checks, and joker-cap checks. Only change offer generation, pricing, and any shop text needed to communicate the price cleanly.

- [ ] **Step 4: Add focused joker-generation tests**

Create `tests/JokerTests.cpp` with unit coverage for:

```cpp
expect(plainJoker.tier == JokerTier::Weak, "plain joker tier");
expectEqual(plainJoker.shopPriceRange.min, 4, "weak joker min price");
expectEqual(heavyJoker.shopPriceRange.max, 10, "strong joker max price");
```

Add a deterministic RNG test that repeatedly calls slot-one generation and asserts no strong jokers appear, then repeatedly calls weighted-slot generation and asserts all returned jokers come from the nine-card catalog.

- [ ] **Step 5: Wire the new test target into CMake**

Add:

```cmake
add_executable(joker_tests
    tests/JokerTests.cpp
    src/game/Joker.cpp
    src/game/HandEvaluator.cpp
)
target_include_directories(joker_tests PRIVATE src)
add_test(NAME joker_tests COMMAND joker_tests)
```

`HandEvaluator.cpp` is included so any helper code needed by joker predicates that reference `HandType` or shared scoring utilities is available during linking.

- [ ] **Step 6: Build the joker and hand-evaluator test targets**

Run: `cmake --build build --target joker_tests hand_evaluator_tests`
Expected: successful build of both targets.

- [ ] **Step 7: Run the generation tests**

Run: `ctest --test-dir build -R "joker_tests|hand_evaluator_tests" --output-on-failure`
Expected: PASS with the new catalog and no regressions in existing hand-evaluator behavior.

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt src/game/Joker.h src/game/Joker.cpp src/states/ShopState.h src/states/ShopState.cpp tests/JokerTests.cpp
git commit -m "balance: add tiered joker pool and weighted shop offers"
```

### Task 3: Add Lightweight Boss-Blind Modifiers and UI Text

**Files:**
- Modify: `src/game/RunState.h`
- Modify: `src/game/RunState.cpp`
- Modify: `src/game/HandEvaluator.h`
- Modify: `src/game/HandEvaluator.cpp`
- Modify: `src/states/GameplayState.cpp`
- Modify: `src/states/ShopState.cpp`
- Modify: `tests/RunStateTests.cpp`
- Modify: `tests/HandEvaluatorTests.cpp`

- [ ] **Step 1: Add boss-modifier state and preview helpers to `RunState`**

Store the current boss modifier plus the next one that shop/gameplay can preview:

```cpp
BossBlindModifier currentBossModifier = BossBlindModifier::None;
BossBlindModifier nextBossModifier = BossBlindModifier::None;
Suit currentBlockedSuit = Suit::Clubs;
Suit nextBlockedSuit = Suit::Clubs;

void rollNextBossModifier(std::mt19937& rng);
void enterCurrentBlind();
const char* bossModifierName(BossBlindModifier modifier);
const char* bossModifierDescription(BossBlindModifier modifier, Suit blockedSuit);
```

Flow rule:
- when a run starts, roll `nextBossModifier`
- when advancing from big blind to boss blind, promote `nextBossModifier` to current and roll the next preview
- when advancing off a boss blind, clear `currentBossModifier`

- [ ] **Step 2: Pass the active modifier into hand scoring**

Extend `HandEvaluator::evaluate(...)` and final-score calculation so boss effects apply after normal hand base values, scoring-card chip bonus, and joker callbacks.

Implement the modifier rules directly:

```cpp
PairTax:         if (type == HandType::Pair || type == HandType::TwoPair) score = score * 75 / 100;
SmallHandPunish: if (playedCards.size() <= 3) score = score * 70 / 100;
SuitLock:        remove blocked-suit rank chips from scoring-card chip bonus before final score
FaceTax:         J/Q/K/A scoring cards contribute half rank-chip bonus
HighCardWall:    if (type == HandType::HighCard || type == HandType::Pair) score = score * 70 / 100;
```

For `SuitLock` and `FaceTax`, update `HandResult` fields consistently so preview text matches the actual score shown after play.

- [ ] **Step 3: Add evaluator tests for each modifier**

Extend `tests/HandEvaluatorTests.cpp` with direct cases like:

```cpp
HandResult pairTax = HandEvaluator::evaluate(pairCards, {}, BossBlindModifier::PairTax);
expectEqual(pairTax.finalScore, baselinePair.finalScore * 75 / 100, "pair tax lowers pair score");

HandResult punished = HandEvaluator::evaluate(threeCardHand, {}, BossBlindModifier::SmallHandPunish);
expectEqual(punished.finalScore, baseline.finalScore * 70 / 100, "small hand punish lowers short hands");
```

Add at least one `SuitLock` test that blocks the suit of a scoring card and verifies the chip bonus drops, and one `FaceTax` test that proves face-card chip contribution is halved rather than removed.

- [ ] **Step 4: Add `RunState` tests for boss roll and preview flow**

Update `tests/RunStateTests.cpp` so it asserts:

```cpp
expect(run.nextBossModifier != BossBlindModifier::None, "run should preview a boss modifier");
run.blindStage = BlindStage::Big;
run.advanceBlind();
expect(run.currentBossModifier != BossBlindModifier::None, "boss blind should activate stored modifier");
```

If randomness makes the API awkward to test, inject a seedable RNG into the helper instead of depending on `std::random_device` inside the assertions.

- [ ] **Step 5: Render modifier text in gameplay and shop UI**

Update:

- `src/states/GameplayState.cpp` to show the active boss modifier name/description during boss rounds near the blind HUD and in the round-clear summary when the next blind is a boss.
- `src/states/ShopState.cpp` to show the next boss modifier preview alongside the existing "Next: Ante X Blind" text.

Keep the UI lightweight. Two text lines are enough:

```cpp
"Boss: Pair Tax"
"Pair and Two Pair score 75%"
```

- [ ] **Step 6: Build the affected test targets**

Run: `cmake --build build --target run_state_tests hand_evaluator_tests balatro_demake`
Expected: successful build of runtime and test targets.

- [ ] **Step 7: Run automated tests**

Run: `ctest --test-dir build -R "run_state_tests|hand_evaluator_tests|joker_tests" --output-on-failure`
Expected: PASS for all three test executables.

- [ ] **Step 8: Manual verification**

Launch the game and confirm:

- ante 1 small/big/boss targets show `300 / 450 / 600`
- first shop can present a weak or medium joker in slot 1 and tier-appropriate prices
- some shops are skippable because prices exceed current money
- boss rounds display a modifier before play
- score previews and resolved hand scores match under boss penalties

Run: `./build/balatro_demake`
Expected: game launches and the balance/UI changes are visible on the PC build.

- [ ] **Step 9: Commit**

```bash
git add src/game/RunState.h src/game/RunState.cpp src/game/HandEvaluator.h src/game/HandEvaluator.cpp src/states/GameplayState.cpp src/states/ShopState.cpp tests/RunStateTests.cpp tests/HandEvaluatorTests.cpp
git commit -m "balance: add boss blind modifiers and previews"
```

## Final Verification

- [ ] **Step 1: Build all PC targets**

Run: `cmake --build build`
Expected: successful full PC build.

- [ ] **Step 2: Run the full automated test suite**

Run: `ctest --test-dir build --output-on-failure`
Expected: PASS for `run_state_tests`, `hand_tests`, `hand_evaluator_tests`, and `joker_tests`.

- [ ] **Step 3: Perform one full manual balance smoke run**

Verify:

- ante 1 is beatable with no joker
- one early weak joker helps but does not trivialize ante 2
- at least one shop visit offers a reasonable skip
- at least one boss modifier changes the preferred hand choice
- no UI screen shows stale boss preview text after clearing a boss blind

- [ ] **Step 4: Commit any final cleanup**

```bash
git add src/game/RunState.h src/game/RunState.cpp src/game/Joker.h src/game/Joker.cpp src/game/HandEvaluator.h src/game/HandEvaluator.cpp src/states/ShopState.h src/states/ShopState.cpp src/states/GameplayState.cpp tests/RunStateTests.cpp tests/HandEvaluatorTests.cpp tests/JokerTests.cpp CMakeLists.txt
git commit -m "balance: finalize first-pass rebalance"
```

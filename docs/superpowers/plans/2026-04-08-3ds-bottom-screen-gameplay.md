# 3DS Bottom Screen Gameplay HUD Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved 3DS bottom-screen gameplay HUD redesign: balanced hybrid layout, no persistent `Options`/`Run Info` buttons, visible money and score state, hand info grouped with chips/mult, plus `Sort` and `Peek Deck` controls.

**Architecture:** Keep the work inside the gameplay UI boundary, but extract bottom-screen geometry and hit-testing into a small pure helper module so the 3DS HUD can be tested without rendering. Add missing game primitives for hand sorting and deck peeking, then rewire `GameplayState` to render the four-band HUD and honor the new button/touch interactions. Because `START` currently exits the app on 3DS, pause-screen integration is explicitly deferred; `Peek Deck` is implemented as an in-game overlay, not a pause feature.

**Tech Stack:** C++17, SDL2 desktop path, citro2d 3DS path, CMake/ctest

---

## Assumptions To Carry Into Implementation

- 3DS face-button mapping becomes:
  - `A`: toggle current card selection
  - `X`: `Play Hand`
  - `Y`: `Discard`
  - `R`: toggle `Peek Deck`
- `Sort` is touch-only on 3DS and remains clickable/tappable on the bottom HUD
- Desktop parity mapping becomes:
  - `Space`: toggle current card selection
  - `Return`: `Play Hand`
  - `D`: `Discard`
  - `S`: toggle `Sort`
  - `Tab`: toggle `Peek Deck`
- `Peek Deck` shows a bottom-screen overlay with the next few cards from the round deck and the remaining count. It does not attempt to implement the eventual pause/details screen.

## File Map

| Action | Path | Responsibility |
|---|---|---|
| Create | `src/states/GameplayLayout.h` | Pure geometry/types for the 3DS bottom HUD and touch hit-testing |
| Create | `src/states/GameplayLayout.cpp` | Rect builders, HUD action hit resolution, score/progress helpers |
| Create | `tests/GameplayLayoutTests.cpp` | Unit tests for bottom HUD layout/action mapping |
| Modify | `CMakeLists.txt` | Add `gameplay_layout_tests` and `deck_tests` targets |
| Modify | `src/game/Hand.h` | Sorting API for the current hand |
| Modify | `src/game/Hand.cpp` | Sorting implementation that preserves `HeldCard` selection state |
| Modify | `tests/HandTests.cpp` | Hand sort regression tests |
| Modify | `src/game/Deck.h` | Deck peek API for `Peek Deck` overlay |
| Modify | `src/game/Deck.cpp` | Non-destructive top-card access |
| Create | `tests/DeckTests.cpp` | Unit tests for deck peek behavior |
| Modify | `src/states/GameplayState.h` | Replace old bottom-screen layout fields, add HUD/overlay state |
| Modify | `src/states/GameplayState.cpp` | New bottom HUD rendering, new input map, sort/peek behavior |

## Task 1: Add a Testable Gameplay HUD Layout Module

**Files:**
- Create: `src/states/GameplayLayout.h`
- Create: `src/states/GameplayLayout.cpp`
- Create: `tests/GameplayLayoutTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `src/states/GameplayState.h:49-70`

- [ ] **Step 1: Write the failing gameplay layout tests**

Create `tests/GameplayLayoutTests.cpp` with coverage for:

```cpp
#include "states/GameplayLayout.h"

void testPrimaryButtonRects() {
    expectRectEqual(gameplayPlayButtonRect(), GameplayRect{12, 176, 118, 36}, "play rect");
    expectRectEqual(gameplayDiscardButtonRect(), GameplayRect{136, 176, 118, 36}, "discard rect");
    expectRectEqual(gameplaySortButtonRect(), GameplayRect{260, 176, 24, 36}, "sort rect");
    expectRectEqual(gameplayPeekDeckButtonRect(), GameplayRect{290, 176, 24, 36}, "peek rect");
}

void testHudActionHitResolution() {
    expect(resolveGameplayHudAction(20, 180) == GameplayHudAction::Play, "play hit");
    expect(resolveGameplayHudAction(150, 180) == GameplayHudAction::Discard, "discard hit");
    expect(resolveGameplayHudAction(265, 180) == GameplayHudAction::Sort, "sort hit");
    expect(resolveGameplayHudAction(295, 180) == GameplayHudAction::PeekDeck, "peek hit");
    expect(resolveGameplayHudAction(5, 5) == GameplayHudAction::None, "outside hit");
}

void testProgressFillClampsToTrack() {
    expectEqual(roundProgressFillWidth(0, 300, 96), 0, "zero score");
    expectEqual(roundProgressFillWidth(150, 300, 96), 48, "half score");
    expectEqual(roundProgressFillWidth(999, 300, 96), 96, "fill clamp");
}
```

- [ ] **Step 2: Add the new test target**

Add to `CMakeLists.txt` inside the `if(NOT N3DS)` test block:

```cmake
    add_executable(gameplay_layout_tests
        tests/GameplayLayoutTests.cpp
        src/states/GameplayLayout.cpp
    )
    target_include_directories(gameplay_layout_tests PRIVATE src)
    add_test(NAME gameplay_layout_tests COMMAND gameplay_layout_tests)
```

- [ ] **Step 3: Run the new test target and verify it fails**

Run: `cmake -B build`

Expected: configure completes successfully

Run: `cmake --build build --target gameplay_layout_tests`

Expected: compile failure because `GameplayLayout.h/.cpp` do not exist yet

- [ ] **Step 4: Implement the layout module**

Create `src/states/GameplayLayout.h`:

```cpp
#pragma once

struct GameplayRect { int x, y, w, h; };

enum class GameplayHudAction {
    None,
    Play,
    Discard,
    Sort,
    PeekDeck
};

GameplayRect gameplayPlayButtonRect();
GameplayRect gameplayDiscardButtonRect();
GameplayRect gameplaySortButtonRect();
GameplayRect gameplayPeekDeckButtonRect();
GameplayHudAction resolveGameplayHudAction(int px, int py);
int roundProgressFillWidth(int roundScore, int roundTarget, int trackWidth);
```

Create `src/states/GameplayLayout.cpp` following the same pure-layout style as `src/states/ShopLayout.cpp`, with exclusive right/bottom edges for hit tests.

- [ ] **Step 5: Remove obsolete bottom-button rect helpers from `GameplayState.h`**

Delete or stop using:

- `CompactBottomScreenLayout` in `src/states/GameplayState.h:49-70`
- `bottomPlayButtonRect()` in `src/states/GameplayState.h:144-146`
- `bottomDiscardButtonRect()` in `src/states/GameplayState.h:148-155`

Keep `ScreenRect` and top-screen helpers if still needed elsewhere.

- [ ] **Step 6: Re-run the new test**

Run: `cmake --build build --target gameplay_layout_tests`

Expected: build succeeds

Run: `ctest --test-dir build --output-on-failure -R gameplay_layout_tests`

Expected: `100% tests passed`

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt src/states/GameplayLayout.h src/states/GameplayLayout.cpp tests/GameplayLayoutTests.cpp src/states/GameplayState.h
git commit -m "Add a testable layout module for the gameplay bottom HUD"
```

## Task 2: Add Hand Sorting and Deck Peek Primitives

**Files:**
- Modify: `src/game/Hand.h`
- Modify: `src/game/Hand.cpp`
- Modify: `tests/HandTests.cpp`
- Modify: `src/game/Deck.h`
- Modify: `src/game/Deck.cpp`
- Create: `tests/DeckTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing hand sort tests**

Add to `tests/HandTests.cpp`:

```cpp
void testSortByRankPreservesSelectedCards() {
    Hand hand;
    hand.addCard(makePersistentCard(static_cast<int>(Rank::Three), 201));
    hand.addCard(makePersistentCard(static_cast<int>(Rank::King), 202));
    hand.addCard(makePersistentCard(static_cast<int>(Rank::Ace), 203));
    hand.toggleSelect(1);

    hand.sortByRankDescending();

    expectEqual(static_cast<int>(hand.at(0).instanceId), 203, "ace should sort first");
    expectEqual(static_cast<int>(hand.at(1).instanceId), 202, "king should sort second");
    expectEqual(static_cast<int>(hand.getSelectedIndices()[0]), 1, "selected king should stay selected after sort");
}

void testSortBySuitThenRankGroupsSuits() {
    Hand hand;
    hand.addCard(Card{Suit::Spades, Rank::Four, 301});
    hand.addCard(Card{Suit::Hearts, Rank::Jack, 302});
    hand.addCard(Card{Suit::Clubs, Rank::Ace, 303});

    hand.sortBySuitThenRank();

    expectEqual(static_cast<int>(hand.at(0).instanceId), 302, "hearts should sort before clubs/spades");
    expectEqual(static_cast<int>(hand.at(1).instanceId), 303, "clubs should sort after hearts");
}
```

- [ ] **Step 2: Write the failing deck peek tests**

Create `tests/DeckTests.cpp`:

```cpp
#include "game/Deck.h"

void testTopCardsReturnsUpcomingDrawOrderWithoutMutation() {
    Deck deck;
    deck.loadCards({
        Card{Suit::Hearts, Rank::Two, 1},
        Card{Suit::Clubs, Rank::Three, 2},
        Card{Suit::Spades, Rank::Ace, 3}
    });

    const auto top = deck.topCards(2);
    expectEqual(static_cast<int>(top.size()), 2, "topCards size");
    expectEqual(static_cast<int>(top[0].instanceId), 3, "first preview card should match next draw");
    expectEqual(static_cast<int>(top[1].instanceId), 2, "second preview card should match next draw");
    expectEqual(deck.remaining(), 3, "topCards should not mutate deck");
}
```

- [ ] **Step 3: Add the deck test target**

Add to `CMakeLists.txt`:

```cmake
    add_executable(deck_tests
        tests/DeckTests.cpp
        src/game/Deck.cpp
    )
    target_include_directories(deck_tests PRIVATE src)
    add_test(NAME deck_tests COMMAND deck_tests)
```

- [ ] **Step 4: Run the hand/deck tests and verify they fail**

Run: `cmake --build build --target hand_tests deck_tests`

Expected: compile failure because `sortByRankDescending`, `sortBySuitThenRank`, and `topCards` do not exist

- [ ] **Step 5: Implement hand sorting**

Update `src/game/Hand.h`:

```cpp
    void sortByRankDescending();
    void sortBySuitThenRank();
```

Implement in `src/game/Hand.cpp` by sorting `m_cards` directly so the `HeldCard` selection flags move with the cards:

```cpp
void Hand::sortByRankDescending() {
    std::stable_sort(m_cards.begin(), m_cards.end(), [](const HeldCard& left, const HeldCard& right) {
        const int leftRank = left.card.rank == Rank::Ace ? 14 : static_cast<int>(left.card.rank);
        const int rightRank = right.card.rank == Rank::Ace ? 14 : static_cast<int>(right.card.rank);
        return leftRank > rightRank;
    });
}
```

Use a second stable comparator for `sortBySuitThenRank()` so suits group consistently and ranks remain descending inside each suit.

- [ ] **Step 6: Implement deck peek**

Update `src/game/Deck.h`:

```cpp
    std::vector<Card> topCards(int count) const;
```

Implement `topCards()` in `src/game/Deck.cpp` by reading from `m_cards.rbegin()` forward without popping.

- [ ] **Step 7: Re-run the tests**

Run: `cmake --build build --target hand_tests deck_tests`

Expected: build succeeds

Run: `ctest --test-dir build --output-on-failure -R "hand_tests|deck_tests"`

Expected: both targets pass

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt src/game/Hand.h src/game/Hand.cpp src/game/Deck.h src/game/Deck.cpp tests/HandTests.cpp tests/DeckTests.cpp
git commit -m "Add hand sorting and non-destructive deck peek primitives"
```

## Task 3: Rebuild the Bottom HUD Around the Four-Band Layout

**Files:**
- Modify: `src/states/GameplayState.h:49-70, 191-206`
- Modify: `src/states/GameplayState.cpp:483-585`
- Modify: `src/states/GameplayState.cpp:1-80` (add local draw helpers)
- Modify: `src/states/GameplayState.cpp:332-482` if top-screen/boss text alignment needs matching updates

- [ ] **Step 1: Add HUD state to `GameplayState`**

Add minimal new state in `src/states/GameplayState.h`:

```cpp
enum class GameplaySortMode {
    Rank,
    Suit
};

    GameplaySortMode m_sortMode;
    bool m_showPeekDeck;
```

Initialize them in the constructor and reset them in `enter()` / `startNewRound()` so a new round starts in a predictable HUD state.

- [ ] **Step 2: Add small local render helpers in `GameplayState.cpp`**

At the top of `src/states/GameplayState.cpp`, add pure-ish draw helpers such as:

```cpp
void drawHudCell(ScreenRenderer& r, int x, int y, int w, int h,
                 const std::string& label, const std::string& value);
void drawPrimaryAction(ScreenRenderer& r, const GameplayRect& rect,
                       const char* label, int rCol, int gCol, int bCol, bool enabled);
```

Keep these local to the file. Do not introduce a new rendering abstraction layer.

- [ ] **Step 3: Rewrite the `Playing` bottom-screen branch**

Replace `src/states/GameplayState.cpp:518-577` with the four-band HUD:

- top strip: blind name, round score, target, money
- optional thin progress track under the strip
- scoring cluster: selected count, hand type, chips, mult
- run-state row: hands, discards, ante, round
- action band: play, discard, sort, peek deck

Rules:

- remove the single-line `compactStatusLine(...)` text
- do not render persistent `Options` or `Run Info`
- keep boss text only if it still fits cleanly below the main HUD bands

- [ ] **Step 4: Rewrite the `Scoring` bottom-screen branch**

Update `src/states/GameplayState.cpp:486-517` so `Scoring` uses the same four-band structure as `Playing`, with only these changes:

- round score uses `m_scorer->displayRoundScore()`
- chips and mult use `m_scorer->displayChips()` / `displayMult()`
- hand type uses `m_scorer->handType()`
- primary action buttons draw in a disabled style

- [ ] **Step 5: Build and verify the game target**

Run: `cmake --build build --target balatro_demake`

Expected: target builds successfully

- [ ] **Step 6: Commit**

```bash
git add src/states/GameplayState.h src/states/GameplayState.cpp
git commit -m "Render the gameplay bottom screen as a four-band HUD"
```

## Task 4: Wire Button Mapping, Sort, and Peek Deck Overlay

**Files:**
- Modify: `src/states/GameplayState.cpp:196-324`
- Modify: `src/states/GameplayState.h:184-206`
- Modify: `src/states/GameplayState.cpp:483-585`

- [ ] **Step 1: Add the failing behavior checks to the existing logic tests**

Extend `tests/GameplayLayoutTests.cpp` with expectations for the two smaller actions:

```cpp
void testSmallActionButtonsStillHaveExclusiveEdges() {
    expect(resolveGameplayHudAction(260, 176) == GameplayHudAction::Sort, "sort top-left");
    expect(resolveGameplayHudAction(284, 176) == GameplayHudAction::None, "sort right edge excluded");
    expect(resolveGameplayHudAction(290, 176) == GameplayHudAction::PeekDeck, "peek top-left");
}
```

This keeps the touch surface honest as the input work lands.

- [ ] **Step 2: Re-run the layout test before input changes**

Run: `cmake --build build --target gameplay_layout_tests`

Expected: test still passes; if not, fix the geometry before wiring input

- [ ] **Step 3: Rework the 3DS button map**

Update `src/states/GameplayState.cpp:204-234`:

- `KEY_A` toggles current card selection
- `KEY_X` calls `playHand()`
- `KEY_Y` calls `discardSelected()`
- `KEY_R` toggles `m_showPeekDeck`

When `m_showPeekDeck` is true:

- suppress `playHand()` / `discardSelected()`
- allow `KEY_R` or tapping the peek button to close the overlay

- [ ] **Step 4: Rework the desktop keyboard/mouse path**

Update `src/states/GameplayState.cpp:262-315`:

- keep `Space` for selection toggle
- keep `Return` for play and `D` for discard
- add `S` for sort toggle
- add `Tab` for peek toggle
- route mouse clicks through `resolveGameplayHudAction(...)` instead of hand-written play/discard rect checks

- [ ] **Step 5: Implement touch/mouse handling for all four action buttons**

Use `resolveGameplayHudAction(...)` in both the 3DS touch path and the desktop mouse path. Avoid duplicating play/discard-only hit logic.

- [ ] **Step 6: Render the peek overlay**

When `m_showPeekDeck` is true in `renderBottomScreen()`:

- draw a modal panel over the bottom HUD
- label it `Peek Deck`
- show `m_runState->roundDeck().topCards(5)` in upcoming draw order
- show `remaining()` count
- show a small footer hint like `B/R to close` on 3DS and `Tab/click to close` on desktop

Do not change top-screen card selection rendering while the overlay is open.

- [ ] **Step 7: Run the focused test/build pass**

Run: `cmake --build build --target gameplay_layout_tests hand_tests deck_tests balatro_demake`

Expected: all targets build

Run: `ctest --test-dir build --output-on-failure -R "gameplay_layout_tests|hand_tests|deck_tests|scoring_animator_tests|run_state_tests"`

Expected: all listed tests pass

- [ ] **Step 8: Manual verification**

Run the game on desktop and verify:

- `Space` still toggles selection
- `Return` plays the hand
- `D` discards
- `S` toggles the sort mode and visibly reorders cards
- `Tab` opens/closes the peek overlay
- mouse clicks hit all four bottom-screen buttons correctly
- `Scoring` keeps the same HUD structure with disabled action buttons

- [ ] **Step 9: Commit**

```bash
git add src/states/GameplayState.h src/states/GameplayState.cpp tests/GameplayLayoutTests.cpp
git commit -m "Wire the new gameplay HUD controls and deck peek overlay"
```

## Task 5: Final Verification and Cleanup

**Files:**
- Modify only if verification uncovers issues

- [ ] **Step 1: Run the full desktop test suite**

Run: `ctest --test-dir build --output-on-failure`

Expected: all desktop tests pass

- [ ] **Step 2: Build one final time**

Run: `cmake --build build --target balatro_demake`

Expected: successful build with no new warnings introduced by this feature

- [ ] **Step 3: Inspect git status**

Run: `git status --short`

Expected: clean working tree

- [ ] **Step 4: Write the completion summary**

Capture:

- changed files
- simplifications made
- remaining risks:
  - no pause-screen integration yet
  - 3DS physical fit and touch comfort still need device validation

- [ ] **Step 5: Commit any final verification fixes if needed**

```bash
git add <resolved-files>
git commit -m "Polish and verify the 3DS gameplay bottom HUD rollout"
```

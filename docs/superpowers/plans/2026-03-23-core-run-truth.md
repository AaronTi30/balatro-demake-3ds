# Core Run Truth Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce persistent run state and a parity-oriented scoring/progression foundation so a single run behaves more like Balatro.

**Architecture:** Add a shared `RunState` model that owns ante, money, round target, hands, discards, jokers, and deck state across screens. Refactor scoring into explicit classification and score-calculation helpers, then move gameplay/shop progression logic onto the shared run model.

**Tech Stack:** C++17, CMake, SDL2, citro2d/citro3d, existing game state machine

---

## File Map

- Create: `src/game/RunState.h`
- Create: `src/game/RunState.cpp`
- Modify: `src/states/GameplayState.h`
- Modify: `src/states/GameplayState.cpp`
- Modify: `src/states/ShopState.h`
- Modify: `src/states/ShopState.cpp`
- Modify: `src/states/TitleState.cpp`
- Modify: `src/game/HandEvaluator.h`
- Modify: `src/game/HandEvaluator.cpp`
- Modify: `src/game/Deck.h`
- Modify: `src/game/Deck.cpp`
- Modify: `CMakeLists.txt`

### Task 1: Add Persistent RunState

**Files:**
- Create: `src/game/RunState.h`
- Create: `src/game/RunState.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Define the `RunState` interface**

Add a class that owns:

- `ante`
- `roundTarget`
- `money`
- `handsRemaining`
- `discardsRemaining`
- `jokerLimit`
- `std::vector<Joker> jokers`
- `Deck deck`

Include methods for:

- `startNewRun()`
- `startRound()`
- `awardRoundWin()`
- `advanceAnte()`
- `isRoundWon() const`
- `isRunComplete() const`

- [ ] **Step 2: Wire the new files into the build**

Update `CMakeLists.txt` to compile `src/game/RunState.cpp`.

- [ ] **Step 3: Build to catch interface issues early**

Run: `cmake --build build`
Expected: compilation reaches existing state files and fails only where `RunState` integration is still missing.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt src/game/RunState.h src/game/RunState.cpp
git commit -m "feat: add persistent run state model"
```

### Task 2: Thread RunState Through Game Screens

**Files:**
- Modify: `src/states/GameplayState.h`
- Modify: `src/states/GameplayState.cpp`
- Modify: `src/states/ShopState.h`
- Modify: `src/states/ShopState.cpp`
- Modify: `src/states/TitleState.cpp`

- [ ] **Step 1: Update constructors to accept shared run ownership**

Use `std::shared_ptr<RunState>` so `TitleState` creates the run, `GameplayState` mutates round state, and `ShopState` mutates shop-related progression without copying run data.

- [ ] **Step 2: Replace duplicated progression fields**

Remove or stop using local copies for:

- ante
- money
- jokers
- round target
- hands/discards when they are part of run state

Read them from the shared `RunState` instead.

- [ ] **Step 3: Move round start/end transitions onto RunState**

`GameplayState` should call `startRound()`, update score via run-owned round data, and transition to shop/title based on `RunState` predicates.

- [ ] **Step 4: Update shop transitions**

`ShopState` should return to gameplay by reusing the same shared run object instead of reconstructing state from copied values.

- [ ] **Step 5: Build and smoke-check**

Run: `cmake --build build`
Expected: successful build.

- [ ] **Step 6: Commit**

```bash
git add src/states/GameplayState.h src/states/GameplayState.cpp src/states/ShopState.h src/states/ShopState.cpp src/states/TitleState.cpp
git commit -m "refactor: move run progression into shared state"
```

### Task 3: Refactor Hand Evaluation Into an Explicit Pipeline

**Files:**
- Modify: `src/game/HandEvaluator.h`
- Modify: `src/game/HandEvaluator.cpp`

- [ ] **Step 1: Introduce explicit helpers**

Split the evaluator into small helpers for:

- hand classification
- scoring-card selection
- base-value lookup
- final total calculation

- [ ] **Step 2: Expand the result model**

Ensure the final score result exposes:

- detected hand
- scoring cards
- base chips/mult
- final chips/mult
- final score

This metadata is needed for UI explanation and future effect triggers.

- [ ] **Step 3: Preserve current behavior where parity decisions are not yet implemented**

Do not add speculative systems. Keep joker application compatible with the current callback approach, but make the ordering explicit and isolated.

- [ ] **Step 4: Build and manually verify score paths**

Run: `cmake --build build`
Expected: successful build.

Manual verification:

- Play a pair hand
- Play a flush/straight if available
- Confirm result display uses the new score breakdown without regression

- [ ] **Step 5: Commit**

```bash
git add src/game/HandEvaluator.h src/game/HandEvaluator.cpp
git commit -m "refactor: make hand scoring pipeline explicit"
```

### Task 4: Move Ante Target and Reward Logic Into RunState

**Files:**
- Modify: `src/game/RunState.h`
- Modify: `src/game/RunState.cpp`
- Modify: `src/states/GameplayState.cpp`

- [ ] **Step 1: Centralize round target calculation**

Move ante target lookup and any next-round setup logic from `GameplayState` into `RunState`.

- [ ] **Step 2: Centralize reward handling**

Move round-win money rewards into `RunState::awardRoundWin()` so screen code no longer owns economy logic.

- [ ] **Step 3: Update gameplay flow to use the centralized API**

`GameplayState` should ask `RunState` whether the round is won, whether the run is complete, and what the next state should represent.

- [ ] **Step 4: Build and verify progression**

Run: `cmake --build build`
Expected: successful build.

Manual verification:

- Win a round and confirm money increases through `RunState`
- Enter shop and confirm money/jokers persist
- Start the next round and confirm ante target updates correctly

- [ ] **Step 5: Commit**

```bash
git add src/game/RunState.h src/game/RunState.cpp src/states/GameplayState.cpp
git commit -m "refactor: centralize ante targets and round rewards"
```

### Task 5: Stabilize Deck Persistence for Future Parity Work

**Files:**
- Modify: `src/game/Deck.h`
- Modify: `src/game/Deck.cpp`
- Modify: `src/game/RunState.cpp`

- [ ] **Step 1: Verify deck ownership lives in RunState**

Ensure the deck is no longer recreated by gameplay screen construction and instead persists through the run model.

- [ ] **Step 2: Keep current reset semantics only where needed**

For now, reset/shuffle at round start only if that matches the current intended behavior for the prototype. The important change is that deck lifetime is run-owned, ready for future persistent deck modifications.

- [ ] **Step 3: Build and smoke-test draw/discard flow**

Run: `cmake --build build`
Expected: successful build.

Manual verification:

- Start a run
- Play and discard multiple hands
- Confirm no crashes or duplicate initialization issues

- [ ] **Step 4: Commit**

```bash
git add src/game/Deck.h src/game/Deck.cpp src/game/RunState.cpp
git commit -m "refactor: make deck ownership run-scoped"
```

### Task 6: Final Verification

**Files:**
- Modify: any files touched above as needed

- [ ] **Step 1: Build PC target**

Run: `cmake --build build`
Expected: successful build with no compile errors.

- [ ] **Step 2: Build 3DS target if toolchain is available**

Run: `cmake --build build-3ds`
Expected: successful build, or a clearly documented environment/toolchain failure unrelated to code changes.

- [ ] **Step 3: Manual gameplay verification**

Verify:

- Title screen starts a run successfully
- Gameplay uses shared run values
- Winning enters shop with persistent money/jokers
- Leaving shop returns to gameplay with the same run
- Losing returns cleanly to title

- [ ] **Step 4: Commit the final integration**

```bash
git add CMakeLists.txt src/game/RunState.h src/game/RunState.cpp src/game/HandEvaluator.h src/game/HandEvaluator.cpp src/game/Deck.h src/game/Deck.cpp src/states/GameplayState.h src/states/GameplayState.cpp src/states/ShopState.h src/states/ShopState.cpp src/states/TitleState.cpp
git commit -m "feat: add parity-oriented run state foundation"
```

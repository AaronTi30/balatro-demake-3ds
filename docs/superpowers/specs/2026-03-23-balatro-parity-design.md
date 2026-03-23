# Balatro Parity Design

## Goal

Bring the demake closer to the original Balatro by preserving the run structure, scoring logic, economy pressure, and buildcraft decisions that define the game's feel, while compressing presentation only where the 3DS form factor requires it.

## Design Principles

### Gameplay First

Parity should be judged by how a run plays, not by whether every visual or content surface is already present. The primary target is decision tension: what the player chooses to play, hold, buy, sell, skip, and scale.

### Compress Presentation, Not Systems

The 3DS version can simplify layout density, animations, and information presentation. It should not simplify away the systems that create Balatro's core rhythm: scaling blinds, money pressure, hand valuation, and synergistic joker/deck growth.

### Persistent Run State

A run must stop feeling screen-driven and start feeling system-driven. Ante, money, deck contents, joker inventory, hand levels, and future economy hooks should live in a persistent run model rather than being recomputed ad hoc inside gameplay/shop states.

## Current Gaps

The current project has a playable shell but not parity-grade run logic.

- `GameplayState` owns transient round state and basic ante progression.
- `HandEvaluator` simplifies scoring and scoring-card selection.
- `Deck` resets to a plain 52-card deck each round.
- `ShopState` offers two random jokers with simple prices and no broader economy structure.
- `Joker` supports only a small pool and a flat evaluation callback model.

This is enough for a prototype but not enough to preserve Balatro's essence.

## Target Architecture

### Run State

Add a dedicated run model that persists across gameplay and shop screens.

Responsibilities:

- Track ante, blind/round progression, score target, money, hands, discards, joker slots, current jokers, and persistent deck state.
- Own rewards and penalties that carry from one round to the next.
- Provide screen states with a stable source of truth instead of duplicating progression logic.

### Scoring Pipeline

Refactor scoring into a deterministic pipeline with explicit phases.

Suggested phases:

1. Determine the played hand and the scoring cards.
2. Compute base hand chips/mult.
3. Add scoring-card chip values.
4. Apply joker effects in a stable order.
5. Produce final score and result metadata for UI feedback.

This pipeline must be extensible enough for future additions like card enhancements, editions, seals, and consumables without forcing another rewrite.

### Blind and Round Structure

Gameplay should represent a run as a sequence of increasingly demanding rounds rather than repeated local rounds. Blind targets and round rewards need to create the same pressure curve that makes Balatro runs feel deliberate instead of arcade-like.

### Economy and Shop Structure

The shop should become a run-progression surface rather than a random reward screen.

Required behavior direction:

- Money earned from round success follows a stable payout model.
- The player has meaningful tradeoffs between saving, buying, and later rerolling/selling.
- Joker capacity is enforced as a strategic limit.
- Shop inventory generation supports future expansion beyond jokers.

## Phased Delivery

### Milestone 1: Core Run Truth

Focus on the systems that determine whether a run feels correct.

- Introduce persistent `RunState`.
- Refactor scoring and scoring-card logic.
- Add proper ante/blind target progression and round reward handling.

Success criteria:

- A single run feels structurally closer to Balatro even with limited content.
- Score outcomes are explainable and mechanically consistent.
- Gameplay and shop screens read/write the same persistent run state.

### Milestone 2: Decision Pressure

Focus on between-round choices.

- Rebuild the shop around economy pressure.
- Add buy/sell/reroll style decisions.
- Expand joker logic with more faithful trigger categories and ordering.

Success criteria:

- Shop choices meaningfully affect later rounds.
- Joker builds can form recognizable synergies.
- Money management becomes a core run decision.

### Milestone 3: Build Depth and Surface Parity

Focus on content breadth and feel.

- Persistent deck manipulation.
- Card modifiers and consumable systems.
- UX and presentation polish for PC and 3DS.

Success criteria:

- Runs diverge in recognizable Balatro-like ways.
- Feedback, pacing, and screen flow feel intentional.

## Milestone 1 Detailed Design

### RunState Boundary

Introduce a new game-level object, tentatively `RunState`, owned outside individual states and passed by shared ownership into `GameplayState` and `ShopState`.

Initial responsibilities:

- `ante`
- `roundIndex` or blind stage identifier
- `money`
- `scoreThisRound`
- `roundTarget`
- `handsRemaining`
- `discardsRemaining`
- `jokerLimit`
- `jokers`
- `deck`

This should intentionally leave room for future additions without prematurely implementing all of Balatro's systems.

### Hand Evaluation Boundary

`HandEvaluator` should stop mixing hand detection, scoring-card selection, and joker application in a way that is difficult to evolve.

Refactor around:

- Hand classification helpers
- Scoring-card extraction helpers
- A stable score result object with enough metadata for the UI and future effect triggers

### Screen Responsibilities

`GameplayState` should:

- Read round parameters from `RunState`
- Apply play/discard actions to round state
- Trigger end-of-round transition logic

`ShopState` should:

- Read/write money and joker inventory from `RunState`
- Generate inventory based on run progression
- Return control to gameplay without reconstructing run information manually

## Testing Strategy

The project currently has no automated tests. Milestone 1 should introduce focused tests around pure game logic first.

Priority test targets:

- Hand classification
- Scoring-card selection
- Final chip/mult calculation
- Blind target progression
- Run-state round transitions

Pure logic tests provide the best return because they protect the parity-sensitive parts of the game without depending on SDL or 3DS rendering.

## Risks

### Overfitting to UI Before System Truth

Adding more shop visuals or more jokers before the scoring/economy model is corrected will produce visible progress but weak parity.

### Embedding More Run Logic in Screen States

If progression remains split across `GameplayState` and `ShopState`, future parity work will become harder and more error-prone.

### Expanding Content Before Trigger Semantics Are Stable

Adding many jokers before the effect pipeline is stable will create rework when more authentic ordering and trigger categories are introduced.

## Recommendation

Start implementation with Milestone 1 only:

1. Add `RunState`.
2. Refactor scoring into a future-proof pipeline.
3. Move ante/round target/reward flow into the persistent run model.

This yields the highest parity gain per unit of work and lays the foundation for every later system.

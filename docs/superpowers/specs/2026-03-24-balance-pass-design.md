# Balance Pass Design

## Goal

Make the demake feel closer to Balatro's tension curve without a full systems rewrite. The first pass should stop early joker snowballing, make shops less reliably positive, and add one recurring external pressure source through boss blinds.

## Design Principles

### Power Must Be Conditional

Jokers should stop functioning as cheap, automatic upgrades. Early power should help the player survive, but not remove the need to adapt hand choice or economy decisions.

### Pressure Must Come From Both Sides

Balatro feels tight because player power and game pressure rise together. This pass should add both:

- weaker and less reliable shop gains
- boss blind rules that periodically suppress dominant strategies

### Keep Scope Small Enough To Tune

This is a first-pass rebalance, not a full parity rewrite. The changes should fit the current code structure and stay limited to economy, shop generation, joker pool strength, and boss-blind score modifiers.

## Current Balance Problems

The current demake becomes trivial after one or two jokers for structural reasons:

- the joker pool is tiny and all offers are strong positive effects
- shop offers are cheap relative to blind rewards
- blind targets do not rise fast enough to offset early multiplier growth
- there is no boss-level pressure that forces the player off a single dominant line

As a result, the player can buy early power with almost no tradeoff and carry that power through multiple blinds without being meaningfully challenged.

## Target Outcome

After this pass:

- ante 1 remains beatable with no joker
- one joker helps but does not solve the run
- some shop visits produce awkward or skippable options
- boss blinds occasionally force a different hand choice
- mid-run success depends more on build fit than on buying the first available joker

## Proposed Balance Model

### 1. Economy Retune

Keep the starting economy small, but reduce the rate at which the player converts wins into guaranteed joker purchases.

Recommended reward values:

- small blind: `$2`
- big blind: `$3`
- boss blind: `$4`

Keep starting money at `$4`.

### 2. Blind Target Retune

Use a moderate early ramp so ante 1 is comfortable but ante 2-3 become the first real build check.

Recommended first-pass targets:

- ante 1: `300 / 450 / 600`
- ante 2: `900 / 1350 / 1800`
- ante 3: `2400 / 3600 / 4800`

Ante 4 and above should continue scaling sharply from there, but this pass does not need a perfect late-game table yet. The important change is making ante 2-3 meaningfully test whether the player actually has a build.

### 3. Tiered Joker Pool

Replace the current "all useful" pool with a small tiered pool.

Weak jokers:

- `Plain Joker`: `+2 Mult`
- `Greedy Joker`: `+20 Chips if Pair`
- `Suit Joker`: `+2 Mult per scoring Spade`

Medium jokers:

- `Focused Joker`: `+8 Mult if <= 3 cards played`
- `Flush Joker`: `+40 Chips if played hand is Flush`
- `Straight Joker`: `+40 Chips if played hand is Straight`

Strong jokers:

- `Heavy Joker`: `+5 Mult`
- `Aggro Joker`: `+60 Chips if Pair or better`
- `Precision Joker`: `+12 Mult if exactly 1 card scores`

This tiering is meant to create variance and build direction, not just lower numbers.

### 4. Shop Quality and Price Bands

Each shop should still offer two jokers, but not both from the same flat pool.

Generation rule:

- slot 1: guaranteed `weak or medium`
- slot 2: weighted from the full pool

Recommended weights for slot 2:

- weak: `50%`
- medium: `35%`
- strong: `15%`

Recommended price bands:

- weak: `$4-$5`
- medium: `$6-$7`
- strong: `$8-$10`

This preserves shop usefulness while removing the current "buy almost anything and get stronger" pattern.

### 5. Lightweight Boss Blind Modifiers

Only boss blinds should get rule modifiers in this pass.

Each boss blind rolls one modifier from a fixed list and shows it before the round begins.

Recommended initial modifier set:

- `Pair Tax`: pair and two-pair hands score at `75%` of normal final score
- `Small Hand Punish`: hands with `<= 3` played cards score at `70%`
- `Suit Lock`: scoring cards of one blocked suit contribute no rank-chip bonus
- `Face Tax`: J/Q/K/A scoring cards contribute half chip bonus
- `High Card Wall`: high card and pair hands score at `70%`

Rules:

- one modifier per boss blind
- modifier affects final score or chip contribution only
- no modifier should change hand classification in this pass

This is intentionally lightweight. The purpose is to create periodic disruption, not to build a full boss content system yet.

## System Boundaries

### RunState

`RunState` should own:

- blind reward values
- blind target values
- current boss modifier
- next boss modifier preview if needed for shop/UI

### Joker

`Joker` should gain enough metadata to support:

- strength tier
- shop generation weighting
- price band or price rule

### ShopState

`ShopState` should:

- generate one weaker slot and one weighted slot
- assign prices from tier-based ranges
- display prices and current offers without any other economy rewrite

### HandEvaluator

`HandEvaluator` should remain the place where score is finalized, but it should accept an optional boss modifier step that adjusts score after normal hand/joker scoring.

### GameplayState

`GameplayState` should render:

- current boss modifier during boss rounds
- next boss modifier preview if the run-state model exposes one in the shop transition flow

## What This Pass Does Not Include

To keep scope controlled, do not add:

- consumables
- rerolls
- sell mechanics
- interest
- hand levels
- deck editing systems
- a large content expansion beyond the initial tiered joker set

Those systems may matter later, but they are not required to make the current prototype stop collapsing after one or two good purchases.

## Acceptance Criteria

This pass is successful if:

- the player can beat ante 1 with no joker
- the player cannot reliably trivialize ante 2-3 from one cheap early joker
- shops sometimes present a reasonable skip decision
- boss blinds occasionally force a change in hand selection
- runs still allow high-roll outcomes, but not from almost every shop visit

## Recommended Next Step

Turn this design into a focused implementation plan with three tasks:

1. retune economy and blind targets
2. add tiered joker pool plus weighted shop generation
3. add lightweight boss-blind modifiers and UI text

That keeps the first balance pass testable and easy to hand off.

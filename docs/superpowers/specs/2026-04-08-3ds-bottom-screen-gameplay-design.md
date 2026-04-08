# 3DS Bottom Screen Gameplay HUD Design
_2026-04-08_

## Goal

Redesign the gameplay bottom screen for the 3DS layout so it preserves Balatro's dashboard feel without trying to literally shrink the desktop left rail into unreadable UI. The bottom screen should support fast gameplay, keep the most important run state visible, and leave room for button-first controls with touch as a bonus.

## Context

The desktop reference uses a tall left-side rail to hold blind info, score state, chips, mult, money, hands, discards, ante, round, and utility buttons. That structure does not map directly to the 3DS bottom screen, which is short and wide and must also support action controls.

The design target is parity in information hierarchy and feel, not one-to-one visual reproduction.

## Design Principles

### Compress the rail into bands, not tiny boxes

The bottom screen should preserve the same categories of information as Balatro, but reorganize them into horizontal bands that fit the 3DS aspect ratio.

### Keep live play information on the main HUD

The player should not need to open a secondary panel to see the current blind, target, money, chips, mult, hand preview, hands, discards, ante, or round.

### Favor stable layout over phase-specific rearrangement

The bottom screen should keep the same overall structure during normal play and scoring. Emphasis may change, but the player should not have to re-learn where information lives.

### Button controls first, touch second

The screen should read clearly for players using physical controls, with touch interaction available as a convenience rather than a requirement.

## Confirmed Decisions

- Remove the persistent `Options` button from the gameplay HUD.
- Do not keep a dedicated `Run Info` button on the gameplay HUD.
- Expanded run details move behind `Pause`.
- Keep `money` always visible on the main gameplay HUD.
- Keep `Sort` and `Peek Deck` on the main HUD if they fit at a readable and tappable size.
- Group `hand info` with the scoring display rather than burying it inside the compact run-state row.
- Optimize the gameplay HUD for button controls, with touch as a bonus.

## Bottom Screen Layout

The gameplay HUD is split into four horizontal bands.

### Band 1: Top Info Strip

The top band shows:

- current blind name
- current round score
- target score
- money

This band replaces the desktop blind header and cash block with a compact strip that stays visible throughout gameplay. The score and target relationship may be reinforced with a thin progress treatment directly beneath the strip if it fits cleanly.

### Band 2: Scoring Cluster

The second band is the visual center of the bottom screen. It shows:

- current hand type
- selected count
- chips
- mult

This area should feel like the compressed equivalent of Balatro's chips and multiplier presentation. Chips and mult remain large and color-distinct. Hand type and selected count sit in the same cluster because they are part of the player's immediate scoring decision.

### Band 3: Run-State Row

The third band is a compact row of mini-cells for:

- hands remaining
- discards remaining
- ante
- round

These values stay visible, but are visually secondary to the scoring cluster.

### Band 4: Action Band

The bottom band contains the primary gameplay controls:

- `Play Hand`
- `Discard`
- `Sort`
- `Peek Deck`

`Play Hand` and `Discard` should be the clearest actions in the row. `Sort` and `Peek Deck` should remain visible only if they can fit without becoming tiny leftover buttons. If space becomes too tight, `Peek Deck` is the first candidate to demote into a smaller secondary treatment.

## Interaction Model

### Primary controls

Gameplay is designed for physical controls first:

- card selection remains driven by buttons and directional input on the top-screen hand
- `A` maps to `Play Hand`
- `Y` maps to `Discard`

Touch should still support tapping the bottom-screen buttons directly, but it is not the primary control path.

### Sort behavior

`Sort` should act as an in-place toggle rather than a deep submenu. The control can cycle between rank and suit ordering while staying in the same bottom-screen position.

### Peek Deck behavior

`Peek Deck` can temporarily take over the bottom screen or open a lightweight overlay. When dismissed, gameplay returns to the same bottom-screen HUD without rearranging the rest of the layout.

### Pause behavior

`Pause` becomes the home for:

- expanded run details
- control reminders
- settings and options

This replaces the need for dedicated `Run Info` and `Options` buttons during active play.

## Screen State Behavior

### Playing

During normal play, all four bands are visible. As the player changes card selection, the scoring cluster updates the selected count and hand preview live.

### Scoring

During scoring, the bottom screen keeps the same structure, but the scoring cluster becomes more prominent:

- hand type remains visible
- chips and mult animate in place
- current round score visibly advances toward the updated total
- score-related feedback uses the same visual region as normal play
- `Play Hand` and `Discard` appear disabled while scoring is in progress

This preserves continuity between selection and scoring.

### Peek Deck

`Peek Deck` may temporarily replace the normal HUD on the bottom screen. Closing it should restore the exact same gameplay layout.

### Pause

Pause fully replaces the gameplay HUD with the expanded info/options surface.

## Implementation Direction

This redesign should remain largely inside the gameplay UI boundary rather than forcing a broader architecture change.

Primary implementation surface:

- `src/states/GameplayState.h`
- `src/states/GameplayState.cpp`

Expected work:

- redefine `CompactBottomScreenLayout` around the new four-band structure
- replace the current single-line status text with explicit mini-cells
- promote hand preview into a clearer scoring cluster
- add layout support for `Sort` and `Peek Deck`
- remove any expectation of persistent `Options` or `Run Info` gameplay buttons
- keep the same bottom-screen structure during `Playing` and `Scoring`

## Guardrails

- Do not try to reproduce the desktop rail one panel at a time on the 3DS.
- Do not hide core gameplay state behind extra taps.
- Do not let `Sort` and `Peek Deck` shrink the primary actions below a readable size.
- Do not make scoring use a completely different bottom-screen layout unless testing proves the stable-layout approach fails.

## Out of Scope

- Pause menu redesign beyond defining its responsibility as the home for expanded info and options
- Shop bottom-screen layout
- Blind-select bottom-screen layout
- Final visual polish details such as exact colors, pixel art chrome, and animation timing

## Recommendation

Implement the bottom screen as a balanced hybrid HUD:

- compact top strip for blind, target, and money
- visible round score and target relationship in the top strip
- prominent scoring cluster for hand info, chips, and mult
- slim run-state row for hands, discards, ante, and round
- bottom action band for play/discard/sort/peek deck

This approach best preserves Balatro's dashboard identity while fitting the realities of the 3DS bottom screen.

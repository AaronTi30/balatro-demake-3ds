# Shop Joker Inspect Panel Design

## Goal

Give the player a readable way to see what each shop joker does without relying solely on the cramped description text rendered on the card itself.

## Design Principles

- No new state or data needed — `Joker` already carries `name` and `description`
- Keep the top screen unchanged; use the bottom screen's existing empty space
- Selection model must work identically on 3DS (D-pad) and PC (mouse hover)

## Interaction Model

**Trigger:** selection/highlight — whichever joker `m_cursorIndex` points to is the "inspected" joker.

- **3DS:** D-pad left/right already moves `m_cursorIndex`. No change needed.
- **PC:** Mouse hover must update `m_cursorIndex` each frame (currently only mouse click updates it). This requires moving hit-test logic out of the click handler and into a per-frame hover pass.

## Bottom Screen Layout

The bottom screen currently has:
- `"SHOP CONTROLS"` header at y=30 (removed — redundant once the panel is present)
- `"Jokers: X/5"` at y=50–60
- Buy and Next Blind buttons at y=160

Replace with:

```
[y=20]  <Joker Name>            — bright white, scale 1
[y=45]  <Joker Description>     — dim grey, scale 0 (small)
[y=80]  Jokers: X/5             — unchanged
[y=160] [Buy] [Next Blind]      — unchanged
```

When no joker is selected (empty shop or cursor out of range):
- 3DS: `"Use D-pad to inspect"`
- PC: `"Hover a card to inspect"`

Both in dim grey at the same y=20/45 positions.

## Top Screen Card

Remove the description line from the joker card render. The card becomes: name at top, price at bottom. This cleans up the cramped card layout and makes the bottom panel the single source of description truth.

## Files

- Modify: `src/states/ShopState.cpp` — hover logic, bottom screen render, card render

## Out of Scope

- Sell jokers
- Reroll shop
- Comparing held jokers vs shop jokers

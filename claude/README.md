# Claude collaboration system

This folder is how **you** (Justin) and **Claude** keep each other in sync while building Analog Strike. The basic loop:

```
   ┌──────────────────────────────────────────────┐
   │  1. You edit / play / break something        │
   │  2. You run ./tools/claude/snapshot.sh       │
   │  3. claude/snapshot/ is populated            │
   │  4. You commit + push (or just tell Claude)  │
   │  5. Claude reads the snapshot and responds   │
   │  6. Claude commits code changes back         │
   └──────────────────────────────────────────────┘
```

## What each piece is for

### `claude/snapshot/`  *(auto-generated — do not hand-edit)*

What `tools/claude/snapshot.sh` writes here whenever you run it:

| File | What's in it |
| --- | --- |
| `state.json` | Full level inventory — every actor's class, label, position, and mesh; lighting summary; totals. |
| `layout_top.png` | A deterministic top-down minimap of the level rendered locally from `state.json`. Always works (no UE rendering needed). Color-coded: green = trees/terrain, brown = walls/floors, red = enemies, blue = objectives, etc. |
| `screenshots/*.png` | Real UE5 screenshots from 3 angles (overhead, aerial, ground) in two modes: `*_lit.png` (with lighting, the way the game looks) and `*_flat.png` (unlit base color — guaranteed visible even if lighting is broken). |
| `log_tail.txt` | Last ~300 lines of the most recent UE editor log — surfaces errors, warnings, missing assets. |
| `raw_shots/` | Pre-postprocess versions of the screenshots — usually ignore. |
| `_editor_run.log` | Stdout from the headless editor run that produced this snapshot. |

### `claude/inbox/`  *(you write here)*

Drop a markdown note here when you want Claude to focus on something specific. There's a template — copy it and describe what you're seeing / what's wrong / what to change. Claude reads `claude/inbox/*.md` on the next session.

## Running a snapshot

```bash
# Close the UE5 editor first (file locks)
./tools/claude/snapshot.sh
```

Takes a few minutes (the editor boots offscreen, runs the snapshot script, renders, exits). When it finishes you'll see a summary in the terminal and the `claude/snapshot/` folder will be populated.

Then just tell Claude *"I ran snapshot"* — they'll read the folder and pick up where you left off, with eyes on the actual current state of the game.

## Why this exists

Throughout the build, the loop has been: Claude writes code blind → you run it → you describe what happened → Claude guesses at the fix. Many rounds get wasted. With the snapshot system, Claude reads `state.json` (knows exactly what's in the level), looks at `screenshots/` (sees how it looks), and scans `log_tail.txt` (catches errors). That collapses a multi-turn debug loop into one turn.

## Limits / honest caveats

- The lit screenshots (`*_lit.png`) sometimes come out black because of UE's offscreen-rendering quirks with dynamic lighting. The `*_flat.png` unlit pass is the guaranteed-visible fallback.
- `snapshot.sh` requires the editor to be closed (file locks).
- `layout_top.png` is procedurally drawn from actor positions — it's accurate to actor placements but doesn't show actual geometry/materials. It's the "minimap" version.
- You still need to manually `git add claude/snapshot/` and commit if you want a snapshot tracked in history.

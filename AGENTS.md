# Lobby2 Agent Instructions

## Project

This add-on is for an Arma Reforger game mode inspired by Arma 3 and Arma 2:

- Before the match, players enter a lobby and select roles.
- Players can prepare and review a mission brief.
- The match begins after the lobby phase is complete.
- After the match, players receive an after-action report with results and statistics.

Treat the lobby, briefing, match, and after-action report as distinct phases. Keep the phase transitions explicit and easy to test.

## Game-mode isolation

This game mode must remain separate from all other game modes. The add-on may stay loaded while another game mode is selected, so its code and authored data must coexist safely with unrelated modes.

- Do not override, replace, or globally modify shared game-mode behavior unless the change is explicitly required and proven safe for every mode.
- Prefer add-on-owned names, resources, prefabs, components, and state over edits to shared assets.
- Scope initialization, event handling, UI, rules, and cleanup to this game mode and its active lifecycle.
- Do not leave persistent state, subscriptions, entities, or callbacks active after this game mode is unloaded or another mode becomes active.
- Before merging a slice, test that the mode still works when other game modes are loaded or selected, and that those modes retain their original behavior.

## Development rules

- Make every change a small, coherent, testable slice.
- Prefer the simplest implementation that completes the requested slice.
- Do not add speculative abstractions, general-purpose helpers, or extra features unless the slice requires them.
- Follow existing add-on conventions in `scripts`, `Prefabs`, and `worlds` before introducing new patterns.
- Keep server authority, client presentation, and replication responsibilities clear for multiplayer behavior.
- Do not mix unrelated cleanup or refactoring into a feature change.

## Test loop

For each implementation slice:

1. Identify the smallest owning surface: script, prefab/resource, world, UI, or a necessary combination.
2. Implement only that slice.
3. Validate with the native Workbench/compiler path when available.
4. Reload and test the affected phase or transition in Workbench.
5. Inspect relevant logs and record any manual or unavailable checks.

Do not claim a slice is complete based only on source inspection when live validation is feasible.

## Scope discipline

When a request is ambiguous, preserve the current phase model and choose the smallest reversible change that moves the requested behavior forward. Ask for direction only when the choice would materially change the game-mode design or multiplayer ownership model.

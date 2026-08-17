# Lobby2 Agent Instructions

## Architecture reference

Before changing Lobby2 lifecycle, multiplayer state, game-mode integration, controllers, groups, briefings, or menu behavior, read [`docs/Lobby2_Architecture_and_Best_Practices.md`](docs/Lobby2_Architecture_and_Best_Practices.md). It is the project design reference and records established module boundaries and lessons learned. This `AGENTS.md` remains authoritative if the documents conflict.

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

## Source of truth and base-game integration

Base-game systems are the authoritative source for faction, group, player membership, and character affiliation. Lobby2 must query and use the existing base-game functions and events even when that adds some performance overhead or requires additional traversal.

- Do not create a second faction, group-membership, or player-ownership roster in Lobby2.
- Treat Lobby2 registries as indexes, discovery lists, or replication views only; they must be derivable from the authoritative base-game state and must not override it.
- Resolve a group's faction through the base-game group APIs and resolve group membership through the base-game membership APIs when producing lobby or UI data.
- Store only lobby-specific state that the base game does not own, such as slot selection, reservation, lock state, or presentation metadata.
- Reference the authoritative group, player, character, and faction identifiers from lobby-specific records rather than copying their ownership relationships into parallel fields.
- Prefer re-querying authoritative state or subscribing to its change events over maintaining shadow state that can become stale.
- Do not modify base-game scripts to establish a competing authority. Use add-on-owned lifecycle hooks, modded integration points, or event subscriptions only to observe and coordinate with the base systems.
- When a base-game API is unavailable or insufficient, document the gap and ask for direction before introducing a new ownership source.

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

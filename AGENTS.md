# Lobby2 Agent Instructions

## Architecture reference

Before changing Lobby2 lifecycle, multiplayer state, game-mode integration, controllers, groups, briefings, or menu behavior, read [`docs/Lobby2_Architecture_and_Best_Practices.md`](docs/Lobby2_Architecture_and_Best_Practices.md). It is the project design reference and records established module boundaries and lessons learned. This `AGENTS.md` remains authoritative if the documents conflict.

## Project

This add-on is for an Arma Reforger game mode:

- Before the match, players enter a lobby and select roles.
- Players can prepare and review a mission brief.
- The match begins after the lobby phase is complete.
- After the match, players receive an after-action report with results and statistics.

Treat the lobby, briefing, match, and after-action report as distinct phases. Keep the phase transitions explicit and easy to test.

Use the `$reforger` skill for every Lobby2 or Arma Reforger question or task. Use the Reforger MCP server for game and Workbench interactions, and pass the applicable instructions and skill to subagents handling related work.

## MCP-driven modding and testing

- Use the Reforger MCP server for all Arma Reforger and Workbench interactions: opening projects/worlds, Play, runtime inspection, lobby navigation, button actions, screenshots, and validation. Do not use Computer Use, desktop input injection, or alternate automation backends. Ordinary source editing and local build/file operations may use development tools.
- Make required testing interactions scriptable through MCP. Extend or repair the owning MCP capability when needed; report unsupported states rather than substituting a different control path.
- Keep Lobby2-specific navigation helpers, scenarios, fixtures, and assertions in the optional project-owned test addon described in [tests/README.md](tests/README.md). Load it only for testing. Production Lobby2 must not depend on it or run its controls automatically.
- Use the server's generic runtime interface to invoke project test helpers. Keep the MCP server addon-agnostic, and exercise production handlers and authority checks without duplicating gameplay logic or granting test permissions.
- Verify resulting state, receipt-correlated logs, and MCP screenshots where relevant. A dispatched action alone does not establish success; never blindly replay an uncertain mutation. Record limitations such as embedded-only coverage instead of claiming multiplayer validation.

## Game-mode isolation

This game mode must remain separate from all other game modes. The add-on may stay loaded while another game mode is selected, so its code and authored data must coexist safely with unrelated modes.

- Shared `modded` classes are valid and necessary integration points. Preserve vanilla functionality when Lobby2 is absent from the active world; do not require replacing stock groups or avoiding inherited integrations. Any globally effective behavior change requires explicit justification and compatibility evidence.
- Prefer add-on-owned names, resources, prefabs, components, and state over edits to shared assets.
- Scope initialization, event handling, UI, rules, and cleanup to this game mode and its active lifecycle.
- Do not leave persistent state, subscriptions, entities, or callbacks active after this game mode is unloaded or another mode becomes active.
- Treat every shared `modded class` override, especially VON/voice, player, controller, and UI components, as affecting all game modes while the addon is loaded. Addon presence alone never authorizes Lobby2 behavior: verify that the current game mode owns an active Lobby2 instance before any Lobby2-specific side effect.
- When Lobby2 is inactive, preserve the inherited behavior and return values through the normal `super` chain where applicable. Do not suppress stock callbacks, change voice routing/audio variables, register Lobby2 listeners, or alter shared settings in unrelated modes. Guard asynchronous callbacks and RPC entry points too; a cached instance or an earlier check is not proof of current ownership.
- Track and release Lobby2-owned changes to shared state when leaving the mode. Cleanup must neither leak into the next mode nor overwrite changes owned by other systems.
- Validate shared overrides through MCP with the addon loaded in an unrelated game mode, comparing affected behavior against a baseline without the addon, and after leaving Lobby2. For VON, include normal voice transmission/reception and routing. Source guards or successful compilation alone do not establish coexistence; record missing runtime coverage explicitly.

## Source of truth and base-game integration

Base-game systems are the authoritative source for faction, group, player membership, and character affiliation. Lobby2 must query and use the existing base-game functions and events even when that adds some performance overhead or requires additional traversal.
- Store only lobby-specific state that the base game does not own, such as slot selection, reservation, lock state, or presentation metadata.
- Reference the authoritative group, player, character, and faction identifiers from lobby-specific records rather than copying their ownership relationships into parallel fields.
- Prefer re-querying authoritative state or subscribing to its change events over maintaining shadow state that can become stale.
- When a base-game API is unavailable or insufficient, document the gap and ask for direction before introducing a new ownership source.
- Keep server authority, client presentation, and replication responsibilities clear for multiplayer behavior.

## Admin actions and logging

- Every admin-action entry point must verify, at execution time, that the requesting player has an active authenticated admin login. UI visibility, client claims, or an earlier check are not authorization.
- Admin actions must be server-authoritative whenever the engine and action permit it. Validate authorization on the server before changing state; clients may request an action but must not decide whether it is authorized.
- Reject an admin action without applying any state change when the requester cannot be resolved or is not currently logged in as an admin.
- Route every admin action through one add-on-owned central logging function. Do not scatter direct admin-action `Print` calls or create per-action logging paths.
- The central logging function must currently emit the admin action through the engine's print logging and remain the single extension point for future logging destinations or audit behavior.
- Log the requester, action, relevant target or context, and outcome, including authorization rejection, without recording secrets or credentials.
- Tests for admin actions must cover an authenticated admin, an unauthenticated or logged-out requester, server-side rejection without mutation, and emission through the central logging function.

## Development rules

- Do not add speculative abstractions, general-purpose helpers, or extra features unless the slice requires them.
- Carefully and comprehensively plan features before implementation; avoid over-engineering, premature abstraction, and scope creep.
- Follow YAGNI. NEVER speculate new features, aim to create a small change rather than re-architecturing unless there's a articulable reason for why it would be necessary. Extraordinary changes require extensive justification.
- If an existing system is low quality or conflicts with this philosophy, remove it and redesign the feature properly from first principles.
- Choose the best long-term design, not the easiest short-term solution. When unsure research and compare the top paths forward.
- Each feature must have one canonical system only, with all related behavior flowing through it—no fallbacks, secondary implementations, duplicate logic, compatibility layers, workarounds, or parallel paths.
- Move duplicated behavior to a shared package only when it represents the same responsibility and must evolve together.
- Do not refactor for stylistic reasons, do not create one line functions for trivial operations that will only be used once, prefer including such operations in the main functions instead.
- Test meaningful behavior, contracts, regressions, edge cases, and non-trivial transformations. 
  Do not add tests to verify a generated's file change, test the code itself.

## Scope discipline

When a request is ambiguous, preserve the current phase model and choose the smallest reversible change that moves the requested behavior forward. Ask for direction only when the choice would materially change the game-mode design or multiplayer ownership model.

## Git commit policy

After completing a task, commit and push the task's changes. The commit subject must contain no more than five words, and the commit description/body must be blank.

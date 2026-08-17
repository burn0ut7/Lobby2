# Lobby2 Architecture and Best Practices

## Purpose

This document is the durable design reference for Lobby2. It records the architecture, ownership boundaries, implementation practices, and lessons established while separating the lobby game mode from global base-game behavior.

Read this document before changing game-mode lifecycle, phase behavior, replication, player controllers, groups, lobby registries, briefing systems, or menu navigation. `AGENTS.md` remains authoritative if the two documents ever conflict.

## Product model

Lobby2 implements four ordered mission phases:

`PREGAME → BRIEFING → GAME → POSTGAME`

- `PREGAME` covers mission preview and role selection.
- `BRIEFING` covers the authoritative mission briefing phase.
- `GAME` is the active match.
- `POSTGAME` is the completed match and future after-action-report phase.
- `LOBBY` is a local menu view, not a fifth mission phase.
- A phase may advance only to its explicit successor and may never move backward.

Lobby2 owns this sequence through `GC_ELobbyPhase`. Do not extend or reinterpret `SCR_EGameModeState` to represent Lobby2-only phases.

### Base-game state mapping

| Lobby2 phase | Base `SCR_BaseGameMode` state | Transition behavior |
|---|---|---|
| `PREGAME` | `PREGAME` | Lobby2 prevents automatic base-game start. |
| `BRIEFING` | `PREGAME` | Lobby2 changes only its replicated phase. |
| `GAME` | `GAME` | Authority calls native `StartGameMode()` and verifies success. |
| `POSTGAME` | `POSTGAME` | Authority calls native `EndGameMode()` and verifies success. |

The base game remains authoritative for its own match lifecycle. Lobby2 coordinates with it instead of maintaining a competing replacement.

## Architectural principles

### 1. Opt in instead of modifying globally

Lobby2 must be safe while another game mode is selected.

- Use `GC_LobbyGameMode` and `GC_LobbyPlayerController` only in Lobby2-owned resources and worlds.
- The one shared integration exception is `modded SCR_AIGroup` in `scripts/Game/modded/Entities/SCR_AIGroup_M.c`. It calls the base implementation and immediately exits unless `GC_LobbyManager.GetManager()` can cast the active mode to `GC_LobbyGameMode`.
- Do not use an unguarded `modded class` to change `SCR_BaseGameMode`, `SCR_PlayerController`, `SCR_AIGroup`, or another shared runtime class.
- A harmless enum registration such as Lobby2 menu preset names may extend a shared enum, but it must not alter existing values or behavior.
- `GC_LobbyManager.GetManager()` returning `null` is a normal condition outside Lobby2. Callers must tolerate it.
- Never let Lobby2 callbacks, static subscriptions, UI, or replicated requests remain active after its owning mode is gone.

### 2. Preserve base-game authority

The base game owns players, factions, groups, group membership, characters, and possession.

Lobby2 may keep derived indexes for discovery and presentation, but it must re-query authoritative APIs when answering ownership or membership questions. Lobby2 owns only data that the base game does not provide, including:

- Lobby slot selection and slot presentation metadata.
- The Lobby2 phase.
- Briefing content and marker snapshots.
- Lobby-specific voice-channel presentation state.
- Local menu view selection.

Do not turn `GC_LobbyRoster` into a competing player, faction, or group authority.

### 3. Keep server authority and presentation separate

- Clients request intent; they do not publish authoritative state.
- An owning `GC_LobbyPlayerController` RPC derives the requesting player ID from the controller.
- `GC_LobbyManager` validates authority, lifecycle, administration status, current phase, and exact successor.
- Clients never update phase optimistically.
- Reliable authority broadcasts and JIP snapshots update local phase state.
- UI observes replicated state and derives interaction permissions from it.

### 4. Prefer deep modules

A module should expose a small, meaningful interface while hiding the data and mechanics required to implement it.

- Keep policy and validation with the authority coordinator.
- Keep domain collections and their invariants in focused domain objects.
- Keep parsing free of widget creation and side effects.
- Keep rendering free of server authority decisions.
- Keep action execution and map animation lifecycle out of the main briefing view.
- Split a file when it begins mixing unrelated reasons to change, not merely because it crosses an arbitrary line count.

Small related value types may share a file. Large classes should not become folders of speculative one-method abstractions.

## Module map

### Game-mode and phase ownership

- `scripts/Game/GameMode/GC_LobbyGameMode.c`
  - Lobby2-only game-mode root.
  - `GetGameMode()` is the single guarded lookup for the active Lobby2 mode.
  - Prevents automatic base-game start while Lobby2 is in pregame or briefing.
  - Opens the initial local lobby view with bounded controller-readiness retries.
  - Removes pending callbacks when destroyed.
- `scripts/Game/GameMode/GC_LobbyPhase.c`
  - Defines `GC_ELobbyPhase`.
  - Encodes legal successors, validity, names, and explicit ordering.
  - Rejects duplicate and backward applications, including stale proxy updates.
- `scripts/Game/GC_LobbyManager.c`
  - Server-authoritative façade and integration coordinator.
  - `GetManager()` resolves the manager only through `GC_LobbyGameMode.GetGameMode()`.
  - Owns RPC publication, request validation, base-game transition calls, lifecycle integration, and UI invokers.
  - Delegates domain storage to focused modules.

### Domain state

- `scripts/Game/Lobby/GC_LobbyRoster.c`
  - Derived indexes of base-game factions and groups.
  - Lobby-only slots and pending JIP references.
  - Queries authoritative group membership before slot fallback.
- `scripts/Game/Voice/GC_LobbyVoiceState.c`
  - Lobby voice presentation assignments and serialization.
  - Does not replace the base voice/audio system.
- `scripts/Game/Briefing/GC_LobbyBriefingCatalog.c`
  - Authored briefings, runtime marker snapshots, revisions, filtering, and serialization.
  - Keeps JIP snapshots independent of entity streaming order.
- `GC_LobbyPhaseMachine`
  - Stores the current phase and enforces monotonic application.

### Opt-in engine integration

- `scripts/Game/Player/GC_LobbyPlayerController.c`
  - Owns Lobby2 request RPCs.
  - Is selected only by the Lobby2 player-controller prefab.
- `scripts/Game/modded/Entities/SCR_AIGroup_M.c`
  - Calls `super` for every base group lifecycle event.
  - Registers playable groups and suppresses playable-group unit spawning only while the active mode is `GC_LobbyGameMode`.
  - Unrelated game modes retain the original `SCR_AIGroup` behavior because the manager cast returns `null`.
- `scripts/Game/Briefing/GC_BriefingMarkerComponent.c`
  - Registers authored markers with bounded retries.
  - Cancels retries and unregisters during deletion.

### Briefing UI

- `GC_BriefingViewComponent`
  - Coordinates selection, widget discovery, subscriptions, view sizing, and collaborator lifecycle.
- `GC_BriefingParser`
  - Pure allowlist and custom-tag parsing.
- `GC_BriefingRenderer`
  - Scenario and block-image widget creation, sizing, measurement, and cleanup.
- `GC_BriefingInteractiveRenderer`
  - Inline text, decoration, image, and button rendering plus button subscription cleanup.
- `GC_BriefingActionHandler`
  - Validates external URLs and dispatches supported briefing actions.
- `GC_BriefingMapFocusController`
  - Owns asynchronous map pan/zoom state and event cleanup.

## Resource ownership

### Production resources

`Prefabs/MP/Modes/GameMode_Lobby.et` is the production game-mode prefab. It should contain only production defaults and required Lobby2 components.

Production resources must not contain:

- Demo briefing content.
- Test-only bootstrap components.
- Debug fixtures required only by example worlds.
- Overrides that affect unrelated game modes.

### Example resources

`Prefabs/MP/Modes/GameMode_Lobby_Example.et` derives from the production prefab and owns sample briefing content. Example worlds reference this fixture explicitly.

Playable group resources remain ordinary base `SCR_AIGroup` prefabs in every world. The guarded `SCR_AIGroup` extension supplies Lobby2 registration when `GC_LobbyGameMode` is active, so authored group resources do not need a Lobby2-specific subclass.

When adding test content:

1. Put reusable production behavior in the production prefab or its owned scripts.
2. Put demonstrations and test data in the example prefab or example world.
3. Do not make production startup depend on a test component.
4. Keep prefab metadata names synchronized with their actual resource paths.
5. When changing a prefab root class, update explicit root-class declarations in every world layer that instantiates it.

## Phase-transition contract

### Client request path

1. The header determines whether a control is navigational, selected, locked, or the immediate admin-only successor.
2. An admin click calls `GC_LobbyPlayerController.RequestLobbyPhaseAdvance(target)`.
3. The owning-controller RPC sends only the target phase.
4. The server derives the player ID from that controller.
5. `GC_LobbyManager.TryAdvanceLobbyPhase` rejects the request unless:
   - The manager is active and not shutting down.
   - The handler is running on authority.
   - The player ID is valid.
   - The target enum is valid.
   - The player is currently an admin.
   - The target is exactly the next phase.
6. The authority performs the corresponding native transition when required.
7. The authority publishes the accepted Lobby2 phase reliably.
8. UI reacts only after the authority update arrives.

### Mission completion path

Trusted mission logic and the admin control must both use `GC_LobbyManager.EnterPostGame(endData)`. Do not create a second end-state variable or bypass this API with a separate Lobby2 phase mutation.

### Rejection behavior

Backward, skipped, duplicate, stale, non-admin, invalid-enum, non-authority, and shutdown-time requests must leave both phase and UI unchanged. Rejections should produce concise `GC | Phase |` diagnostics.

## UI state rules

Mission phase and local menu view are separate concepts.

| Mission phase | Normal navigation | Admin-only successor | Locked |
|---|---|---|---|
| `PREGAME` | Preview, Lobby | Briefing | Game, Debrief |
| `BRIEFING` | Lobby, Briefing | Game | Preview, Debrief |
| `GAME` | None | Debrief | Earlier views and current Game |
| `POSTGAME` | None | None | All controls |

- Selected controls do not hover or accept clicks.
- Locked controls do not hover, focus-highlight, click, or play sounds.
- Non-admin players still see the immediate successor but cannot interact with it.
- Administration status may change during a session, so permission presentation must refresh and clean up its polling callback on detach.
- Entering `GAME` closes lobby UI.
- `POSTGAME` currently closes the UI without opening a debrief screen.
- Every widget component must unsubscribe invokers and remove queued callbacks in `HandlerDeattached`.

## Replication and JIP

Keep save and load order identical. The current snapshot order is:

1. Lobby2 phase.
2. Registered group replication IDs.
3. Lobby slots.
4. Voice presentation state.
5. Briefing and marker catalog.

Guidelines:

- Use bounded enum ranges that include every `GC_ELobbyPhase` value.
- Treat entity replication IDs as references that may not resolve during the first JIP load pass.
- Retry cross-entity resolution through a bounded or lifecycle-owned callback.
- Remove retry callbacks during shutdown.
- Do not serialize copied base-game ownership relationships.
- Reliable updates must be idempotent; stale phase updates must not revert local state.
- When changing snapshot layout, update save and load together and retest every JIP phase.

## Lifecycle and cleanup

Every subscription or queued callback creates an ownership obligation.

- `GC_LobbyManager.Shutdown()` is idempotent and is called for game-mode and world termination.
- Manager shutdown removes call-queue entries, unsubscribes group and slot events, and clears Lobby2-owned domain state.
- Entity components clean up in `OnDelete`.
- Widget components clean up in `HandlerDeattached`.
- Game-mode-owned startup callbacks are removed by the Lobby2 game-mode destructor.
- Renderer helpers provide explicit `Clear`/`Detach` behavior because dynamically created widgets and button invokers outlive a simple local variable assignment.
- Static base-game invokers require particular care: subscribe only while at least one owned Lobby2 consumer exists, and always remove the subscription when the last consumer leaves.

Cleanup code should be safe when called twice and safe when initialization completed only partially.

## Logging convention

Use structured prefixes so multiplayer logs can be filtered quickly:

- `GC | Phase |` for accepted state changes and validation failures.
- `GC | Phase RPC |` for request and broadcast boundaries.
- `GC | Phase JIP |` for snapshot phase serialization.
- `GC | Lifecycle |` for startup and teardown.
- `GC | JIP` for topology/reference resolution.

Log the role, previous phase, target phase, base-game state, and requesting player where relevant. Avoid per-frame or repeated retry spam.

## Lessons learned

### Global modifications are deceptively convenient

A global `modded class SCR_PlayerController` or an unguarded `SCR_AIGroup` extension makes the first implementation easy but silently changes every mode loaded with the add-on. The guarded `SCR_AIGroup` integration is an intentional exception: it calls `super`, requires the active `GC_LobbyGameMode`, and becomes inert for unrelated modes.

### A blocker does not need to be a separate component

Lobby2's automatic-start policy belongs on `GC_LobbyGameMode`. Native manual `StartGameMode()` remains available because base `CanStartGameMode()` governs automatic pregame advancement, not explicit authority calls.

### Added base enum members create hidden serialization bugs

Extending a shared phase enum caused ordering and range assumptions to leak into unrelated systems. An add-on-owned enum makes the complete phase model explicit and allows independent serialization.

### Prefab and world declarations must migrate together

Changing a prefab root class is incomplete if world-layer instances still declare the old root type. Likewise, moving an attributed field into a nested domain object requires migrating world-level overrides.

### Script defaults can overwrite authored values

Do not initialize an attributed object field in script when the prefab authors that object. Author a production default object in the prefab, then override it in derived example resources.

### JIP is a data-order problem as well as a networking problem

Replicated entity references can arrive before their proxies. Snapshot deserialization must preserve unresolved IDs and retry without inventing alternate ownership data.

### Large UI components usually contain multiple lifecycles

The old briefing view mixed parsing, widget construction, browser actions, button subscriptions, and asynchronous map animation. Splitting by lifecycle and side effect made cleanup and testing clearer than splitting by arbitrary method count.

### A successful compile is necessary but not sufficient

Script validation does not prove that prefab metadata, world-layer types, hover behavior, JIP timing, or dedicated-server authority are correct. Resource inspection, reload logs, and multiplayer role testing remain separate gates.

## Change checklist

Before implementing a slice:

- Identify the authoritative owner of every changed datum.
- Confirm whether the behavior is Lobby2-only or truly safe globally.
- Trace prefab ancestry and world instances for changed root classes or attributed fields.
- Identify every subscription, callback, dynamic widget, and replicated reference that needs cleanup.
- Define the server request, validation, publication, and JIP paths.
- Keep local view state separate from mission state.

Before calling a slice complete:

- Run fresh native Workbench script validation.
- Resolve every Lobby2 compiler diagnostic.
- Run `git diff --check`.
- Inspect production and example resources separately.
- Reload scripts and inspect fresh logs.
- Test the affected flow in a Lobby2 world.
- Test an unrelated game mode with Lobby2 still loaded.
- For multiplayer changes, test dedicated server, listen host, owning/admin client, non-admin client, concurrent requests, and JIP where the environment permits.
- Record any test that remains manual or unavailable; do not silently treat source inspection as runtime proof.

## Extending the architecture

New behavior should generally fit one of these locations:

- New authoritative transition policy: `GC_LobbyManager` plus the phase machine if ordering changes.
- New Lobby2-owned phase data: a focused domain object serialized through the manager snapshot.
- New player-originated request: `GC_LobbyPlayerController` RPC with server-side manager validation.
- New group behavior: extend the guarded `SCR_AIGroup` integration only when the behavior is required for every base group inside `GC_LobbyGameMode`; otherwise use an add-on-owned component on a narrower Lobby2 resource.
- New briefing syntax: parser first, renderer second, action handler only if it creates a side effect.
- New menu view: local view state in the header/menu layer; add a mission phase only when it changes authoritative mission progression.
- New example content: the example prefab or example world, never the production prefab.

If a feature does not fit these boundaries cleanly, pause and revisit ownership before adding another cross-cutting manager responsibility.

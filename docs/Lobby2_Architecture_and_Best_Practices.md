# Lobby2 Architecture and Best Practices

## Purpose

This document is the durable design reference for Lobby2. It records the architecture, ownership boundaries, implementation practices, and lessons established while separating the lobby game mode from global base-game behavior.

Read this document before changing game-mode lifecycle, phase behavior, replication, player controllers, groups, lobby registries, briefing systems, or menu navigation. `AGENTS.md` remains authoritative if the two documents ever conflict.

## Product model

Lobby2 implements five ordered mission phases:

`PREGAME → SLOTTING → BRIEFING → GAME → POSTGAME`

- `PREGAME` is the mission preview phase.
- `SLOTTING` is the lobby and role-selection phase.
- `BRIEFING` covers the authoritative mission briefing phase.
- `GAME` is the active match.
- `POSTGAME` is the completed match and future after-action-report phase.
- A phase may advance only to its explicit successor and may never move backward.

The active `GC_LobbyGameMode` owns this sequence through the base
`SCR_EGameModeState` domain. Lobby2 adds `SLOTTING` and `BRIEFING` with a
`modded enum`; it does not maintain a parallel phase value.

### Base-game state mapping

| Game-mode state | Transition behavior |
|---|---|
| `PREGAME` | Native initial state; Lobby2 prevents automatic start. |
| `SLOTTING` | Guarded Lobby2 transition updates the base game-mode state. |
| `BRIEFING` | Guarded Lobby2 transition updates the base game-mode state. |
| `GAME` | Authority calls native `StartGameMode()` and verifies success. |
| `POSTGAME` | Authority calls native `EndGameMode()` and verifies success. |

The base game-mode state property remains the single automatically replicated
authority used by Lobby2 and other game-mode systems.

## Architectural principles

### 1. Opt in instead of modifying globally

Lobby2 must be safe while another game mode is selected.

- Use `GC_LobbyGameMode` and `GC_LobbyPlayerController` only in Lobby2-owned resources and worlds.
- The one shared integration exception is `modded SCR_AIGroup` in `scripts/Game/modded/Entities/SCR_AIGroup_M.c`. Playable spawn suppression is guarded directly by the active `GC_LobbyGameMode` so AI-world startup cannot race manager-component discovery; registration and lobby-owned lifecycle work still require `GC_LobbyManager.GetManager()`.
- Do not use an unguarded `modded class` to change `SCR_BaseGameMode`, `SCR_PlayerController`, `SCR_AIGroup`, or another shared runtime class.
- Harmless enum registration may extend a shared enum with explicit unique values, but must not alter existing values or unrelated behavior.
- `GC_LobbyManager.GetManager()` returning `null` is a normal condition outside Lobby2. Callers must tolerate it.
- Never let Lobby2 callbacks, static subscriptions, UI, or replicated requests remain active after its owning mode is gone.

### 2. Preserve base-game authority

The base game owns players, factions, groups, group membership, characters, and possession.

Lobby2 may keep derived indexes for discovery and presentation, but it must re-query authoritative APIs when answering ownership or membership questions. Lobby2 owns only data that the base game does not provide, including:

- Lobby slot selection and slot presentation metadata.
- The two add-on-defined members of the base game-mode state domain.
- Briefing content and marker snapshots.
- Lobby-specific voice-channel presentation state.
- Local menu view selection.

Do not turn `GC_LobbyRoster` into a competing player, faction, or group authority.

### 3. Keep server authority and presentation separate

- Clients request intent; they do not publish authoritative state.
- An owning `GC_LobbyPlayerController` RPC derives the requesting player ID from the controller.
- `GC_LobbyManager` validates authority, lifecycle, administration status, current phase, and exact successor.
- Clients never update phase optimistically.
- `SCR_BaseGameMode`'s automatically replicated state is the only phase transport for live transitions and JIP.
- UI observes `GC_LobbyGameMode.GetState()` and derives presentation and interaction permissions from it.

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
  - Prevents automatic base-game start throughout Lobby2's preview, slotting, and briefing phases.
  - Exposes one phase-presentation function that reads the replicated base state; both player-controller initialization and later state changes call it directly.
  - Does not poll, retry, or gate the initial menu on a separate registration/readiness flag.
- `scripts/Game/GameMode/GC_LobbyPhase.c`
  - Adds `SLOTTING` and `BRIEFING` to `SCR_EGameModeState` with explicit unique values.
  - Encodes Lobby2's legal successors, validity, names, and explicit ordering without storing state.
- `scripts/Game/GC_LobbyManager.c`
  - Server-authoritative façade and integration coordinator.
  - `GetManager()` resolves the manager only through `GC_LobbyGameMode.GetGameMode()`.
  - Owns RPC publication, request validation, base-game transition calls, slot/player lifecycle reconciliation, and UI invokers.
  - Loads the configured AI-world formation resource once on authority and retains only Lobby2-owned name/offset data.
  - Captures each playable group's authored world transform at registration; runtime group recentering from player membership never changes the slot-spawn anchor.
  - Honors the base group's authored terrain-snap policy when converting formation positions into final member positions.
  - Delegates domain storage to focused modules.

### Domain state

- `scripts/Game/Lobby/GC_LobbyRoster.c`
  - Derived indexes of base-game factions and groups.
  - Lobby-only slots, UUID/entity lookup helpers, and pending JIP references.
  - Queries authoritative group membership before slot fallback.
- `scripts/Game/Lobby/GC_PlayerFormationCatalog.c`
  - Reads formation names and offset vectors once from the configured AI-world resource.
  - Retains no `Resource`, `BaseContainer`, or native AI formation object after extraction.
  - Provides server-local spawn geometry and is not replicated or serialized.
- `scripts/Game/GameMode/GC_LobbyReconnectComponent.c`
  - Lobby2-owned reconnect policy installed only on `GameMode_Lobby.et`.
  - Preserves UUID, slot entity, and cached claimant data for the configured reservation timeout.
  - Delegates expiry cleanup and phase catch-up to `GC_LobbyManager`.
- `scripts/Game/Voice/GC_LobbyVoiceState.c`
  - Lobby voice presentation assignments and serialization.
  - Does not replace the base voice/audio system.
- `scripts/Game/Briefing/GC_LobbyBriefingCatalog.c`
  - Authored briefings, runtime marker snapshots, revisions, filtering, and serialization.
  - Keeps JIP snapshots independent of entity streaming order.
- `GC_LobbyPhase`
  - Validates the five supported base game-mode states and their explicit order.
- `scripts/Game/modded/GameMode/SCR_BaseGameMode_M.c`
  - Adds one guarded setter for `SLOTTING` and `BRIEFING`.
  - Rejects unrelated game modes and cannot replace native `GAME` or `POSTGAME` transitions.

### Opt-in engine integration

- `scripts/Game/Player/GC_LobbyPlayerController.c`
  - Owns Lobby2 request RPCs.
  - Is selected only by the Lobby2 player-controller prefab.
  - Applies the current phase menu directly from `OnInit` on the owning local player controller, including a headed listen server. Dedicated servers never create or switch Lobby2 menus, and remote player-controller proxies cannot alter another client's local view.
  - Receives reliable owner-only spectator enter/leave RPCs. The owning client alone creates and deletes the local manual camera; the server never spawns or replicates that camera.
- `scripts/Game/Camera/GC_SpectatorManualCamera.c`
  - Extends the base manual camera and owns the active client-local spectator camera mode and target through the Lobby2 spectator camera prefab.
  - Leaves native manual-camera components responsible for free movement, rotation, and zoom. Shoulder mode uses the animated `Camera` bone as its positional anchor, the animated `Head` bone as its unadjusted rotation, and prefab-authored lateral, backward, and vertical offsets. Eye mode uses the animated `leftEye` bone for position and the camera-aligned animated `Head` basis for rotation. Mirror mode remains client-local and observes the presentation replicated on the target proxy without sending input or changing possession: outside ADS it selects the stock `Camera`/`Head` direct bone and obtains its position through the stock first-person `GetLookTransformationLS`, character-pitch, and `0, 0.03, -0.07` offset path. During ADS it obtains position and zeroing from the current base weapon sight camera. In every mirror state, viewing direction comes from the animated `Head` basis so the proxy's presented freelook remains visible, but model-space position and look direction are lightly stabilized and animated roll is removed before composition with the unsmoothed character world transform. This reduces third-person proxy animation bob without making the camera trail behind player movement. When an already-observed player changes ADS state, mirror blends its last rendered position and FOV to the new camera over the base character controller's authored ADS time while continuing to follow head rotation live; selecting a player who is already ADS snaps immediately because there is no prior presentation to blend. Mirror directly applies the unmodified local first-person FOV outside ADS and the stock first-person near plane, uses the active PiP sight's main-camera FOV or clamps ordinary ADS to the sight's authored FOV, applies the local player's ADS-focus setting through `PlayerCamera` only while ADS is active, explicitly applies the result after the inherited manual-camera component stack, and restores the prior manual-camera lens values on exit. Because the base PiP optic activation rejects weapons not owned by the local controlled character, mirror mode directly enables only the observed PiP sight's client-local render target and overlay camera for its ADS lifetime, then disables both on ADS exit, sight or target switch, and spectator teardown. Eye and mirror modes hide the target's identity head and head-cover cloth through the stock camera-handler alpha-test path, then restore them on every exit or target-switch path. Shoulder and eye modes do not interpolate; manual movement or look input returns attached modes to free camera.
  - The separate network first-person prototype samples the target owner's active camera at the player-controller prefab's configurable rate (default 30 Hz). Reliable owner/server RPCs establish one transient spectator subscription, then unreliable camera samples travel target owner to server to that spectator controller's owner only; no broadcast or replicated property publishes them to unrelated players. Every sample uses one canonical transform space for ADS and non-ADS: the owner sends the camera's world-aligned position offset from the controlled-character origin plus its world-aligned basis. The spectator's client-local manual camera is parented beneath the stock translation-only attachment helper, which follows the observed proxy's origin every post-frame without inheriting proxy rotation, and interpolates the streamed offset and basis between samples. Proxy translation therefore remains continuous while independently timed turn-in-place or root-motion rotation cannot be applied a second time; there is no alternate ADS transform or fallback. Each sample also carries the main-camera FOV, owner-local ADS state, selected weapon-sight index, variable-magnification index, live PiP-camera FOV, and the selected sight's actual owner-side FOV. The spectator applies the selected iron sight or optic and its zoom only to the local target proxy presentation, interpolates all three FOV values, and restores the proxy's prior sight/zoom values and the manual camera's prior FOV on exit. Proxy ADS animation and sight geometry are cosmetic only: the PiP render camera copies the interpolated network camera transform and uses the owner's final PiP-camera and sight FOV values for magnification, objective-edge, and reticle scale, rather than consuming divergent local-proxy sight state. Vignette, edge, and parallax centers remain nominally aligned instead of reconstructing the remote ocular geometry. Leaving the mode, changing targets, or destroying the spectator camera removes the subscription and deletes the attachment helper while preserving the camera's world transform.
  - While a network-camera subscription is active, bounded diagnostics log matching owner and spectator records on ADS/sight transitions and every 30 ADS samples. Sequence-keyed records compare camera, character root, aiming component, selected sight transform, main FOV, PiP FOV, and the transported owner sight FOV in camera-local coordinates without changing any presentation transform.
  - Registers no replicated property or global camera override. Target eligibility is re-queried from base player and character components, and invalid or streamed-out targets return to free camera.
- `scripts/Game/modded/Entities/SCR_AIGroup_M.c`
  - Calls `super` for every base group lifecycle event.
  - Purges any AI-world request queued by native initialization, registers playable groups through the lobby manager, and suppresses later queued `RequestSpawn()` and direct `SpawnUnits()` calls whenever the active mode is `GC_LobbyGameMode`.
  - Preserves empty playable group entities for Lobby2 respawn and slot reuse regardless of their authored native delete-when-empty setting, while retaining native empty-event and dormant behavior.
  - In Workbench edit mode, materializes transient previews for every playable group from the cached formation data, including Lobby2 worlds. Runtime Lobby2 playable spawning remains suppressed outside edit mode.
  - Unrelated runtime game modes retain the original `SCR_AIGroup` behavior because the manager cast returns `null`.
  - The editor-only playable preview is an explicit guarded Workbench exception and never changes runtime behavior.
- `scripts/Game/modded/Groups/SCR_GroupsManagerComponent_M.c`
  - Preserves empty authored playable groups while Lobby2 is active, even when their native delete-if-no-player setting is enabled.
  - Routes the exception through the native empty-group cleanup branch and restores the authored setting afterward.
  - Calls native behavior unchanged for non-playable groups and every group outside Lobby2.
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

### Shared context-menu UI

- `GC_ContextMenuContext`
  - Holds one temporary local target reference for a faction, group, slot, player, controlled entity, or voice channel.
  - Re-queries base-game player and entity state when deriving presentation labels; it is never replicated or retained as ownership state.
- `GC_ContextMenuActionRegistry`
  - Provides the shared action catalogue for every Lobby2 screen and spectator presentation.
  - Registered action scripts decide visibility and executability from the target plus requesting player and must be unregistered by their owning component when detached.
- `GC_ContextMenuComponent`
  - Creates one top-level popup at the cursor, clamps it to the viewport, and owns input-listener cleanup and dismissal.
  - Presentation is shared between the two-dimensional lobby/briefing screens and spectator rather than implemented as separate context systems.
  - Context actions may initiate intent, but authoritative game-state mutations must still use the owning player-controller RPC and manager validation path.

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
7. The authority changes the base game-mode state and bumps its existing replicated property.
8. `SCR_BaseGameMode.OnGameStateChanged` updates components and local UI on authority, proxies, and JIP.

### Mission completion path

Trusted mission logic and the admin control must both use `GC_LobbyManager.EnterPostGame(endData)`. Do not create a second end-state variable or bypass this API with a separate Lobby2 phase mutation.

### Rejection behavior

Backward, skipped, duplicate, stale, non-admin, invalid-enum, non-authority, and shutdown-time requests must leave both phase and UI unchanged. Rejections should produce concise `GC | Phase |` diagnostics.

## UI state rules

The replicated phase determines progression and the default presentation. Once
reached, Slotting and Briefing remain local navigable views; opening an earlier
view never requests or performs a backward phase transition.

| Mission phase | Presented view | Admin-only successor | Locked |
|---|---|---|---|
| `PREGAME` | Preview | Slotting | Briefing, Game, Debrief |
| `SLOTTING` | Lobby/Slotting | Briefing | Preview, Game, Debrief |
| `BRIEFING` | Briefing for assigned players; spectator presentation for unassigned players | Game | Preview, Debrief |
| `GAME` | None; Slotting and Briefing remain available from the pause menu | Debrief | Preview |
| `POSTGAME` | None; reached Slotting and Briefing views remain available from the pause menu | None | Preview and phase controls |

- Selected controls do not hover or accept clicks.
- Locked controls do not hover, focus-highlight, click, or play sounds.
- Non-admin players still see the immediate successor but cannot interact with it.
- Administration status may change during a session. Never cache it as UI authority: every admin-gated presentation refresh and interaction queries the base player manager through `SCR_Global.IsAdmin`, and server authority independently re-checks every mutation. The authority game mode publishes role changes through one reliable Lobby2 manager broadcast so dedicated clients receive the prompt even when the base callback is not raised on their local proxy. Transported role state is used only as a bounded replication-convergence hint; presentation and permissions never consume it as authority.
- Every Lobby2 widget that presents a player-derived name uses the shared player-name presentation rule: current administrators are yellow and all other player names use the normal white. Player, voice, slot, spectator, and context presentations consume the manager's converged player-role event, then query `SCR_Global.IsAdmin(playerId)` live; they do not poll or store parallel role state.
- Entering `GAME` closes lobby UI. The Lobby2 player controller adds scoped
  Slotting/Briefing access to the active pause menu; no shared pause-menu class
  or layout is modified for unrelated modes.
- Briefing requires an authoritative player faction derived from the player's
  current base-game group/slot. A player without one remains a spectator and is
  presented the spectator camera, HUD, and marker overlay when `BRIEFING`
  begins. Slotting remains available from the pause menu.
- `POSTGAME` currently closes the UI without opening a debrief screen.
- Every widget component must unsubscribe invokers and remove queued callbacks in `HandlerDeattached`.

### UI event domains

`GC_LobbyManager` exposes narrow presentation notifications for registry reset, player connection, player affiliation, faction/group topology, faction presentation, slot presentation, voice assignment, voice topology, and player role. A normal mutation emits its owning domain event once. Consumers re-query authoritative base-game state and update the identified widget; they do not treat an event payload or presentation map as ownership state.

Full collection reconstruction is limited to initial attachment and a completed JIP/registry reset. Topology events reconcile keyed widgets while preserving unchanged rows and cards. Voice or deafen changes never refresh briefings, briefing markers, faction lists, or unrelated player rows. Player-name role changes update only presentations for the identified player.

Continuous presentation remains lifecycle-owned where no discrete state event can describe the change: spectator world-to-screen projection and distance animation update per frame, spectator draw-distance activation is checked periodically, and the scenario clock refreshes only its time-dependent rows. Zero-delay layout settling and bounded manager/entity attachment retries are lifecycle sequencing rather than steady-state polling. All recurring callbacks are removed when their owner detaches or stops.

## Replication and JIP

The base `SCR_BaseGameMode` state is the phase authority and is not duplicated
in Lobby2's custom snapshot. Keep the remaining save and load order identical:

1. Registered group replication IDs.
2. Lobby slots.
3. Voice presentation state.
4. Briefing and marker catalog.
5. Scenario parameters.

Guidelines:

- Modded game-mode enum values are intentionally non-contiguous. Serialize
  Lobby2 phase-bearing records as integers, validate them through
  `GC_LobbyPhase.IsValid`, and never infer lifecycle order from their values.
- Treat entity replication IDs as references that may not resolve during the first JIP load pass.
- Retry cross-entity resolution through a bounded or lifecycle-owned callback.
- Remove retry callbacks during shutdown.
- Do not serialize copied base-game ownership relationships.
- Reliable updates must be idempotent; stale phase updates must not revert local state.
- When changing snapshot layout, update save and load together and retest every JIP phase.

## Slot lifecycle and phase reconciliation

`GC_LobbyManager` is the lifecycle coordinator. `GC_Slot` owns slot transitions and is the only API that changes claimant identity: `SetPlayer`, `MarkDisconnected`, `ReconnectPlayer`, `ReleaseClaim`, `SetEntity`, and `MarkKIA`. The roster lookup helpers are derived discovery indexes; base-game group and player APIs remain authoritative.

Slot states have these meanings:

- `FREE`: claimable; an existing living entity may remain dormant.
- `RESERVED`: claimed but not currently spawned.
- `LOCKED`: claimed with a living spawned entity.
- `DISCONNECTED`: a non-KIA UUID reservation awaiting reconnect.
- `KIA`: the character died during `GAME` and is unavailable for the match.

Player catch-up is idempotent and runs after identity registration, reconnect, and relevant phase notifications:

- A connected player's live claim is resolved by its authoritative session player ID before consulting UUID reservations.
- No live claim or UUID reservation: global voice and spectator assignment.
- `PREGAME`: restore slot/group claim, remain spectator.
- `SLOTTING`: restore slot/group claim, remain spectator, and allow authoritative slot changes.
- `BRIEFING`: stage the exact slot prefab in bounded batches, keep the player spectator, and deactivate staged AI.
- `GAME`: force-spawn a missing living slot when possible and assign its entity through `SetInitialMainEntity`; failed assignments remain spectators.
- `KIA` or `POSTGAME`: spectator behavior without altering match results.

The Lobby2 spectator body and spectator camera have separate lifetimes. The server spawns and possesses the replicated invisible spectator body during initial preview and slotting catch-up without creating a manual camera. When `BRIEFING` begins, every connected player is reconciled: an unassigned player receives the owner-only enter RPC and locally creates the complete spectator presentation, while an assigned player remains in the spectator body but receives the briefing screen without a manual camera. `GAME` reconciles all players again; players without a living assigned slot use the same spectator entry path, while assigned players possess their characters and receive the leave RPC. The add-on-owned spectator camera derives from `{D6DE32D1C0FCC1C7}Prefabs/Editor/Camera/ManualCameraBase.et`, and its mode and target remain transient on the owning client. KIA uses the same entry path. Entering `POSTGAME` removes any remaining local spectator camera while preserving the authoritative spectator body/state.

Killed-from presentation follows the same client lifecycle. Only the owning client subscribes to its controlled character's damage events and retains the latest direct external source so unconsciousness, bleed-out, healing, passive damage ticks, or respawn cannot replace the original attacker. A structurally empty duplicate callback cannot overwrite an already retained complete impact. The server performs no continuous killed-from tracking; its existing controllable-destruction and deletion callbacks send one owner-only fallback for an attacker that was not streamed to the victim. The owner captures separate causal and terminal visuals: the attacker's visual facts come from the retained direct hit, while the victim's prefab, exact world matrix, controller state, hierarchy, and bone facts are refreshed immediately before control leaves the dead character. For projectile damage, the client reconstructs the incoming shot ray from impact velocity, falling back to hit direction. Character weapon entities, the containing vehicle, and every mounted weapon provide candidate firing anchors for the captured fallback. The displayed trace is then anchored to the reconstructed killer weapon: its muzzle particle or muzzle/barrel bone is preferred, with the weapon's target-facing bound as the final visual fallback. Before screen projection, the client clips that world-space segment against the active camera's near plane; the canvas then clips its off-screen edges without discarding a partially visible trace. Non-vehicle explosives continue to use the damage-source origin, while vehicle-fired explosive projectiles retain the mounted-weapon ray. Each actor is reconstructed by spawning its captured character prefab locally at the literal captured matrix, deactivating clone AI, and applying its captured stance, dynamic stance, signed lean amount, aiming angles, weapon-raised state, and ADS state. The attacker uses the selected K0 live-control treatment, which reapplies its captured bone deltas, animation-graph weapon state, and actor-relative weapon transform every presentation update. Lean, aiming angles, and ADS are written to the duplicate's character input context while raised and ADS are also asserted on its character controller; this drives the full upper-body animation instead of restoring only controller flags. When either actor occupied a vehicle, the owner also captures that vehicle's prefab and exact matrix, the actor's compartment type/index, and every turret's raw aiming rotation and ADS state in deterministic prefab-hierarchy order. Reconstruction spawns the vehicle before the actor, force-seats the actor clone in the matching compartment, and reapplies turret rotation and ADS every update. Killer and victim vehicle paths are symmetric. The owner-only authority fallback remains one-shot: its existing snapshot RPC is followed by a bounded vehicle augmentation RPC only when a killer vehicle exists; a complete locally captured vehicle always wins over that reduced fallback. This preserves the character, inventory, equipment, vehicle, compartment, turret, and animation systems that raw render shells lack. Only duplicate hierarchies are traversed for physics isolation. Every physics body and each of its individual geometry elements are repeatedly assigned `EPhysicsLayerDefs.Unused` because geometry masks are independent from the containing body mask and character, inventory, or vehicle initialization may rebuild either after spawn. After applying character and vehicle state, every physics body in all duplicate hierarchies is repeatedly set to `ActiveState.INACTIVE`; configured simulation states are deliberately left unchanged. This is the verified `ALL-OFF` treatment from the client comparison test and prevents the previews from participating in physics. The source attacker, source victim/corpse, source vehicles, and their gameplay state are never modified. Preview entities, trace geometry, and authored marker widgets are removed when disabled, when spectator presentation is suspended, or when spectator mode ends. Killer previews use a red spectator-style icon and `KILLED YOU` label; victim previews use a blue icon and `YOU` label.

Disconnects preserve UUID, cached name, slot, and entity until `m_iSlotReservationTimeoutSeconds` (default `120`; `0` releases immediately). A connected claim without a persistent UUID is released on disconnect because it cannot form a reconnect reservation. Before `GAME`, expiry clears claimant identity and deletes any staged slot entity; during `GAME` or `POSTGAME`, expiry clears the claim but preserves the slot entity as a match record. Non-slot lobby spectators are deleted on expiry, KIA slots remain unavailable, and shutdown is owned by world teardown without creating reconnect reservations.

Slotting requests are server-authoritative and JIP-safe. Claims, changes, and
toggling out are unrestricted by faction only during `SLOTTING` and remain
`RESERVED`. From `BRIEFING` through `GAME`, a player's authoritative current
base-game faction is the faction lock: later slot requests may target only that
faction, and toggling out is rejected so the lock cannot be cleared. A player
who reaches `BRIEFING` without a slot may make one initial selection, which
establishes the lock. KIA claims cannot switch slots. Entering `BRIEFING` stages
every claimed slot in bounded batches; each pending spawn must re-check the
claim so a released slot cannot be materialized by a stale queue entry.

Tracked controllable destruction is phase-sensitive: before `GAME`, a claimed slot is restaged; during `GAME`, it becomes `KIA` and any connected claimant moves to spectator. Repeated callbacks must be harmless. All authoritative transitions use the existing reliable slot update path, and queued JIP/spawn callbacks are removed during shutdown.

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
- `GC | Admin Action |` for the central audit record of every accepted or rejected admin action.
- `GC | Admin Trace |` for initial role reads, role-change propagation, UI permission state, blocked controls, and server-side role observations.
- `GC | Lifecycle |` for startup and teardown.
- `GC | JIP` for topology/reference resolution.
- `GC | Killed From |` for owner-side damage capture, retained-source selection, authority fallback publication, snapshot merging, preview creation, physics readback, render visibility transitions, toggles, and cleanup.

Log the role, previous phase, target phase, base-game state, and requesting player where relevant. Avoid per-frame or repeated retry spam.

Killed-from diagnostics are event-driven. Every raw damage callback reports its source, instigator, hit position, direction, velocity, and retained-source state; filtered incomplete duplicates explicitly report why they were rejected. Each captured event also reports signed attacker and victim lean amounts, aiming angles, vehicle prefab/transform, seat identity, turret count, selected line-origin mode, and whether it replaced the retained external source. Each actor/vehicle clone spawn reports its prefab, requested and actual root matrices, recursively isolated physics-object count, captured bone or turret count, and `sourceMutation=none`. Seat attachment reports the requested compartment and actual owner/slot. One character-state readback per clone compares requested and input lean, current animated lean, requested and input aiming angles, controller/input ADS, and raised state; one turret readback compares every captured and actual raw rotation and ADS state. One post-deactivation report records disabled killer, victim, and vehicle physics-body counts, followed by hierarchy readbacks containing every duplicate body's layer, simulation/activity/kinematic state, geometry count, and every geometry's final layer. Tracer and label visibility are logged only on their initial evaluation and subsequent visibility transitions, never once per render update.

All authoritative admin-action paths must call `GC_AdminActionLogger.Log` with the requester, action, target or context, and outcome. The logger currently prints one structured record and is the only extension point for future audit destinations; action handlers must not create parallel audit logging.

## Lessons learned

### Global modifications are deceptively convenient

A global `modded class SCR_PlayerController` or an unguarded `SCR_AIGroup` extension makes the first implementation easy but silently changes every mode loaded with the add-on. The guarded `SCR_AIGroup` integration is an intentional exception: it calls `super`, requires the active `GC_LobbyGameMode`, and becomes inert for unrelated modes.

### A blocker does not need to be a separate component

Lobby2's automatic-start policy belongs on `GC_LobbyGameMode`. Native manual `StartGameMode()` remains available because base `CanStartGameMode()` governs automatic pregame advancement, not explicit authority calls.

### A parallel phase authority creates JIP divergence

The base game mode already replicates `m_eGameState` and invokes
`OnGameStateChanged` for JIP. A second Lobby2 phase property can arrive or be
presented in a different order, leaving clients on a stale or missing menu.
Lobby2 therefore extends `SCR_EGameModeState` and uses the base property as the
only authority. The added enum values are explicit and non-contiguous, so
Lobby2-owned serialization validates raw integer values and uses a separate
ordering function.

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
- Derive the phase menu directly from the replicated base game-mode state.

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

- New authoritative transition policy: `GC_LobbyManager` plus `GC_LobbyPhase` if ordering changes.
- New phase state: extend the base enum only when base-game consumers must observe it; do not add another replicated phase property.
- New player-originated request: `GC_LobbyPlayerController` RPC with server-side manager validation.
- New group behavior: extend the guarded `SCR_AIGroup` integration only when the behavior is required for every base group inside `GC_LobbyGameMode`; otherwise use an add-on-owned component on a narrower Lobby2 resource.
- New briefing syntax: parser first, renderer second, action handler only if it creates a side effect.
- New menu view: local view state in the header/menu layer; add a mission phase only when it changes authoritative mission progression.
- New example content: the example prefab or example world, never the production prefab.

If a feature does not fit these boundaries cleanly, pause and revisit ownership before adding another cross-cutting manager responsibility.

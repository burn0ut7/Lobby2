# Mission framework dependency

Lobby2 requires **TilW Mission Framework**, addon GUID `61F0E6AB2D06B1C6`.
Keep its existing addon files unchanged. The local checkout is the sibling
`tilw-mission-framework` directory.

The custom Lobby2 mission framework has been removed, including flags, events,
conditions, actions, interaction prefabs, authoring entities and its test suite.
Old `GC_Mission*` rules and the US-defeated example are no longer supported.
The Arland example retains its lobby setup and now contains one empty TilW
framework entity ready for authored rules.

Use `tests/McpTestAddon/addon.gproj` for development. Its Lobby2 dependency now
also requires TilW; keep the existing isolation probe and MCP test addon loaded.
Confirm actual loaded addons through `workbench_project_context`; a saved project
dependency alone does not establish that the running editor has loaded it.

## Lobby2 integration

`scripts/Game/Integrations/TilW` extends TilW's existing lobby interface from the
dependent Lobby2 addon. The TilW checkout remains unchanged. `#define LOBBY2`
is declared in `scripts/Game/GC_Lobby2Define.c`; integration uses guarded runtime
lookups, not a preprocessor branch in the earlier-loaded dependency. A standalone
define did not activate the tested TilW dependency branch during Reload.
The dependent MCP test addon does observe `LOBBY2=1` (receipt
`rcedf07579fb34d1d9b9f79bdf8cc40ca`).

The adapter supplies the active game mode and state-change notifications, native
player faction lookup, spectator/slot identification, faction-change notifications
for TilW AO limits, removal of unclaimed slots, ticket operations and respawn waves.
Unrelated modes delegate to TilW's inherited behavior. Lobby2 subscriptions and
queued mission work are released on teardown; queued event instructions are
cancelled at POSTGAME. Mission completion goes through Lobby2's phase coordinator.
Existing TilW callback names and the TILW-prefixed notification adapter names are
kept together; new Lobby2 mission operations use the GC prefix.

Ordinary events run during GAME. TilW's existing pregame-event option permits
PREGAME, SLOTTING and BRIEFING; Lobby2 prevents new events at POSTGAME. Lobby2
continues to own phases, briefings, parameters, administration and statistics.

### Faction respawn tickets

On the framework entity, author an **Edit Respawn Tickets** instruction with a
native faction key (for example `US`), operation and nonnegative amount. Budgets
start at zero per mission. Enable its spectator-respawn option to request a wave
after editing the budget. Granting tickets alone does not automatically respawn
on each subsequent death; author another event when the next wave should happen.

Each successful respawn consumes one ticket from that faction. Only connected
players retaining a KIA slot in the faction qualify. A failed spawn refunds the
ticket. Living slots, disconnected reservations and other factions are skipped.
Waves operate only during GAME; budget edits are rejected after POSTGAME. Invalid
amounts, division by zero and arithmetic overflow are rejected without a wave.
Subtraction clamps at zero. Budgets are server-only; no new ticket UI is included.

Respawn uses the slot's original saved group start and formation position, or its
saved admin-created spawn point. It never uses the corpse location; slots without
a saved starting anchor are rejected. Existing admin respawn choices retain their
own behavior and do not spend this mission-event budget.

### Boundaries

- TilW's Edit Map Item faction visibility does not map to Lobby2's briefing
  catalog audiences. It reports an explicit unsupported error without changing
  visibility. Use Lobby2's existing briefing authoring for now.
- TilW source currently broadcasts `true` through its flag-change invoker even
  when clearing a flag. Flag storage clears correctly. This upstream behavior is
  preserved; consumers should query `IsMissionFlag` for the current value.
- Place the actual `{8F846D0FD5D6EA51}Prefabs/MP/TILW_MissionFramework.et` prefab
  in the mission, as in Arland. The upstream automatic-creation resource spelling
  differs; it was not changed here.
- Remote clients, JIP, AO presentation and an addon-free isolation baseline have
  not been validated in this integration pass.

## Dependency loading (2026-10-28)

Workbench 1.8.0.13 loaded TilWMissionFramework, Lobby2, Lobby2McpTests and
Lobby2IsolationProbe together. A single restart activated the new dependency;
script Reload alone left TilW available but unloaded. See Bohemia's
[dependency loading documentation](https://community.bistudio.com/wiki/Arma_Reforger%3AResource_Manager%3A_Options).

Combined Game and Workbench script Reload succeeded. The reload cycle in
`logs_2026-10-28_01-58-33/console.log` (lines 4064–4124) has zero errors and
38 warnings, including existing obsolete APIs and unused TilW variables.
The cleaned Arland world reopened and entered embedded Play with Lobby2's
preview visible. Runtime inspection found its `GC_LobbyGameMode` and manager
(receipt `rf72284dd35f2432cb6e65a54eeff9884`). No SCRIPT errors were logged through
that smoke check. This is startup coverage, not multiplayer or TilW rule-execution
validation. Existing resource/plugin diagnostics remain, including a stale
ArlandLobby world reference and unknown `SCR_DedicatedServerPlugin`; both also
occur in the prior session `logs_2026-10-28_01-21-32`.

All pre-existing TilW source/resource file hashes remained unchanged except
`resourceDatabase.rdb`, which Workbench regenerated while loading the addon.

## Integration validation (2026-10-28)

Embedded Play in the same Workbench process, using the optional
`GC_TilWIntegrationTests.c` fixtures and production instructions/phase requests:

- Reload at `logs_2026-10-28_01-58-33/console.log` lines 6186–6247 completed
  without script compile errors. PREGAME, SLOTTING and BRIEFING kept the GAME
  event flag false; it became true on GAME (receipt
  `r0ab3982bee5e41b685029ea4fac7188b`).
- Native death after moving slot 9 twenty metres from its start set KIA. A zero
  US budget and a USSR-only wave left it dead. US SET 2 with a wave returned the
  slot to its exact start (`fromStart=0`, `fromDeath=20`, `controlled=1`) and left
  one ticket (`rf8e7812e0b1d433596600bcd8ee4095e`,
  `r31d58330ddc94d2a967746aeaef85ff1`; log lines 6631–6641).
- Repeating a wave while alive spent no ticket. DIV 0, overflowing ADD and a
  negative SET preserved the budget and player state.
- End Game reached POSTGAME. A queued two-second event was cancelled and still
  had not set its flag sixteen seconds later; a subsequent SET 5 was rejected
  (`rf49edaeee4c94507922965560d590c1e`,
  `r3aa3c053955b4eb8896d450b877a5d56`,
  `r935f38ea0d234b66bc6d1ed02bbfa394`).
- With both addons loaded, the unrelated isolation world reported no lobby
  mode, state invoker, tickets or framework singleton, and retained native AI
  classification. This matched before and after entering/leaving Lobby2 without
  an intervening script reload (`r0539e6daf20d4c54b5473868e06b2724`,
  `r3e02d51221a440af87a63123f4f873ef`). The final test-only reload and these
  transitions had no SCRIPT errors through log line 7447. This is loaded-addon
  isolation coverage, not an addon-free baseline comparison.

Receipt BEGIN/END logs and resulting state were inspected. The run retains the
existing `Loadout manager is missing in the world!` diagnostic during slot
selection and existing resource warnings; it is not a globally clean-log claim.
An earlier fixture failed because its scripted empty conjunction lacked an
initialized operand array. The fixture was corrected before the runs above.

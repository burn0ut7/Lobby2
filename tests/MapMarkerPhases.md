# Player map markers and mission phases

Player marker creation is locked in `GAME` and `POSTGAME`, using the active
Lobby2 mode's replicated base-game state. Reopening Slotting or Briefing does
not unlock it. Slotting and Briefing before GAME retain stock placement. The
existing read-only Preview screen remains unchanged.

The map UI blocks creation and replacement through editing/dragging. The marker
manager blocks local and shared player insertion; the server RPC independently
checks its current phase before assigning ownership, evicting old markers at the
limit, or publishing. Existing markers, replicated/JIP delivery, dynamic markers
and explicitly server-owned mission markers retain their native paths. Deletion
is unchanged. Hooks query the active mode on each call and own no cached state.

## Reproduction

The marker probes live in `tests/IsolationProbe`; the optional MCP test addon
loads that dependency and supplies the phase/slot helpers described below.
Load it per `tests/README.md`. On the Arland example's
`GameMode_Lobby2`, invoke `SCR_MapMarkerManagerComponent.GC_TestMarkerPlacement`
with `private`, `shared`, `rpc`, or `server`. The `rpc` probe calls the real server
handler on the embedded host; it is not a remote-client transport test. Compare
the returned total of active plus disabled markers before and after insertion.

Use the production phase requests through
`GC_LobbyFreezeComponent.GC_TestFreezeAdvance` and confirm each resulting phase.
For briefing UI, unlock and join a slot through the authenticated production
admin and ordinary slot requests before leaving Slotting. With a map open,
`GC_TestMarkerEditor` supports `open`, `military`, `confirm`, `cancel`, and
`snapshot`; it invokes production UI handlers. No test runs automatically.

## Evidence — 2026-10-24

Workbench 1.8.0.13, embedded host, Arland example. Native script reload succeeded.
Raw MCP requests/results, receipts and original window captures are retained in
the MCP repository's `.local/session-markers/` directory.

| Check | Observed result |
| --- | --- |
| SLOTTING private/shared/server handler | Each added exactly one marker |
| BRIEFING private/shared/server handler | Each added exactly one marker |
| Briefing editor open/confirm | Dialog opened, confirmation created a marker |
| Enter GAME with editor open | Dialog closed; existing two markers retained |
| GAME private/shared/server handler | Count stayed at two; no owner assigned |
| GAME mission-owned marker | Count increased from two to three |
| Reopened briefing in GAME | Normal and military editors remained absent |
| POSTGAME private/shared/server handler | Count stayed at three |
| Addon-free unrelated mode | Private/shared/server handler/mission routes each added one |
| Unrelated mode with Lobby2 loaded, after leaving Lobby2 | Same counts and owners as addon-free baseline |

Briefing capture: `1792829476661756700-6a137f66f0af49c4a7b8af0e33454e11.png`.
Reopened GAME briefing: `1792829504347613500-ea59f49c6a4e493b8f63be02c9c6892d.png`.
Receipt-correlated engine logs were read through MCP. No marker script exception
was observed. The session also logged existing-world/resource problems, including
missing loadout manager, mouse image mask, and legacy resource properties; this
is not a claim of a clean whole-project log.

Earlier attempts are retained: the empty test world lacked a map, the initial
counter omitted disabled markers, and selecting a locked slot did not join it.
Those attempts do not establish UI success. The final run used the normal admin
unlock and ordinary join requests, not a test permission override.

The compatibility comparison used the same marker probe in `Isolation.ent`.
Native addon inventory confirmed Lobby2 absent for PID 10756 and present for
PID 42512. The latter entered and exited Lobby2 PREGAME, then entered the unrelated
mode without script reload. Both returned counts 0→1→2→3→4 and owners 1/1/1/-1.
MCP logs contain matching begin/end receipts. This checks marker handlers after
mode exit, not unrelated-map UI or exit from GAME specifically.

During this work another change to `GC_AdminFactionTests.c` introduced unresolved
test-helper references and the combined test suite stopped compiling. That failed
reload is retained; its files were not changed here. The compatibility run used
an evidence-local project manifest depending only on Lobby2 and IsolationProbe,
with no extra scripts or alternate executor. Both that project and the standalone
IsolationProbe compiled successfully with the final marker probe. This does not
claim that the combined suite's separate compile failure was repaired.

Dedicated server/remote-client RPC delivery, JIP, physical mouse/gamepad inputs,
dragging across the transition, and alternate viewport sizes remain unverified.

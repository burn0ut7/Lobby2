# Collection lifecycle

Use the named `SCR_EGameModeState.GAME`, not a literal integer. In the installed
engine GAME is 1 and POSTGAME is 2; Lobby2's slotting and briefing values are
explicit extensions of the same enum.

- Session logs and mission history start at world initialization. They retain
  joins/leaves, phase times, administration, command history and server health.
- Playback telemetry is armed by default (existing 1024 MiB quota), starts on
  GAME, and stops/seals at POSTGAME. Enabling it before GAME only arms it;
  disabling it disarms it. Authenticated admins can stop/restart it during GAME.
  A restart waits for the preceding recording to seal. POSTGAME cannot restart it.
- Combat, movement, role and radio statistics start through the native GAME
  callback. Their existing `playSeconds` and `connectedDuringGame` contract
  continue to mean connected time during the match, including spectators.
- Lobby/briefing attendance is retained through session joins/leaves and phase
  timestamps. It is not added to the gameplay counter or combat-rate denominator.
  A separately named connected/session-time total can be derived from these
  intervals; this change does not add that aggregate to website player totals.

Playback timestamps retain the existing session-world clock, including the
offset before the first recorded frame. No wire schema or clock origin changes.

## Verification — 2026-11-03

Installed source: `SCR_BaseGameMode.c:436–458` dispatches component start only
on GAME, end only on POSTGAME. Workbench Reload had no script compilation errors;
existing fixture Arland resource-reference errors were unchanged.

Live Anizay session `01a1a123-0027-8560-8400-00000019761d`:
pregame and briefing retained a join with no recorder/statistics batch, even
when invoking the start handler explicitly. Authenticated pregame arm/disarm
succeeded, while requester -1 was rejected without state change. GAME created
recording `01a1a123-b013-8585-8400-0000001cbd98` automatically. At 55 seconds of
session time, gameplay playtime was 11.129 seconds. POSTGAME sealed the recording
with two batches and zero dropped records, stopped statistics, and rejected an
attempted restart. Test had no slotted player; actor/combat sampling was not
retested here. Optional helper: `GC_TestTelemetryPhase` in McpTestAddon.

Audit evidence: Workbench `logs_2026-11-02_23-30-45/console.log:29567–29672`.
No shared base-class override was changed. Dedicated multiplayer was not rerun.

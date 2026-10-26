# Mission Framework validation

## Optional helpers

The test addon adds `GC_TestMissionRules()` and `GC_TestMissionSetup(operation)`
to `GC_LobbyManager`. On Arland the named target is `GameMode_Lobby2`.
Use MCP runtime prepare/invoke with return type `string`.

- Run `GC_TestMissionRules` before GAME to verify phase gating, then during GAME
  to exercise the actual evaluator/scheduler with test-only conditions/actions.
  It checks All/Any, independent deadlines, execution after conditions clear,
  one-shot behavior, cancellation, duplicate names, empty conditions, negative
  delays, unknown factions and unpopulated-faction protection. Its simulated
  timestamps test the production scheduling algorithm, not the call-queue cadence.
- `GC_TestMissionSetup("spawn")` instantiates the production USDefeated example
  only in runtime; `inspect` reads its state, and `delete` removes it.
- Select US and a slot through the existing production UI helpers, advance phases
  through authenticated production buttons, and verify possession during GAME.
  Use the existing `GC_TestHealKill(playerId)` fixture, then inspect KIA/possession,
  event state, delayed native phase transition and receipt-correlated logs.
- `GC_TestMissionIsolation("spawn"/"inspect")` on the optional
  `IsolationProbe` / `GC_IsolationProbeComponent` tests the same production prefab
  in a non-Lobby2 world. It must remain inactive.

## Recorded evidence — 2026-10-26

Workbench PID 1940, version 1.8.0.13; loaded Lobby2, Lobby2McpTests and isolation
probe. Same Workbench process/project retained. Reload operation
`d27a6d6b464d46c2b65a38b933b0f1da` returned `reloaded`. Initial compilation caught
and corrected the entity constructor argument name and a test report exceeding
the engine's string.Format argument limit.

In `worlds/examples/arland/Arland_Lobby.ent`:

| Check | Receipt / result |
|---|---|
| Pregame gating and All/Any | `r74d474b0f6104a34a4f414b8b4af85e8`: pass=1 |
| GAME evaluator/scheduler suite | `rfd0a3bb278db4bee9a17706605a9bd9c`: all checks 1; livingUS=1, observedUS=1 |
| Authored example before death | `r71fae35888de492b87d65c6d95fb6060`: GAME, started=0, pending=0, attempted=0 |
| Production player damage | `rce2e6da272724908bb49045f112b6a97`: death arranged |
| Pending action readback | `r0554754b61a74ffead5e6bd4a6442a85`: GAME, started=1, pending=1, attempted=0 |
| Native slot outcome | `r7d435189212f4739b58dfd4ce2162ce5`: slot 11 KIA, no possession |

MCP `log_read`, Workbench session `logs_2026-10-25_20-20-49`, lines 7039–7061:
death receipt ends at 00:39:54.083; event starts at 00:39:54.248; native
`EndGameMode`/POSTGAME transition occurs at 00:39:59.267; the action reports
`applied=1` at 00:39:59.285. This verifies the authored five-second delay against
real world time, with evaluation granularity. The normal game-over flow then
restarted the scenario after 30 seconds, removing the runtime-only fixture.

Existing world/resource/GUI warnings and a missing loadout manager message were
observed; this is not a clean-log claim. Negative configuration tests deliberately
emit validation errors. Native end-screen widget warnings were also retained.

Coverage is embedded host with one real connected player. Dedicated server,
remote proxy/JIP, disconnect/reconnect, unconscious-player and simultaneous
multi-faction outcomes are not runtime-verified here. No shared production
override was added or changed. The broad TilW feature catalog is outside this slice.

The cancellation assertion calls the production runtime's `Stop`; it does not
prove deletion of a placed entity while an action is pending. Prefab spawning
was tested in an already initialized world, not an authored world-start instance.
Duplicate event names were tested, but multiple Mission Setup entities have not
been exercised together at runtime. These remain separate integration cases.

Final native reload `45701b2a113b4ab585c4cd6bcda0a936` returned `reloaded` after
the diagnostic index and optional isolation helper were added. In the unrelated
isolation world, receipt `r518d3811bea5444e9fb759dc16cf7568` identified
`SCR_BaseGameMode` and `SCR_PlayerController`. The production example spawned
under `rcdcac2b484e14123a9a152c842c95fcd`; subsequent receipt
`rd388f216dd4042ecbeab17fe1f651443` reported `setup=inactive` with native state 1.
No Mission Framework execution logs appeared in that reload cycle. This tests
inactive ownership after leaving Lobby2 with the addon still loaded; it is not
an addon-free baseline comparison. The test was stopped normally afterward.

Native prefab-definition inspection verified the saved example separately from
runtime state: one `GC_MissionEvent`, name `US defeated`, All conditions, faction
`US`, At most `0`, End Mission delay `5`, and winning faction `USSR`. The example
opens through its GUID and inherits the empty Mission Setup prefab.

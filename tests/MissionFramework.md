# Mission Framework validation

## Current game-mode authoring

Open the optional test addon's `worlds/MissionFramework.ent`. It places
`GameMode_USDefeated.et` as its sole game mode, named `MissionFrameworkMode`,
with a playable US group. This tests authored world-start initialization.
Use the generic MCP runtime interface on its `GC_LobbyManager` component:

- `GC_TestMissionFramework()` reports the game-mode-owned runtime.
- `GC_TestMissionRules()` runs the same production evaluator/scheduler tests
  before GAME and during GAME. Synthetic timestamps test scheduling decisions.
- Select US and an available slot with the existing UI helpers. Advance phases
  through the authenticated production buttons; inspect possession, kill that
  player through `GC_TestHealKill`, then observe the five-second native POSTGAME
  transition and logs.

The former spawn/delete-setup and isolation-spawn helpers were removed along
with the separate setup entity. Historical evidence below describes that earlier
version and is not a claim that its authoring surface still exists.

## Game-mode migration evidence — 2026-10-26

Native reload `0c8c1c00022842678d9e6c58f2cd4cfa` succeeded in the existing
Workbench PID 1940. Native entity inspection of the authored test world found
`m_aMissionEvents` on `MissionFrameworkMode` itself, containing the event inherited
from `GameMode_USDefeated.et`. No separate setup entity exists in that world.

- World-start initialization: `r98b68a6a9be747c8947399b0cff20f04` returned
  PREGAME, stopped=0, started=0, pending=0, attempted=0.
- Pregame suite: `r0682f3ee28954c9c9273c1d1acdb125f` passed.
- Original Arland game mode with an empty event list:
  `r038fc090c120426e864e3b80039d043c` returned `phase=PREGAME inactive`.
- GAME suite: `r0b77742c3848455b8bd5eed52392612d` passed all checks with livingUS=1.
- Player 1 was verified possessing slot 1, then killed using the existing damage
  fixture under receipt `r7cff6109695846dc9a105e1c2b094194`.
- `r6b1c6e2d93674cb795554ff6e1675245` observed GAME, started=1, pending=1,
  attempted=0. `r959f4c5949b54359b2f1d5f61cd352de` subsequently observed POSTGAME,
  stopped=1, started=1, pending=0, attempted=1.
- Workbench session `logs_2026-10-25_20-20-49`, lines 8068–8087: event started
  01:52:04.585; native EndGameMode/POSTGAME at 01:52:09.596; action applied=1
  at 01:52:09.613. This exercises the game-mode-authored five-second delay.

Embedded-host coverage only. Dedicated/remote/JIP behavior and teardown with a
still-pending action were not separately tested in this migration. Native end
screen warnings remain. The standalone-entity initialization and duplicate-setup
limitations below are historical; setup entities have been removed. A new
world-start initialization check is recorded above.

## Historical standalone evidence — 2026-10-26

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

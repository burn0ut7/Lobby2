# Mission Framework validation

## Current lobby-component authoring

Open the optional test addon's `worlds/MissionFramework.ent`. It places
`GameMode_USDefeated.et` as its sole game mode, named `MissionFrameworkMode`,
with a playable US group. This tests authored world-start initialization.
Use the generic MCP runtime interface on its `GC_LobbyManager` component:

- `GC_TestMissionFramework()` reports the lobby-component-owned runtime.
- `GC_TestMissionRules()` runs the same production evaluator/scheduler tests
  before GAME and during GAME. Synthetic timestamps test scheduling decisions.
- Select US and an available slot with the existing UI helpers. Advance phases
  through the authenticated production buttons; inspect possession, kill that
  player through `GC_TestHealKill`, then observe the five-second native POSTGAME
  transition and logs.

The former spawn/delete-setup and isolation-spawn helpers were removed along
with the separate setup entity. Historical evidence below describes that earlier
version and is not a claim that its authoring surface still exists.

## Lobby-component migration review — 2026-10-26

Mission Events now belongs to GC_LobbyManager on the game-mode entity. The
example overrides that existing component by its inherited ID. Initialization
runs from the component's OnWorldPostProcess; Shutdown cancels deferred startup,
ticks and pending actions. Context and callbacks require that exact active
manager, so a stopped or replaced component cannot execute rules.

Source checks: one attributed event array, one framework Start call, all context
constructors updated, example component ID matches the base prefab, and
`git diff --check` passes. The evaluator and action delay rules are unchanged.

### Live verification — 2026-10-26, 09:08–09:10 EDT

User requested opening Workbench with tests and leaving it open. PID 20124,
Workbench 1.8.0.13, loaded Lobby2, Lobby2McpTests and Lobby2IsolationProbe.
Reload `7efb54624f4142d79d044cb9bbcecd34` returned `reloaded`.

Native inspection of MissionFrameworkMode found GC_LobbyManager at component 2
with one inherited GC_MissionEvent in m_aMissionEvents. The entity root's full
86-property listing had no mission-framework fields.

- Authored initialization: `r6059a55e6aeb4e50acbbc68fac9c38f3` returned
  PREGAME, stopped=0, started=0, pending=0, attempted=0.
- Pregame evaluator: `r1617773f8a374aaa815753efda2f4095`, pass=1.
- Production UI helpers selected US and slot 1 and advanced through briefing
  to GAME. Possession confirmed by `r6f66eae6e9454473a470c82f64a48c11`.
- GAME evaluator: `r2743562d5da54b4aaa4cd1578d3dc458`, all checks passed:
  All/Any, independent deadlines, one-shot execution, cancellation, invalid
  configuration rejection, unpopulated protection and native livingUS=1.
- Death: `r5edca3324ea54194b6bfcfdfc7e15280`.
- Pending: `rec65b8b786e8438687473f8795563cbc`, GAME, started=1,
  pending=1, attempted=0.
- Completion: `raf6d54565fbf40fb8b0b8d1ae147b336`, POSTGAME,
  stopped=1, started=1, pending=0, attempted=1.
- Session logs_2026-10-26_09-08-04, lines 742–761: event started
  09:09:43.186; native EndGameMode 09:09:48.190; action applied=1
  09:09:48.205. MCP screenshot showed the Soviet victory end screen.
- Returned to Arland: `r1b01d757245743179ca01ce0def860a3` reported
  PREGAME inactive with its empty event list. Play stopped afterward;
  Workbench and test addons remain open on Arland.

Embedded-host coverage only. Cancellation above tests the runtime Stop contract;
world teardown with an action still pending, dedicated/remote/JIP behavior remain
unverified. Existing resource, missing map/loadout-manager and end-screen widget
messages remain; this was not a clean-log run. The invalid-configuration errors
are expected assertions from the evaluator suite. No production changes were
needed during verification.

## Historical game-mode migration evidence — 2026-10-26

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

## Framework expansion — 2026-10-26 (in progress)

Workbench PID 20124, 1.8.0.13, Lobby2 MCP Tests; installed source revision
4ee49efc58494a12b2fde693a84d62e9. Native reloads 272be0ea6b70475faf5a566278cccbe7,
d66350f045804acf9a787d49fc5ee301, 5e610c8ea01249d39ae2f008e5b8a327 and
a4df96dbd6e64c3bb71ddf30a07c3eec returned reloaded. Earlier test compile failures
(reserved variable name and too many string.Format arguments) were corrected.

- Expansion suite refbf438839f34d13a287044300e37e9b: pass=1 for nested groups,
  inversion, held timing, Run/Rearm/Disable, automatic/latched flags, random
  boundaries, sampling without mutating authored arrays, Stop rejection,
  cancellation of delayed follow-ups, percentage boundaries and capture reversal.
- Pregame suite rd04c817ca99440fa940c3737ec83681c: pass=1 phase gate.
- GAME suite r9bfad6ccd7b8485a8e010a7c24074c21: pass=1 including native player
  observation (US casualty percentage 0), one-shot deadlines and invalid config.
- Actual casualty end: rbbe99be0b78a43cb94814246bfb5d136 killed the sole player.
  rb29fe38f48e84fbba1515a83d29a489e observed POSTGAME, started=1, pending=0,
  attempted=1. Log session logs_2026-10-26_09-08-04 lines 1810–1828 shows trigger
  10:06:03.214 and completion 10:06:08.274.
- World actions rfdaf122b41cb445781738114ca882d4c started the armed M1025 and
  exercised spawn/waypoint add/clear. r819a72a3e79f466f99d8c8b201fe533a dispatched
  deferred boarding. ra2aac1a928dc41dd917f2ce9a77aec1f confirmed pass=1, two
  occupants, fuel at 25%, health at 50%, area membership, hidden briefing with
  preserved audience, budget arithmetic and deletion.
- Initial world test failed because it checked boarding synchronously, used the
  unarmed M1025 while expecting a gunner, and used briefing index 999 (max 32).
  Corrected fixture uses the armed prefab's exact GUID, a separate deferred
  readback, and briefing 32. Production visibility validation now enforces 1–32.
- Environment re0e8b5fd83104ef1a6e766b74f1c3cac: native time readback matched
  15:30 and was restored; weather Clear request dispatched. This is not evidence
  for a transition to a different weather state.
- Presentation r64f9c358742d455e99d7ca21e43fe242: custom component invoked once,
  repeated only when enabled, and executed on the local-client path. Hint/chat
  dispatched. Native hint and chat visible in Art/References/mission-framework/message.png.
  Later reruns were for screenshot capture; their once/count assertions fail
  because the fixture intentionally retains the script's execution history.
- Set Flag native character DoStartObjectAction / DoPerformObjectAction:
  rb172e1f2f1e54caea2965b9053fbc0a8 setup, rd89e269e3e5445969a81553870720699 start,
  r2dcac196fabd478fb3c931e5f753d5c7 performed. Logs 3049–3050 show flag set and
  interaction applied=1. The combined snapshot's boolean fields need further
  checking; the native log confirms the mutation. A denied-faction call after
  clearing the flag left it false (r4536cee8316b44ed97833fd3df7dba9d).
- Teleport r1184dfbcb72a434a83187c97f75bb7d3 performed via the native action path;
  r4763e202cd744a5ca9abf6ebdd90d0d9 measured 0.0000216531 m from destination.

Additional embedded-host evidence, same log session:

- Reload febeab935c734692b5053112a9493007 succeeded, including native AI hold settings.
- Core regression r7b28041c359d4c50a52620f904b163be and vehicle regression
  r6fd8241af58a4f7dbabc4197e5a80928 both passed.
- Utility test r0c6d0d324e284f35aac0d33c9772e0c2 passed area positions,
  protected selection/deletion/release, valid flag snapshots, and rejected-null
  flag snapshots. The deliberate invalid flag logs an expected error.
- After fixing the circular sampling attempt budget, rd386c3213893482dbfb8ee82e501bf91
  spawned and counted all 1000 requested entities, then removed the fixtures.
- Native Set Flag action rd708165920df46fc9a67bde96129ccab followed by
  ra442df8bcf634661aeaff2746170b937 confirmed flag=1 and canPerform=0.
  The previous combined snapshot reused expression temporaries; capturing the
  values separately fixed the test's readback.
- AI casualty test rd2e86ceca5ab46089181bb952a8ff93b spawned a native US soldier;
  r9c878a7fc0d6439bb03cfb576f74975d observed 0% before killing it through the
  production health action. r251faebcb36f4175987375bf52b10f98 confirmed dead=1,
  recorded=1, after=100%.
- Respawn test disabled the fixture's ending event, killed player 1, and verified
  KIA with possession released (ra1c358f5c0e945cda27e071fe843fbc5).
  rccb601b100044798bede5dcd3972535c applied a one-ticket respawn wave and consumed
  its ticket; r3ff30236c7c2483083e1950f1d3d1b10 confirmed a new character, KIA=0,
  possessed=1, active player state.

- Turret fixture: r4b88674428be493cad8e3631306c12a3 returned seated=1,
  dismount=1 before marking. r927f29d8b7d4469f841927f842c1343a returned dismount=0
  after marking. rc94d3837f0954e29bbf164dd138d1022 returned dismount=1 after Stop,
  with the previous native movement setting restored. Hold readback
  ra6346092759548389921a7e7279c584d returned applied=1, idle=1; this alone does not
  establish actual stopping/resuming movement.
- Unrelated SCR_BaseGameMode after leaving Lobby2, addon still loaded:
  r6776dc5419ae4cf79c4f71545e60ff8e returned seated=1 dismount=1.
- Addon-free baseline, PID 21424, native loaded-addon snapshot
  f0258f2720094dbb87adc12ae68e4c41 contained core, ArmaReforger and the independent
  IsolationProbe only. Same fixture returned seated=1 dismount=1 at
  rd4924160f0924c4ca894bc0671a4af68, matching the unrelated loaded-mode result.

- Supply/marker test r160ddf9722cf4751af2d0ec6067e7c95: native M923A1 supply
  container read back 37; marker hide preserved US audience and show with audience
  replacement changed it to USSR. All assertions passed.
- Dedicated suite GC_DedicatedMissionSuite, server PID27368 and remote peers
  PID61176/47752, launched through MCP dedicated tool operation
  7864c1c4ed7441b995a1acf65ba565a3. Server log
  ArmaReforger/logs/logs_2026-10-26_11-13-54/console.log lines783–882 records
  initial state before either client connected, both joining client reports
  flag=1/briefingHidden=1/authority=0, both update reports
  flag=0/briefingHidden=0/authority=0, and native test SUCCESS at11:14:40.771.
  This proves joining snapshots and live updates for flags and briefing visibility;
  it is not blanket multiplayer coverage for every action.
- The first movement fixture on the user's Arland scene failed native pathfinding;
  logs reported missing soldier/vehicle navmesh files. This is not a hold-action
  pass. A separate MissionFrameworkArland test scene supplies shipped GM Arland
  navigation files without changing the user's scenario.

Navigation rebuild, automatic-cleanup exclusion and local debug-area rendering have not received live outcome verification. Native terrain-object deletion has not been verified across remote clients.
Existing empty-world map/loadout and native resource warnings remain; this is not
a clean-log run. Screenshots show native UI reuse, not a new layout. The optional
keep-gunners-seated feature adds a scoped SCR_AICombatComponent override.

### Final crew and movement checks

- Custom crew variants: rceef27004a3f496d9d5917ad41b8e1cb returned pass=1,
  crew=2, customCrew=2 and all vehicle/area/catalog/ticket assertions true.
- The initial native movement-speed-setting approach failed: formation movement
  continued despite wantedSpeed=IDLE. It was replaced with native movement controls.
- Reload 314d00f25d78497c862480d6766f569b compiled the current implementation.
- Native single-member group moved 7.58 m (rf4e03bb5bb99493cb39980b143136c23).
  Hold raf8f6d8686414f74a32df8aeee560678 changed disabled=0 to1.
  r30446687cadb4b61a4de3617dbe26e27 stopped within0.59m (deceleration).
  Release r46b3ba32c9de4d1d996d880adbd2779f changed disabled=1 to0.
  The old movement request did not resume automatically; after reassigning the
  waypoint, r76d3b3483d49487aaee2a6d2b4f2ea40 measured17.16m of movement.
  This confirms movement is permitted after release, not automatic order recovery.
- Runtime Stop restored the prior control value, rbff3e332d5a7482198934d9796c0f708.
- Holding then native player takeover and production runtime evaluation returned
  possessed=1 disabled=0 (r053c72bbb11b40369113a02d96cc384d). This tests cleanup
  against real possession, not a simulated player identifier.
- Perception/fire controls are not modified in source; live combat firing while
  held and dedicated replication of this restriction remain unverified.
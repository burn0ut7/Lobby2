# Player body retention

GC Lobby Dev gives a player character 3,600 seconds of native garbage-collection
lifetime at death. An existing shorter remaining lifetime is extended; a longer
one and native blacklist exclusions are preserved. AI and spectator proxy bodies
are excluded. Respawn leaves the dead player body with native cleanup instead of
deleting it immediately. Explicit admin deletion and world teardown can still
remove it; native proximity rules may keep it longer than an hour.

The implementation uses GC Lobby's existing server death callback and the native
Insert/GetRemainingLifetime/Bump APIs. It has no world scan, recurring callback,
body registry, extra replication or global base-game override. Retaining the
entities still increases their memory/rendering cost compared with early cleanup;
this is not a measured performance benchmark.

## Explicit local checks

Temporarily mount this directory's scripts/Game and the sibling
RespawnLocations/scripts/Game under separate Workbench profile scripts/Game
junctions. Reload, start a fresh local Arland example session, and invoke these
methods on GameMode_Lobby2's GC_LobbyManager through Reforger MCP:

1. GC_PrepareLocationWaveTest(false), then GC_FinishLocationWavePreparation after staging.
2. GC_KillBodyRetentionPlayer(), then GC_CheckBodyRetention after death handling.
3. GC_TestBodyRetentionPolicies(): insertion before native death insertion,
   extension of a 30-second deadline, preservation of a 7,200-second deadline,
   and respect for a native blacklist.
4. GC_TestAIBodyRetention(): an unpossessed character's context must not get an
   extended lifetime. This immediate check does not prove AI expiry timing.
5. GC_RunLocationWaveCase(0), then GC_CheckLocationWaveCase after completion;
   GC_CheckBodyRetention must still find the original body after respawn.

These fixtures kill/respawn the local player and temporarily change native timers;
use an isolated embedded session. Stop Play, remove only the temporary junctions,
and Reload afterward. Nothing runs automatically or ships in the release addon.

## Source evidence

Installed game catalogue revision c52648a62fcb4229ad089610d060ea44:
- scripts/Game/generated/System/GarbageSystem.c:15-47 documents timer units,
  insertion, lookup and lifetime extension.
- scripts/Game/Character/SCR_CharacterControllerComponent.c:323-335 inserts a
  character on death.
- scripts/Game/GameMode/SCR_BaseGameMode.c:1347-1367 supplies the native victim
  context and dispatches to game-mode components.

Full-hour expiry, dedicated/multi-client behavior and a crowded-corpse performance
benchmark are not covered by these short embedded checks.

## Verification, 2026-11-13

Workbench 1.9.0.37, fresh local Arland sessions through Reforger MCP:
- Real player death: retained=1, remaining=3594.91 seconds.
- Native insertion/extension/longer lifetime/blacklist cases: 4/4 passed.
- AI context: playerId=0, unchanged native remaining value (-1 before/after).
- Wave respawn: case=0 passed=1 tickets=1; original corpse still present with
  3587.66 seconds remaining (operation ca022501a9e6490d81b2e5afb246d3c3).
- Scripts compiled successfully. The scene's existing ArlandLobby resource GUID
  errors, invalid sample mission event and missing loadout-manager message remain
  outside this change.

The respawn check initially exposed immediate deletion in the replacement owner;
the final repeat above verifies its correction as well as the native timer.

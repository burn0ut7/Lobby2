# Phase elapsed time

A plain `mm:ss` readout sits at the right of the existing lobby header, before the player count. It resets when the authoritative game state enters Preview, Lobby, Briefing, Game or Debrief. Changing menu tabs does not reset it. There is no gameplay HUD timer in this slice. Minutes continue beyond 59.

The server manager starts the initial clock in OnWorldPostProcess and updates it on phase transitions. A reliable RPC supplies existing clients; the replication snapshot supplies late joiners. The clock epoch carries its phase so an out-of-order phase/clock update cannot show the previous phase's time. Headers compute elapsed simulation time from the engine-synchronized server timestamp and refresh once per second. Detaching removes the scheduled update; no per-second network messages are needed.

## Validation, 2026-10-24

Workbench/game 1.8.0.13, Arland_Lobby, optional MCP Tests addon. [Phase receipts](../Art/References/briefing-timer/phase-runtime-evidence.json) supersede the older briefing-only evidence. [Final 1280x720 client capture](../Art/References/faction-readiness/compact-peer-1280.png).

- Native script reload passed. Initial Preview clock starts without requiring a phase transition.
- Host and remote client observed zero at Lobby, Briefing, Game and Debrief entry.
- Briefing tab navigation retained elapsed time (104 to 105 seconds).
- A late-joining client received the current briefing epoch and displayed 00:39 alongside authority elapsed 40 with label 00:39. A one-second display lag is expected between refreshes.
- General timer source and multiplayer evidence independently reviewed and approved.

Dedicated-server execution, unusual UI scales and hour-long live timing were not exercised. One editor reload during testing raised an engine resource-leak assertion; subsequent final reloads succeeded. No fix to that engine assertion is claimed.

API provenance: installed index 49a93d9dd25b44e6b3f3399cccf389ae; ChimeraWorld.GetServerTimestamp; SCR_RespawnTimerComponent reliable timestamp RPC; SCR_RespawnTimer 64-bit snapshot serialization; SCR_BaseGameMode.OnWorldPostProcess component dispatch.

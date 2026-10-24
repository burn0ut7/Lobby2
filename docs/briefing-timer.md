# Briefing elapsed time

A plain `Briefing mm:ss` readout sits at the right of the existing header, before the player count. It is visible only during BRIEFING and continues across menu navigation. Minutes continue beyond 59. It is elapsed simulation time, not a countdown or a match-start condition.

The active server manager captures one `WorldTimestamp` on entering BRIEFING. A reliable RPC supplies existing clients; the manager replication snapshot supplies late joiners. Each header computes elapsed seconds from the engine-synchronized server timestamp and refreshes once per second. No per-second network updates or client-owned start times. Leaving Briefing or detaching the header removes the scheduled update.

## Evidence (2026-10-24)

Workbench/game 1.8.0.13, Arland_Lobby, optional Lobby2 MCP Tests addon. [Raw MCP receipts and logs](../Art/References/briefing-timer/runtime-evidence.json), [host](../Art/References/briefing-timer/runtime-host.png), [1280x720 peer](../Art/References/briefing-timer/runtime-peer-1280.png).

- Native script reload passed after correcting a World-to-ChimeraWorld static type error.
- PREGAME/SLOTTING hidden; phase entry starts at zero; minute rollover observed at 02:37; navigation away/back retains elapsed time.
- Existing remote client and authority both reported BRIEFING elapsed 0 with visible 00:00.
- A new peer process joined mid-briefing and reported 65 seconds alongside authority 65. Opening briefing then showed identical elapsed 72 and label 01:11. The once-per-second presentation can trail the sampled elapsed value by one second.
- Production Game transition returned elapsed -1; the remote reopened briefing header reported visible 0.
- Header is legible and clear of navigation/player count at the captured host viewport and 1280x720 peer viewport. No new interactive controls.
- Independent reviewer approved source, screenshots and multiplayer evidence. Readability/diff check passed; existing engine callbacks and mixed surrounding legacy formatting retained.

Initial multiplayer connection was rejected for duplicate engine identity. Restarting Workbench and restoring its temporary local-server test setting cleared it. The failed connection is not counted as timer evidence. Dedicated-server execution, unusual UI scales and hour-long live timing were not exercised.

API provenance: installed game index 49a93d9dd25b44e6b3f3399cccf389ae, generated Core/ChimeraWorld.c GetServerTimestamp, SCR_RespawnTimerComponent.c reliable timestamp RPC, and SCR_RespawnTimer.c 64-bit timestamp snapshot serialization. These installed sources supply the clock and replication pattern; runtime evidence above verifies this implementation.

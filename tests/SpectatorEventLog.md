# Spectator event log

Default off. J toggles Event Log in the spectator options; the action is rebindable. You caches personal history for the open spectator menu lifetime. Everyone refreshes the newest 20 combat events every second. Clicking either view moves the spectator camera around the recorded target position. Coordinates are not rendered.

Rows contain session elapsed time, reported damage (with a killed suffix for a paired fatal hit), attacker > victim, damage type, weapon and distance. Same-actor/same-target damage and death in the same engine tick are combined only in this projection; canonical records remain separate. Bleed-out and respawn remain separate outcomes. A death without a matching hit has no invented damage amount. Reported damage is engine callback damage, not net health lost.

## Bounds

History comes from the existing shared 4096-event session ring, not the persistent server archive. You pages older retained events by scrolling; older records can expire before retrieval. Everyone is a latest-20 feed and can omit intermediate events during very large bursts. Personal history is not retained across menu destruction or sessions. Remote multiplayer transport, gamepad interaction, long-name tooltips and alternate UI scales have not been exercised in this pass.

## Live verification, 2026-10-29

Reforger 1.8.0.13, Workbench PID17588, Lobby2 MCP Tests, Empty_Lobby.ent / GameMode_Lobby3. Production handlers used through the optional test addon. Final script reloads succeeded; existing ArlandLobby GUID, SCR_DedicatedServerPlugin and m_aFormations resource diagnostics remain unrelated.

- Alive-player server request rejected: re23d9b7a03044c84ba3e51699fb738d3.
- Logged-out ordinary spectator was allowed in the initial pass.
- Native damage/death produced separate hits and one combined fatal row; latest label report r17e877435fbe4c5aacee0a1838048f6e.
- Personal scrolling loaded20 then27 rows, next0: rb259eeca91ec45588c6e3e7c622accd5.
- Personal row click moved camera: rde0b5c90b7b6460395805a8b7bc17860.
- Everyone grew9 to11 after additional native AI damage, without a refresh button: r48b56da663094701834e8d48d1326b7a. Its row click moved the camera to179.365,15.831,124.007.
- Switching to You and closing/reopening retained its3 personal rows: r5ced5101b08449c69a926f73a75ffa44.
- Named hotkey dispatch and option binding worked. J replaces conflicting L (ManualCameraLight); native J uses Tasks/MenuJoinGroup outside the spectator contexts.
- Peer test client could not connect to offline embedded Play; it was stopped and original peer settings restored. This does not verify owner RPC transport over a network.

Local visual evidence: Art/References/spectator-events/concept-v2.png (illustrative design), runtime-everyone-v2.png and runtime-combined-v3.png (MCP captures). Independent review approved composition; source review led to fixing unpaired team-kill labeling. No stock shared override was added; this feature extends Lobby2-owned classes only.

## Reproduce

Load the optional MCP test addon. Enter GAME with a selected slot. GC_TestCombatAI spawn/hurt-ai-twice/kill-player uses native damage and transitions the host into spectator. GC_TestSpectatorHotkey toggles the named action with1 then0. GC_TestEventLogSnapshot reports actual UI rows. GC_TestScroll EventScroll1 pages You. GC_TestEventLogCombat3 generates native AI-to-AI damage; use a positive damage amount large enough to trigger native callbacks. GC_TestClick activates EventEveryone, EventYou, EventClose and SpectatorEvent_SEQUENCE. GC_TestEventLogRead calls the production authority read. GC_TestPeerCommand spectator-events can request you/everyone/snapshot/open/release on a connected test peer.

Final J hint resolved in live HUD with event log ON and light OFF: r6fd18d40592a4bc6bad81917a329148a. Final reload: 7cc1638a64ef47ea9e23202734c48b9d. Independent review approved after team-kill fix and combined-label capture.

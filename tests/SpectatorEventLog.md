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

Load the optional MCP test addon. Enter GAME with a selected slot. GC_TestCombatAI spawn/hurt-ai-twice/kill-player uses native damage and transitions the host into spectator. GC_TestSpectatorHotkey toggles the named action with1 then 0. GC_TestEventLogSnapshot reports actual UI rows. GC_TestScroll EventScroll1 pages You. GC_TestEventLogCombat3 generates native AI-to-AI damage; use a positive damage amount large enough to trigger native callbacks. GC_TestClick activates EventEveryone, EventYou, EventClose and SpectatorEvent_SEQUENCE. GC_TestEventLogRead calls the production authority read. GC_TestPeerCommand spectator-events can request you/everyone/snapshot/open/release on a connected test peer.

Final J hint resolved in live HUD with event log ON and light OFF: r6fd18d40592a4bc6bad81917a329148a. Final reload: 7cc1638a64ef47ea9e23202734c48b9d. Independent review approved after team-kill fix and combined-label capture.

## Compact refinement

2026-10-29: moved panel to bottom-right, reduced footprint to 33.5% viewport width and 24.5% height, tightened rows 76 to 56px while preserving font sizes. Spectator elapsed timestamps omit milliseconds; admin precision remains unchanged. Reload 7ee8ff6a397645038f9f3cfb10c8e3fa passed without script errors. Actual capture Art/References/spectator-events/runtime-compact-v4.png and snapshot r8ea69fd4efbd49a2b1a0b0d5083484da confirm compact placement and 00:01 timestamps. Native header/row handlers exercised.

Independent visual/source review approved the compact refinement. Camera movement after row activation confirmed by r1db566d9cc8c44d4a0a033d9aaa4dc44; Everyone showed nine whole-second rows after native damage (r80121b1b33504b958e6f0056c9d22cb8).

## Pointer routing regression

The original GC_TestClick called the component directly and did not test hit routing. Native WidgetManager.ReportMouse over EventSummary/EventInfo returned those text widgets (button=0), receipts r79081d9eec7142628405a209776baa19 and r7c8e50870b3f4a80b7179213344a8c4d. Decorative children now ignore cursor input; the root button owns the tooltip. The same probes return SpectatorEvent_11 (button=1), receipts r25c614007bf64441998c4ede138b8596 and r883fba2cbef04559889c7dda2732bfd8. Handler dispatch separately moved the camera from 135.297,4.01563,123.139 to 134.331,15.9846,97.234, receipt r439ded2d75d242be80f3cf17f1e7f66f. This validates native pointer targeting and production camera handling separately, not an automated physical mouse press/release.

Reload 1bc4e042c4a34f04bf3c39645ce4b15e succeeded after correcting an override argument-name compilation error. Existing unrelated resource diagnostics remain. Actual capture: Art/References/spectator-events/runtime-click-v5.png. GC_TestEventLogPointer(rowName, cellName) is the optional regression probe.

Native root hover populated both tooltip lines (runtime-tooltip-v5.png). Moving the pointer away dismissed it (runtime-tooltip-dismissed-v5.png), using native ReportMouse and MouseDeviceHandler cursor movement via existing test helper.

## Five-row refinement

The panel is now 560x346 reference pixels anchored 24px from bottom/right, with 290px for five 56px rows plus 2px spacing each. Top/left padding is 8px; bottom/right is 4px. Header stays 36px high. Background opacity is 0.68 instead of 0.90. Z order 0 places the complete log below voice/player drawer 100 and spectator options 110.

Reload eea83b8c84704a25941823b2066678ac succeeded without script errors; existing resource diagnostics remain. Native damage produced five personal entries. MCP captures runtime-five-v6.png, runtime-options-v6.png and runtime-voice-v6.png in Art/References/spectator-events confirm five full rows, tighter width/padding, transparency and both expanded panels over the log. Drawer opening exercised production SetHandleFocused and native animation through the optional existing test helper. Other screen sizes were not exercised in this pass; dimensions use the same native reference-pixel scaling as row heights.

## AI event scope

Everyone includes damage and deaths with AI targets, including AI-only combat. You excludes AI-target events even when the local player caused them, but retains AI attacks and kills against the local player. The server projection filters before pagination. Canonical AI-only deaths now pass through RecordDeath; they do not enter the player statistics death batch or alter player kill/death counters. Unknown non-player victim types retain the prior rejection.

This refinement compiled in the current Arland project. Existing earlier native checks cover player-to-AI damage/kills and AI-to-AI damage. AI-only death recording and the new personal exclusion were source-reviewed; no new destructive combat test was run in the user's connected Arland session. Prior personal-history captures predate this exclusion.

## Feed optimization and default tab

Everyone is the initial tab, on the left; You is second. Button proportions, wording, font size and height are unchanged. Panel width is now 476 reference pixels (15% narrower than 560), with the same right margin, height and five-row capacity.

The server remains authoritative for combat and snapshots historical names, damage, weapon, time, distance and world position. Clients receive value records over their owning controller, not replicated entity references; display, scrolling, cached personal history and camera movement remain local. Only open Everyone views poll once per second; closing/hiding removes the poll. No general broadcast to living players is introduced.

Requests include the last combat revision. An unchanged authorized request returns an empty acknowledgement before scanning/formatting rows. New combat and eviction of retained combat advance the revision; unrelated entries do not. The fatal-hit index is shared by readers and rebuilt only after this revision changes. Changed responses remain bounded to 20 events; this is not a lossless archive stream. Client and server must update together because the RPC contract now includes revision/unchanged.

Live validation in Arland (GameMode_Lobby2): first 3 rows revision 10, repeat 0 rows unchanged 1 (rf591999e0f7645ffb21ba97fc218af0a); additional native AI damage/death advanced revision 41 and 20 rows, repeat 0 unchanged 1 (rf72e5b5d2a35494084d07c1663995bfb). Invalid requester returned revision -1, unchanged 0, denied. AI-only fatal row appeared (rcf04451a1c1f496b8c2028e042010030), closing the earlier AI-death verification gap. Personal view was empty for AI-only combat (ra1013a52e2b24e3cbd9cdf435df4f2f7).

Connected non-admin spectator peer 2 received 20 rows, revision 41 (rf4c45a836f284b7b97e903dd99a7f7b5), then 0 rows with unchanged 1 (rac389e46302a42be9e938c15593a0015), closing the previous remote transport gap. Camera jump to distant captured AI death moved the host from 1194,40,2723 to 180,6,121 (r84a145486edb46f38c0abadc2eddae1f). Actual entity absence in a remote replication bubble was not measured; independence is established by the value-only protocol/no entity lookup, not a claimed culling test. No load benchmark or live ring-wrap stress test was run.

Final reload fec861642dcd46ef8c215aa0a9b2ae94 passed without script errors; existing resource diagnostics remain. runtime-default-v7.png shows Everyone initially selected before the final width reduction.

Final 476px panel capture: Art/References/spectator-events/runtime-narrow-peer-v8.png, connected peer PID 36688 at 2026-10-29 21:21:33 UTC. Five full rows, default Everyone on the left, You second and right-edge alignment are visible. Named-action open/release were separated across client frames.
Independent code and final live visual review approved; no blocking findings.

## Spectator scroll ownership

2026-10-29: visible damage-log, options and player/voice drawer regions own cursor scrolling. Lobby2's camera adds a cursor component (priority 98) that sets the native isCursorEnabled flag before movement; stock move-to-cursor already honors this flag. Its zoom subclass honors the same flag and clears pending zoom interpolation. Panel widgets continue receiving normal scrolling. Hidden panels release the region; world scrolling resumes. No shared stock-class override or network traffic is introduced. The custom camera preserves the base zoom lookup contract for stock hint/UI consumers.

Installed game source catalogue 3e84dd3e4386408097aaf18a7a8b6372: data007.pak/scripts/Game/Camera/Components/SCR_EditorCursorManualCameraComponent.c lines 11-16 establishes the cursor flag; SCR_MoveToCursorManualCameraComponent.c lines 11-34 consumes it. SCR_ZoomManualCameraComponent.c lines 55-90 independently reads ManualCameraZoom, requiring the scoped subclass. SCR_ManualCamera.c lines 200-207 uses exact component types. Configs/System/ActionContext/ManualCameraContext.conf lines 5-44 and 536-555 bind lens zoom and cursor movement separately.

Successful Reload 6d321c65a6dc4d59aa826c22727cb9c6; no SCRIPT errors after live tests. Existing unrelated resource diagnostics remain. Optional GC_TestSpectatorScroll moves the native pointer, sets named zoom/move actions, and invokes the resolved production cursor/move/zoom components with a 70-degree sample. Both +1 and -1 inputs yielded FOV 70, movement 0 and cursor disabled over EventScroll (ra0c979b5eca64974912b2e5f4bba063d / rd452e1cba9604063930fe5b847aadf9b), SpectatorOptionsPanel (r4be9f9be8fe7472692ea4abd5ca5f24f / rccd6a46588e547e1a1c5f8355999e338), and VoiceChannelScroll (ra4560e0264ad41939b0ed184d9802567 / rc50b950c9dac4fd4940761dded0a9d7a). World input resumed FOV 63/77 and 0.752m movement (r60418678aa22435498b084fa2f553240 / r799b279a94e645f69e2636e0b175b8de). After closing the log, its header region no longer blocked input (r45f8c1a5b15e4a0e8ed4b92e92ebd976).

Twelve native AI damage entries provided scrollable content. Native EventScroll slider movement shifted EventRows Y from 981.047 to 464.031 while the viewport remained at 981.047 (r95e0b0bdade548708dc86cd38c456c39 / raef9f297166741c0bfc17746ada2744f). Actual panel capture: Art/References/spectator-events/runtime-scroll-v9.png. These checks exercise native pointer geometry, resolved production handlers and native scrollbar positioning, not physical mouse-wheel injection. No layout/style changes were made.
Independent source and live UI review approved with no blocking findings.

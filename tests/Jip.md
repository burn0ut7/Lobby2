# Join in progress

Configure **Join in progress** on `GC_LobbyGameMode`:

- **MANUAL** (default): one invitation; double-press J within 400 ms to request the move.
- **AUTO**: request once after the owner's initial character finishes loading. If there is no safe target, the error explains how to retry with double-J.
- **DISABLED**: no invitations or moves.
- **Faction keys**: empty permits every faction; otherwise only listed keys qualify.
- **Deny seconds**: zero disables the cutoff. A positive value closes JIP at that many seconds of native GAME elapsed time, including outstanding invitations.

The minimum join time is a non-editable 120 seconds of GAME. Connection time is captured on authority before identity audit/character loading. A player who connects at 90 seconds stays ineligible even if loading finishes after 120 seconds. Lobby and briefing time do not count.

Mission-lifetime records use the native account identity, not a slot or player ID. Reconnecting, changing slots, and respawning cannot create another invitation. A different account using the vacated slot receives its own eligibility check. Death or character replacement forfeits an unused invitation. Unconsciousness temporarily blocks a request without forfeiting it. Empty identities are rejected.

The owning controller submits intent with no target, player ID or coordinates. Authority rechecks phase, cutoff, account, initial character, faction and native group membership. It prefers the group's living player leader, then another living connected member (also used when the joining player is leader). AI-only members do not qualify. The owner receives only its expected character ID and server-selected coordinates, so the target need not exist in its replication bubble.

The moving player must be on foot. Destinations use collision-checked dry terrain within ten metres of the target. Targets more than three metres above/below terrain, moving vehicles above 2 m/s, water and obstructed landings are rejected. Full and partly empty vehicles follow the same rule: land beside the vehicle, without taking seats or moving/evicting occupants. No empty vehicle is selected independently of a group player. A rejected preflight preserves the use; dispatch consumes it before transport, and authority observes the resulting position. Uncertain delivery does not grant a second teleport.

## Verification — 2026-11-01

Workbench 1.8.0.13, existing Arland_Lobby world, Lobby2McpTests loaded. Final Reload operation `4e80b6e7fc014bafbf49a732f52abbad` succeeded with no script compilation errors. Existing stale Arland resource references, BlockListApi leak and DedicatedServerPlugin diagnostics remain unrelated fixture issues.

Explicit fixtures in `GC_JipTests.c` use the real local native identity, production admission/request handlers and controlled characters. The timing fixture sets native elapsed time and captured connection time; these are boundary tests, not claims of a naturally timed late network join.

Verified:

- 90-second admission rejected; registering the same account later remained rejected; 120 seconds accepted.
- Exact cutoff and DISABLED rejected; empty faction list allowed US/USSR and a US-only list rejected USSR.
- Manual invitation rendered in white using the stock popup, translated project strings and no input capture. Native reference: `game/data/data010.pak/UI/layouts/Common/PopupUI.layout`; script `SCR_PopupNotification.c:282`. MCP capture: `Art/References/jip/manual-offer.png` (local artifact).
- Native action values DOWN/UP/DOWN/UP triggered the production double-press handler and returned NoTarget without consuming eligibility. A single press did not issue a request.
- Auto used the ready-owner request path and returned NoTarget, retaining manual retry eligibility.
- Review found and fixed phase-arrival ordering: presentation now waits for GAME as well as character/loading readiness. A production response delivered during BRIEFING remained queued and rendered after native advancement to GAME (`r00c1bb1d2dd747bba9f71f16689a1093`, popup visible with translated Unavailable text). The owner teleport also rejects a character that died or became unconscious after dispatch. Transport was repeated after these changes and again confirmed arrival.
- Changing from slot 12 to 13 consumed the previous character's offer; a subsequent request returned Unavailable, with no new invitation.
- Native M2 tripod compartment entry reported occupied; JIP returned LeaveVehicle and remained unconsumed. Receipt `r889f024f63ed4097995a0c338c72540a`.
- Transport-only fixture invoked the production bound-character owner RPC and arrival observer: requested `<1211.55, 39.01, 2719.77>`, authority observed `<1211.55, 39, 2719.77>`, result Arrived. Receipts `r7706b4692c16478dadb9baced86aa643`, `r858be5af7acd4604a98a382f023de2ff`, `r5344c248a797490f93de07f5f2b2dfc0`. This separately validates transport, not group-target selection.

Multiplayer attempt: enabled native Server localhost and launched the existing peer through MCP. The engine connected then rejected it for duplicate identity (`workbench/logs_2026-11-01_17-45-51/console.log:24475`; peer session `logs_2026-11-01_18-41-22`, disconnect group 6/reason 2). No identity or authorization bypass was added. The peer was stopped and Server localhost restored to Off.

Remaining coverage requires two distinct native player identities: distant/unstreamed leader, leader-self fallback to another player, successful request consumption/replay, reconnect and slot reuse by a different account, dedicated-server transport, and full/partly empty vehicle target landing. Moving/airborne vehicle rejection and terrain safety were source-reviewed but not exercised with a multiplayer target. Unrelated-mode baseline was not rerun: additions extend only the Lobby2-owned mode/controller and add an inert named input action to IngameContext.

For follow-up, `GC_TestJip("peer-offer", playerId)` prepares the timing fixture; the existing test-only `GC_TestPeerCommand(playerId, uniqueToken, "jip", "")` invokes the real owning-client request. Inspect authority position, `peer-snapshot`, and remote `scope-snapshot` before/after; distance alone does not prove the target was unstreamed. Tests run only when explicitly invoked and must not ship with production Lobby2.

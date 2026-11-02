# Join in progress

Configure **Join in progress** on `GC_LobbyGameMode`:

- **MANUAL** (default): one invitation; double-press J within 400 ms to request the move.
- **AUTO**: request once after the owner's initial character finishes loading. If there is no safe target, the error explains how to retry with double-J.
- **DISABLED**: no invitations or moves.
- **Faction keys**: empty permits every faction; otherwise only listed keys qualify.
- **Deny seconds**: zero disables the cutoff. A positive value closes JIP at that many seconds of native GAME elapsed time, including outstanding invitations.

The minimum join time is a non-editable 120 seconds of GAME. Connection time is captured on authority before identity audit/character loading. A player who connects at 90 seconds stays ineligible even if loading finishes after 120 seconds. Lobby and briefing time do not count.

Mission-lifetime records use the native account identity, not a slot or player ID. Reconnecting, changing slots, and respawning cannot create another invitation. A different account using the vacated slot receives its own eligibility check. Death or character replacement forfeits an unused invitation. Unconsciousness temporarily blocks a request without forfeiting it. Empty identities are rejected.

The owning controller submits intent with no target, player ID or coordinates. Authority rechecks phase, cutoff, account, initial character, faction and native group membership. It prefers the group's living player leader, then another living connected member (also used when the joining player is leader). AI-only members do not qualify. On foot, the owner receives its expected character ID, server-selected coordinates and target stance; vehicle moves preload the server-selected destination before boarding. Neither path requires the client to discover the group target.

The moving player must be on foot. On-foot targets use collision-checked dry terrain directly behind their body-facing direction: try three metres, then four or five metres if needed. A floor result shifted more than 25 cm horizontally from that rear-axis candidate is rejected, so there is no sideways/front fallback. Targets more than three metres above/below terrain, water and obstructed rear landings are rejected. Authority snapshots the target's standing/crouched/prone stance with the destination; the owner applies the native stance-change command after teleport without locking subsequent player stance input. For a target inside a vehicle, authority selects an accessible, unlocked, unoccupied, unreserved seat (cargo first, then pilot/turret), including child compartments. No available seat displays a retry message and leaves JIP unused. No empty vehicle is selected independently of a group player.

Authority reserves the selected seat and temporarily suppresses destination-node stream-out for that connection. The owner waits for that replication node, then authority rechecks eligibility, group target, vehicle and seat before authorizing native forced compartment entry. The source character is never moved just to load a distant vehicle. Preload failure releases the reservation and permits retry. While a move is in progress, duplicate requests are blocked. Authority confirms actual vehicle/seat occupancy or the ground position before marking JIP consumed. A client cannot claim success or reopen eligibility after execution was authorized; an unconfirmed dispatched move remains pending instead of granting another teleport. The normal mission cutoff and character-lifetime restrictions still apply to retries.

Release rechecks native Game Master ownership for each exact vehicle/child replication node, including editor activation, editable registration and skipped entity types. If GM still requires suppression, its own deactivation releases it. This coordinates the native vehicle streaming writer; arbitrary third-party mods writing the same boolean are not a general shared-ownership protocol. Native map markers use separate marker nodes. The connection's respawn observer is left untouched.

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

Remaining coverage requires two distinct native player identities: distant/unstreamed leader and vehicle preload, leader-self fallback to another player, reconnect and slot reuse by a different account, dedicated-server transport, and concurrent seat races between remote players. Moving/airborne vehicle boarding and terrain safety were source-reviewed but not exercised with a multiplayer target. Unrelated-mode baseline was not rerun: additions extend only the Lobby2-owned mode/controller and add an inert named input action to IngameContext.

For follow-up, `GC_TestJip("peer-offer", playerId)` prepares the timing fixture; the existing test-only `GC_TestPeerCommand(playerId, uniqueToken, "jip", "")` invokes the real owning-client request. Inspect authority position, `peer-snapshot`, and remote `scope-snapshot` before/after; distance alone does not prove the target was unstreamed. Tests run only when explicitly invoked and must not ship with production Lobby2.

## Vehicle retry follow-up — 2026-11-01

Reload `c0e1de4a347e472aaf80dada35e273eb` compiled without script errors. `GC_JipVehicleTests.c` creates a native M1025 with five AI occupants. Its explicit transport-only subclass substitutes AI for a second authenticated group player; production seat selection, reservation, owner RPCs, native entry and authoritative completion remain exercised. This is not a multiplayer/group-selection test.

- Five occupied seats: two consecutive attempts returned VehicleFull, `consumed=0 pending=0`. Receipts `rf8da3bc9bfc144ba9fced60773debd31`, `r491e2717611d4f86a364d96d5cbcdedb`.
- After a native exit, four occupants and a free seat were observed (`rfe889def808b4161b834edbd34667353`). Retry became pending (`r10d85db11e3e4700883c4f5e70b121e0`); the next observation confirmed boarding, Arrived and `consumed=1 pending=0` (`r68fd62c2cd044ba7816307289da7eaca`).
- A subsequent production JIP request returned Unavailable (`r550694bd1e6d4567a9af9f51c273adc5`).
- Native popup capture `Art/References/jip/vehicle-full.png` shows the translated white message directing the player to double-press J to try again in a little while.
- Native evidence at catalogue revision `721cd7b05eb54d3ca3591d9097af381d`: `SCR_CompartmentAccessComponent.c` owner-side `GetInVehicle`; `ExtBaseCompartmentSlot.c` native reservations; `SCR_DynamicSimulationEditorComponent.c` and `SCR_MapMarkerManagerComponent.c` per-connection stream-out suppression. Remote preload remains unverified for the account limitation above.
- Review identified and fixed release overwriting GM suppression. Final Reload `4297bbe509a648aabad46d7d9367b1b4` compiled without script errors. Native editor open/close exercised the production ownership predicate: closed `keepsVehicle=0` (`re2cf93a90ee04027aa07085b0a09bb3a`), opened `keepsVehicle=1` (`rfce83ec2184947b2b34e1cecfb15f6e5`), closed again `keepsVehicle=0` (`r7855e70917b643db8bc1fbf62ea01413`). This verifies the predicate, not remote network retention.


## Behind-target stance follow-up � 2026-11-01

Reload `4d92f77a767945c9b8c6a109ca3a2d08` compiled without script errors. `GC_JipBehindTests.c` is an explicit AI geometry/transport fixture using the production destination helper and bound-character owner RPC; it does not simulate group admission or remote replication.

- Actual landings were approximately three metres behind with less than 1 mm sideways displacement. Target/player native stance matched prone (2: `rf073ad5711714302b3086e87e3085498`), crouch (1: `r829b603b667545529e27534ecb7c4d5a`) and stand (0: `rd601110ae27e4ccbae2396b5acb108a0`) after native stance transitions.
- Geometry checks at yaw 0/90/180/270 all returned rear distance -3 m with negligible lateral error: `rd4ac0b5deb6d4c5ebb199632638bc373`, `r6052b92257c2472796a3203c50bd83f4`, `r461ac51063424842bbd3d39c19da87de`, `rc80f8db6f0904bd6bf29350424aa4057`.
- Installed source evidence: `SCR_EmptyPositionHelper.c:20` collision/floor search and `SCR_AIUtils.c:101` native stance conversion; catalogue revision remains `721cd7b05eb54d3ca3591d9097af381d`. Restricted rear-axis filtering is source-reviewed; obstructed rear positions and remote delivery remain unexercised.

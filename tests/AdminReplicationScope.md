# Admin scope and replication range

Admin Tools scopes describe the server audience. They must not be derived from
the characters or groups currently streamed into the requesting client.

## Source audit

| Interaction | Target and execution ownership | Review result |
|---|---|---|
| Player picker | Native connected-player list; replicated account identity validates selection | Does not require a character. Late identity updates now refresh row identity without rebuilding rows on unrelated voice events. |
| Everyone heal | Server `GetPlayers`, server character/slot resolution | Already independent of requester streaming. Unavailable characters are reported as skipped; KIA slots use the existing respawn path. |
| Selected heal | Player IDs plus account identities; validate the entire selection on server | Already independent of requester streaming; stale targets reject before healing. |
| Nearby heal/repair | Server character position and server radius query | Intentionally limited by radius, not client visibility. |
| Faction heal | Current native faction membership on server | Fixed: client slot/group-derived audience and exact-array comparison removed. Includes native members without slots. |
| Individual teleport | Server resolves both characters and landing position; moving owner receives coordinates; server checks arrival | No destination entity required on the moving client. Dead/unspawned and seated source restrictions remain. |
| Faction teleport | Server resolves current faction and destination, plans landing positions, dispatches to owners, verifies arrivals | Fixed: request carries faction and destination identity, not client-derived member arrays. |
| Faction message | Native faction membership on server; reliable owner delivery | Fixed: no longer restricted to Lobby2 slot occupants. Results expose dispatched/skipped counts; delivery receipt is separate evidence. |
| Everyone/selected/admin messages | Server connection/identity/admin-role selection | No target character required. |
| Kick/ban session list | Server live roster merged into session history | Hardened: requesting the list refreshes every connected player even if a join event was missed. |
| Kick/ban/unban | Server validates current identity; native kick and ban service | No target character required. Persistent ban operations require a dedicated server; listen-server testing cannot prove them. |
| Slot assignment, freeing, removal, respawn | Slot ID/claim and identity requests; server validates and resolves entities | Living-slot Heal eligibility no longer requires a client entity proxy. Client slot lookup also re-resolves proxies after streaming in. |
| Voice mute/priority | Identity-bound requests, server voice assignment and connection checks | No target character required. Player-row context may carry a null entity. |
| Freeze and phase controls | Authoritative mission state and owning-client delivery | No client target-entity enumeration. |
| Spectator follow/map/entity context | Local replicated objects for camera/presentation | Local availability remains an intentional limitation; these are not server-wide audience selectors. |

All mutation entry points retain the active-mode, authority, connected-requester
and current native admin checks, with central action logging. No production
permission override or globally replicated character/position cache was added.

Installed source check: `SCR_FactionManager.GetPlayerFaction` reads native
player-faction records without resolving a character (current MCP PAK catalogue
`4942ab6194e34c74bd5972e7761785e2`, `game/data/data007.pak`,
`scripts/Game/GameMode/FactionManager/SCR_FactionManager.c:236`).

## Acceptance procedure

Use the optional test addon on Everon (`worlds/examples/eden/Eden_Lobby.ent`),
with a separate diagnostic game process. Embedded Play may host the server but
must not stand in for the admin client. A small world or distance alone is not
proof of streaming absence.

1. Spawn two players and move the target far away using a disposable fixture.
2. On the separate client, call `GC_TestAdminScopeSnapshot(targetId)` through
   the correlated `scope-snapshot` peer command. Require `server=0`, `listed=1`,
   `identityReady=1`, and all of `character`, `slotEntity`, `replicationItem` zero.
   Check the authority still has the target. Recheck before each action that
   depends on absence, especially after teleport or respawn.
3. Use real authenticated client requests. Damage fixtures only prepare state;
   they do not implement healing. Verify target health on authority and owner,
   action results, and matching central audit records. Dispatch is not a pass.
4. Exercise Everyone, Selected and Faction heal; KIA recovery; native faction
   membership without a slot; selected and faction teleport in both directions;
   messages and moderation selection. Check the actual client picker and UI.
5. Repeat unauthorized and stale-identity requests, requiring no mutation.
   Test ban/list/unban on a disposable dedicated server and verify cleanup.

`GC_TestAdminScopeMove` moves only the local character onto terrain;
`GC_TestAdminScopeDamage` damages one living server character.
`GC_TestAdminModerationRecovery` removes a disposable presentation-history row
then exercises the production live-roster refresh. None grants admin roles.
`scope-request` uses production controller requests, and `scope-picker` exercises
the production roster/selection component. Peer observations are explicitly
client-reported and correlated by unique tokens.

## Evidence, 2026-10-24

Workbench 1.8.0.13, test addon loaded. Full script reload succeeded after the
changes (operation `17e4ee3c00af409c930c7e77b12cd368`). Evidence is retained in the
MCP workspace at `.temp/local/evidence/admin-scope-20261024/`.

Separate diagnostic process 50212 connected as player 2 to embedded server
18144. Host target at `(4500, 107.722, 8000)`, client at approximately
`(10353.2, 32.74, 362.652)`. Peer token `scope-absent-inland` returned
`server=0 connected=2 listed=1 identityReady=1 character=0 slotEntity=0
replicationItem=0 faction=USSR` (receipt
`r7f12a98e74fa420cb4071f0b524d7f67`). The authority retained both characters.

Host-issued Everyone and Faction heals restored the distant client's health
from 0.5 to 1; its own observations reported zero damaged zones. Requests 2 and
3 logged two healed players and zero skips. These establish server execution
and owner state, **not authenticated remote-admin request coverage**.

The unauthenticated client issued faction heal request 1. Authority logged
`rejected-not-logged-in`; health remained 0.5. Native-roster moderation recovery
passed for player 2 (receipt `rc7f59df66ead4bc8a950e2be8171ad26`).

### Authenticated standalone-client results

The user explicitly authorized temporary native session-admin roles for the
isolated peers. Production permission checks were unchanged. The first peer's
role was revoked before restart (`r41bf9520c1cb4bfb8fc5289db5b83343`); the second
peer's role was revoked after testing (`rd3979289fee54ea0b3d84a6036fdd9c9`,
`changed=1 queriedAdmin=0`).

| Separate-client request/check | Observed result and evidence |
|---|---|
| Everyone heal, request 2 | Absent target's health 0.5 → 1; authority receipt `r2c853dcfc8874d7dbaaa6e2d1eb5dc6e`. Client absence before/after in `scope-admin-absent1` and `scope-after-everyone-absent`. |
| Faction heal, request 3 | Same absent target 0.5 → 1; `r3089deab42554cc6b1252d6d8b4a89cc`. Audience supplied by server, no client member array. |
| Selected heal, request 4 | Absent selected target 0.5 → 1; `rdaae6fdcdeb146c68d89c6be62c50e2b`; `scope-before-selected` confirms absence. |
| Rendered picker and Apply button | In the second client (PID 31288), the normal picker field and rendered row handlers selected target 1 with `character=0`. Normal `ToolApply` healed it; client result `r3c82e265da6d469ea9d4fb7f5075d9e7` reported one healed, zero skips. Authority `re6c460ae7f4f4ad89d20c74805e92e7a` confirmed full health. |
| Faction message | Host target received sender 2's message, `r512a34fca28b4730a83d599f6556620e`; client also displayed it. |
| Teleport to absent destination, request 6 | Client moved from the southeast to `(4499.14,107.6,7999.5)` near target `(4500,107.72,8000)`; `r12b7010395ff4788bb39eea5d9fe1096`, verified audit at 22:23:42.082. Target proxy and slot then resolved on client (`scope-teleport-arrived`), exercising stream-in recovery. |
| Faction teleport of absent member, request 7 | After moving target away and confirming absence (`scope-faction-away`), client brought faction member to itself. Authority position `(4499.14,107.724,8003.5)`, receipt `r438f48f8a179400a991938edce98043a`; verified audit at 22:24:06.683. |
| Individual bring of absent player | After respawn, absence was reconfirmed (`ui-respawned-absent`). Client `teleport-here` moved target to `(10352.4,32.6333,362.151)`; `r14ab8f34802645dbb54ad126084aa85e`, verified audit at 22:29:35.698. |
| Native member without a slot | With faction lock enabled, normal own-slot release retained native USSR affiliation. `rfd67e84e854946d4a16cd767cf5a0d20` confirmed inclusion; client faction message reached that unslotted member (`rd2b2719a864d452fb6abe5ab6107f0ff`). Unlocked release clears affiliation and correctly does not satisfy this fixture. |
| KIA recovery | Native USSR affiliation survived death. Client could see the replacement spectator body but not the distant corpse (`slotEntity=0 replicationItem=0`, `ui-kia-absence`). Client faction heal respawned the target near its distant corpse: living, zero damaged zones (`r6177c6992db94cc1afb1520de9166043`). Audit: one healed, one respawned, zero skips. The new character was again absent from the admin client. |
| Moderation row | Separate client received both players, clicked target 1's row and populated its identity, with Kick/Ban enabled (`rabb4c09cb633424b9f6deee54f58e4d0`). Concurrent absence receipt `r62a190b284664977896029e9e0bd1e86` has all three entity checks zero. Blank-reason Ban activation correctly stopped with a reason prompt. No persistent ban was submitted. |
| Stale identity | Client `heal-stale` request 3 logged `rejected-stale-selection`; target stayed at 0.5 (`rbdf56081ce5c40d1a93c3f355258c003`). |
| Role revoked | Client Everyone request 6 logged `rejected-not-logged-in` at 22:30:01.648. Target remained at 0.5 (`rfaf1bc4979174bca819f0872601beef4`). |

Latest full reload: `13390bf3e10645d3b26ad9e523c29a58`. Actual MCP captures:
[client selected player](../Art/References/admin-tools/replication-client-selected.png),
[client moderation](../Art/References/admin-tools/replication-client-moderation.png),
[faction summary](../Art/References/admin-faction-tools/replication-faction-summary.png).
The first two are standalone-client captures at a 1920×1080 client area; the
faction-summary capture is embedded UI only. The tests invoke rendered widget
handlers; physical mouse/gamepad navigation is not claimed. Long native
Steam/profile text clips in its table cell; the selected Arma UID remains usable.

Dedicated ban/list/unban persistence, reconnect races, large-server load and
physical input remain unverified. The unchanged native ban backend is not
runtime-validated by these listen-server tests. Initial peer launch failed to
resolve base `core`; adding the installed game addon root and nested test-addon
directory to the temporary Peer Tool paths resolved startup. No production
dependency or installed game file was changed.

Cleanup completed: temporary roles revoked, peers stopped with no surviving
processes, original Peer Tool paths restored, Play stopped and World Editor
closed (resetting the temporary localhost-server Play option). Reopened the
production Lobby2 project as PID 45268; native project inventory contained only
core, ArmaReforger and Lobby2. Production-only full reload succeeded:
`7ee0bc51ef4b4a359ab45ac6013856d7`. Independent review approved the implementation
and evidence; final diff/Enforce readability checks passed.

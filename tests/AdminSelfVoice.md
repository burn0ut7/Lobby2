# Admin lobby and spectator voice

Logged-in admins can select Global, any faction HQ, any registered group, and
any connected player's personal channel. The group context menu uses the same
server-authorized request as channel headers. Selecting voice does not change
the player's faction, native group, or slot. Normal soldiers retain their normal
radio behavior; this feature uses the lobby/spectator body.

## Optional MCP fixture

The `McpTestAddon` project is development-only. It depends on Lobby2, runs no
tests automatically, and grants no admin roles. Do not publish it.

1. Save authoring work, stop Play, and close Workbench normally through Reforger
   MCP. Ensure Lobby2 is discoverable in the configured addon search paths.
2. Open `tests/McpTestAddon/addon.gproj` with `workbench_open_project`. Confirm
   both Lobby2 and Lobby2McpTests with `workbench_project_context` and confirm
   `workbench_runtime_setup(action="status")` reports ready.
3. Open `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent` and start Play through MCP.
4. Use `workbench_runtime_prepare` and then its single-use token with
   `workbench_runtime_invoke`. Target entity `GameMode_Lobby3`, component
   `GC_LobbyManager`, and return type `string` for the methods below. Use the
   current Workbench PID. Inspect state and correlated logs after each request;
   a dispatch receipt does not establish acceptance.
5. Stop Play, close Workbench, reopen the parent `addon.gproj`, and verify the
   test addon is unloaded. Restore the original world and Play state.

| Method | Typed arguments | Result |
| --- | --- | --- |
| `GC_TestSelfVoiceTargets` | none | Current player/admin status, valid group IDs, ordinary permission result, assignment, native group/slot, and radio frequencies. |
| `GC_TestSelfVoiceJoin` | int channelType, string factionKey, int groupId, string ownerIdentity, bool contextMenu | Invokes the production context action when contextMenu is true; otherwise invokes the production channel request. |
| `GC_TestSelfVoiceReject` | int requesterId, int groupId | Calls the full authority handler with the actual local target identity and a registered group. Use an unresolved ID, or a connected non-admin requester in a suitable server fixture. |

Channel types are Global `0`, faction HQ `1`, group `2`, and personal `3`.
Use `-1` for an unused group ID and an empty string for unused faction/owner.
Obtain current group IDs and the personal owner identity from the snapshot;
IDs from an earlier Play session must not be reused.

## Checks

- Join groups through the context action despite an ordinary permission result
  of false; verify selected assignment and radio frequency, unchanged native
  group/slot, and that selecting the current group hides the action.
- Select both faction HQs. The chat transceiver must match the selected HQ;
  the faction transceiver must continue to follow native faction membership.
- Select personal and Global channels, including Global during briefing.
- Advance through production phase controls and repeat a group selection in
  GAME while spectating. Test a soldier separately to confirm normal radio use.
- Submit an invalid destination and an unresolved or connected non-admin
  requester. Require rejection, unchanged assignment/radio state, and an audit
  entry from `GC_AdminActionLogger`. Repeat after actual admin logout on a peer.
- Disconnect a personal-channel owner and confirm remaining members leave that
  removed room, including members with an admin-selected assignment.

## Recorded coverage

Embedded Workbench checks exercised production group actions and self requests
for both factions, personal and Global channels, briefing and GAME spectator
selection, invalid destination rejection, and unchanged native group/slot.
After correcting HQ routing, US and USSR selections tuned the chat transceiver
to frequencies 6 and 2 respectively while an unassigned admin's faction
transceiver stayed inactive. A typed full-handler request with requester `-1`
was rejected without changing assignment or frequencies; central audit entries
confirmed the outcome. A stale identity request also rejected without mutation.

These are embedded state/routing checks. They do not establish audible voice,
rendered context-menu hit testing, dedicated-server replication, real peer
login/logout, or personal-owner disconnect behavior. Workbench's local player
has implicit admin status; it cannot substitute for a logged-out connected peer.

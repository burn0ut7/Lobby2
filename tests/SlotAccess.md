# Bulk slot access

Source implementation is not yet runtime-validated: Reforger MCP tools became unavailable in this session. No successful compilation, multiplayer, JIP, screenshot or input result is claimed. Load the optional test addon using tests/README.md once the connection returns.

## Behavior

The game mode's GC_LobbyManager component exposes **Start lobby slots locked** (default on). The single padlock beside **Slots** in Lobby is visible only to authenticated admins. Closed means at least one slot is locked; click unlocks all factions. Open means all slots are open; click locks all. Occupants and reservations stay intact. Newly registered slots follow the last successful all-slot action, or the authored startup setting before that action. Group-specific/per-slot locks remain independent overrides for existing slots. Briefing has no bulk control.

Admin Tools > Slot reservations retains import/export and adds **Clear all reservations**, then **Confirm clear all** / **Cancel**. Only reservations awaiting initial join are cleared; connected players, reconnect holds and KIA records are preserved. This operation keeps lock state and is available only before GAME, following the existing reservation rules. Admins may prepare reservations while slots remain locked for normal selection.

## Required runtime checks

1. Reload through MCP; retain fresh correlated compiler output. Start a clean scenario with the default setting: every authored slot locked, non-admin selection rejected. Restart with the setting off: every slot unlocked. Check an unrelated mode still uses its own behavior.
2. Authenticate using the real admin flow. Capture Lobby with one 48px target and stock padlock next to Slots; confirm no added control on Briefing. Check mouse and native keyboard/controller focus at 1920x1080 and 1280x720. Non-admin should see the normal heading with no button.
3. On authority and each inspected peer call `GC_TestSlotAccessRemember()`. Click `AllSlotLocksButton` through the existing generic test widget helper, or use `GC_TestAdminGroupLock(0, false)` for the production owning-controller RPC. Wait for correlated result and central `slot-group-lock` audit. Run `GC_TestSlotAccessCheck("locks", false)` on each peer; repeat for true. Do not treat dispatch as passing. Confirm direct slot row icons refresh, mixed locks offer unlock-all, and queued UI events do not rebuild all cards.
4. Prepare empty, occupied, offline initial-reservation, reconnect-held and KIA slots. Check lock/unlock preserves their claims/entities. Add a slot after each bulk action and check its lock policy. Join a new client after each action and compare slot locks against authority. Normal network latency remains; the complete lock set is applied within one reliable message per client, not at an identical wall-clock instant on all machines.
5. Reserve a locked slot by context action and by import. It stays locked and reserved. Unlocking leaves the claim intact. Reconnect and first-join assignment retain their existing behavior.
6. Remember state on each peer. Open existing reservation tools; first click `ReservationClearAll` must mutate nothing; Cancel must mutate nothing. Confirm clears only pending first-join reservations. Run `GC_TestSlotAccessCheck("clear", false)` and inspect central `slot-reservation operation=clear-all` audit and result count. Repeat with zero reservations. GAME rejects a forged clear request without mutation.
7. Call `GC_TestSlotAccessReject(realLoggedOutId)` on authority, then with unresolved ID. Require unchanged slots/policy and both central rejection logs. Also dispatch actual logged-out owner RPCs. Log out after opening the UI and verify hiding/disabling plus server rejection.
8. Reopen menus repeatedly; confirm no duplicate requests/listeners. Inspect pending and failure feedback, confirm/cancel focus, permission loss and server response ordering. Never replay an uncertain mutation blindly.

Helpers only snapshot and assert production state. They do not grant admin rights or implement alternate mutation logic. New snapshot/assertion helpers are uncompiled until MCP reload succeeds.

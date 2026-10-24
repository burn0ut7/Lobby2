# Self-service unslotting

Clicking your occupied slot releases it during Slotting, Briefing or Game.
Faction locking restricts the next faction choice, not whether you can release
your role. Native group membership retains your side while factions are locked.
KIA claims remain unavailable for self-service changes.

## MCP regression procedure

Load the optional test addon using `tests/README.md`. Use a disposable session
with an ordinary occupied slot and available slots on both sides. Through the
authenticated admin request, enable faction locking and disable faction balance
so the opposite-side rejection specifically exercises the faction lock. Do not
grant the testing player a faction-unlock exception.

Invoke `GC_TestLockedFactionUnslot(int sameSideSlotId, int otherSideSlotId)` on
`GC_LobbyManager` through the generic runtime prepare/invoke interface. Run once
in Slotting and once in Briefing with fresh sessions. The helper exercises the
production authority handler and verifies claim release, retained native faction,
cross-side rejection, same-side reselection and direct same-side switching. It
ends on the original slot after success; a failure stops without cleanup/replay.

Also test the actual slot button on a connected client: click its occupied slot,
observe the empty role and retained faction, select another available role on the
same side, and verify the server and other client agree. Check KIA
claims remain unchanged, occupied/locked targets reject selection, and unlocked
Slotting still permits changing sides. During Game, verify unslotting transfers
control to the spectator before freeing the role, preserves the match entity,
and lets another eligible player claim it. Capture MCP screenshots and correlate
the server's slot-request logs with resulting state. Handler tests alone do not
establish mouse interaction or replication correctness.

## Current evidence

Source and whitespace review only. Reforger MCP was unavailable in the editing
session, so compilation, execution of this helper, actual button interaction,
active-match spectator transitions, replication and screenshots remain unverified.

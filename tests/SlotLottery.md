# Slot lotteries

## Deep-module extraction - 2026-11-09

`GC_SlotLotteryRuntime` owns draw state, scheduling, cancellation and snapshot
mechanics. Manager authorization, canonical slot assignment, RPC signatures and
snapshot field order remain in place. The optional `GC_DeepModuleTests.c` calls
production handlers without granting permissions or shortening deadlines.

In embedded Arland Play, the real 30-second draw assigned the host and native
group membership was verified (`r1da67e9d3c8b43d2ab0720c908565d43`). Overlap,
direct-slot-claim, duplicate-entry and stale-request guards passed. Purging an
affected group cancelled its draw and left the slot locked/unclaimed
(`rdbf96360fa864965a4921751f0871149`); a phase transition cancelled all draws
(`rcdda03efc79e4d77abe5c4b480c43afc`). A subsequent snapshot after their deadlines
showed no active draws or claims (`rc007975ab5c244b6a7aa78ec17f29c2a`). Both an
unresolved requester and the actually logged-out host were denied without
mutation. Audit output includes the accepted draw and rejected requests in
`logs_2026-11-09_21-18-15/console.log`, lines 20956 onward.

Multiplayer/JIP replay was not rerun in this pass. The installed Workbench 1.9
MCP profile reports multiplayer operations unsupported. Source comparison of
the unchanged transport contracts is not a live peer verification. No shared
base-game override or player-facing UI was changed by this extraction.

Final Reload `5e8bc5c3eacf4408bcbb7cbba6ba5306` succeeded with no compilation
errors. The log retains Arland resource/menu diagnostics and engine
`PacejkaPlotTool`/`BlockListApi` leak messages; this was not a clean engine log.
Static comparison confirmed all 12 manager attributes/fields and 16 RPC
declarations unchanged. Independent implementation review approved the changes.

An authenticated admin right-clicks a faction, group or empty slot and chooses
**Start lottery (30 seconds)** during SLOTTING or BRIEFING. Each start creates an
independent draw. Faction/group starts use one entrant pool across their currently
empty slots. Occupants, first-join reservations, reconnect claims and KIA roles
are excluded. Overlapping active draws are rejected.

The existing admin announcement identifies the scope and explains **/join**.
Its first line shows the faction display name, then group and role when
applicable, separated by dashes. It omits scope, timer and random-assignment
explanations. A blank line
separates these details from entry instructions. Authored lottery notifications
preserve these line breaks; free-form admin popups retain their compact wrapping.
Typing `/join` in any native chat channel enters the newest still-open draw at
server execution time. Right-clicking any participating slot offers **Enter
lottery** or **Leave lottery** for that draw. Choosing a row never chooses the
winning role. Repeated entry does not add chances. The existing slot status area
shows the gameMechanics icon and **Lottery open**; no new menu, input mode or layout is added.

After 30 server seconds, players and available roles are sampled without
replacement. Connected identity, current slot state, KIA, faction locks and
faction balance are rechecked before the canonical assignment handler runs.
Admin-started draws deliberately admit locked empty slots without unlocking
normal self-service. Unawarded locks remain. Disconnected/ineligible entrants
are skipped; unused roles remain empty. Phase changes cancel pending draws;
teardown removes callbacks and state. Multiple independent entries are allowed:
a player who wins two draws moves to the later awarded role normally.

Live updates and joining-player snapshots carry the same draw IDs, timestamps,
slots and entrant identities. Requests derive their player from the owning
controller. Start authorization and draw results use `GC_AdminActionLogger`.
Limits are 64 simultaneous draws, 512 slots per draw and 256 entrants per draw.

## UI reference and acceptance

This is a narrow addition to existing context rows, status text and admin
notifications. Reuse the active references in
`Art/References/admin-notification/brief.md` and
`Art/References/slot-access/brief-v2.md`; no layout redesign was made. Retain the
existing charcoal surfaces, white labels, native gameMechanics icon and amber focus.

| Element | User need | Owner |
| --- | --- | --- |
| Start lottery context row | Select admin scope and duration | Server authorization |
| Enter/Leave context row | Change participation in this exact draw | Owner request, server pool |
| gameMechanics + Lottery open | Find participating slots | Replicated draw state |
| Existing admin popup | Announce scope, deadline and `/join` | Server-originated notification |
| Existing hint | Confirm entry/leave or explain rejection | Owner-only result |

Tests must establish actual delayed assignments, uniqueness within a draw,
independent overlapping-time draws, newest-open chat selection, shared scope,
duplicate/stale/unauthorized rejection, phase cancellation, replication and
teardown. Source inspection, dispatch and mockups do not substitute for runtime
state and MCP captures. Physical mouse/gamepad and constrained viewport coverage
must be reported separately.

## Test entry points

The optional test addon exposes `GC_TestLotterySnapshot`, `GC_TestLotteryContext`,
`GC_TestLotteryEntry`, `GC_TestLotteryFive`, `GC_TestLotteryGuards` and
`GC_TestLotteryPoolGuards`. The existing native chat fixture submits `/join`
through the real chat edit box and submit callback. The connected peer transport
adds `lottery-snapshot` and `lottery-start`; use unique receipt tokens and inspect
subsequent authoritative state. No fixture grants lottery permissions or runs
automatically. `ChatJoinCommand` in the addon-independent isolation probe checks
the inherited native slash-command callback outside Lobby2.

Raw local MCP evidence is retained in the server repository at
`.temp/local/evidence/lottery/`. See [the validation record](../Art/References/slot-lottery/validation.md) for the
observed runs and limitations.

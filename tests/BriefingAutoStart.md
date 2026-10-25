# Briefing automatic start

An accepted CO ready request evaluates every faction with a command slot, matching
the briefing tiles. All must have current readiness acknowledgements. Vacant,
disconnected and KIA command slots block automatic start; a faction with no
command slot is excluded. An empty set never starts a game. Topology changes
alone do not trigger start. Authenticated admin advancement remains independent
of readiness. Both paths use the same final spawning and native session start.

## Runtime checks

Use the optional addon and existing readiness/controller/peer fixtures described
in [README.md](README.md). Observe fresh snapshots and receipt-correlated logs
after each request; immediate dispatch is not a pass.

- Two occupied CO slots: first ready stays BRIEFING; final ready enters GAME
  on authority and peers and starts the normal session/spawn flow once.
- One vacant CO: ready the occupied CO; remain in BRIEFING. Claim the vacant
  CO and explicitly ready it; enter GAME. Lower occupied slots cannot substitute.
- Unready, disconnected, KIA or replaced CO: invalidate its acknowledgement;
  another CO's ready press must not start until all current COs acknowledge.
- Reject non-CO, stale-revision, stale-slot and out-of-phase ready requests
  without starting. Exercise simultaneous requests with fresh revision retry
  and duplicate requests after GAME without a second session start.
- Admin starts with unready/vacant COs through the existing controller request;
  verify normal session start and central audit. Logged-out/ordinary requests
  remain rejected with unchanged phase and a central rejection audit.
- Factions without a command slot do not block; zero command slots cannot start.
- Joining clients observe the authoritative phase and ordinary GAME presentation.

## Validation checkpoint — 2026-10-25

Source diff/readability checks passed. Installed `SCR_BaseGameMode.StartGameMode`
was inspected in source catalogue revision `bf051a869cde4dccb529ebc970249123`,
`data007.pak/scripts/Game/GameMode/SCR_BaseGameMode.c`, lines 494–529: authority
sets GAME, bumps replication and invokes the normal state event.

Runtime checks above are **blocked, not passed**. MCP native reload on PID 43968,
operation `0e9b073f58bc4397891fb04bf3b3ce0a`, returned `reload_failed`. Its fresh
diagnostics report undefined `GC_ResolveAdminToolTargets` and
`GC_SendPlayerToolResult`, plus base-game Tuple/map compilation errors.
Local Workbench log `logs_2026-10-25_15-08-49/console.log`, lines 426–483,
contains that attempt. The same target-resolution and Tuple errors occur in
lines 255–304 at 15:08, before this readiness edit at 16:50.

A fresh optional-test-addon process (PID 53264) reproduced the errors and reported
that World Editor could not initialize. Its log is
`logs_2026-10-25_16-51-27/console.log` under the local Workbench logs directory.
No Play session or multiplayer verification was possible. These failures remain
unresolved and must be repaired before feature acceptance.

Independent source review found no actionable defects and approved the code,
while independently confirming the pre-existing compiler failures. This is
source approval only; the runtime checks above remain outstanding.

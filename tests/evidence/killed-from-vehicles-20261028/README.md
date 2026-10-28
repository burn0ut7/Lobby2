# Killed From: self-deaths and vehicles

2026-10-28, Workbench 1.8.0.13, process 22836, Lobby2 MCP Tests,
Empty_Lobby, embedded listen host. All editor/runtime interactions and images
used Reforger MCP. No project switch or Workbench restart.

## Changes

Self-attributed damage creates one victim reconstruction. An accepted native
Respawn request captures that victim before ForceDeath and supersedes earlier
damage presentation; statistics/kill credit are unchanged. No invented shot
line is shown for self-deaths, terminal captures, or non-kinetic damage.

Mounted actors resolve their complete vehicle through BaseCompartmentSlot.GetVehicle,
and the line uses their occupied station's weapon manager. Turret replay captures
GetAimingRotation converted from degrees to radians: GetRawAimingRotation was zero
in the rotated mounted fixture. Seat entry completes before animation freezing.
Vehicle roots remain at the captured world matrix. Local previews are removed
from the editable-entity registry after initialization so ordinary spectator
target labels do not duplicate the killed-from labels.

Canvas line vertices now use ProjWorldToScreenNative, while FrameSlot labels use
ProjWorldToScreen without a second DPI conversion. The before image records the
misaligned projection, not the original unmodified implementation.

## Observations

- Final source/test Reload `e1970942c7034d6198ba8012203d08ca` completed. The log
  cycle begins at line 18452, 17:02:53.104 local, completion line 18568. No script
  compile errors. Existing resource GUID, dedicated plugin and world formation
  diagnostics persist; this is not a clean-engine-log claim.
- Mounted M1025/M2HB fixture capture `r694b4bcf0a354d578e8db6b4aafd7200` passes
  through the production damage tracker and JSON round-trip. Comparison
  `re2b0debe97954936b5c282289fef5754`: both actors and complete vehicle present,
  vehicle matrix error 0, evaluated turret aim error 0, muzzle error 9.53674e-7 m,
  cloned gunner occupies the seat. Captured yaw .349065 rad, pitch .119999 rad.
- Owner publication `r296c3d43edf1430ea8769c5f740731ba`; live source vehicle and
  victim moved away via `r81848fbcfb3249269ad5942d4c638959`. `vehicle-final.png`
  shows the retained scene and muzzle-to-hit line after that move.
- Native Respawn entry `r209786975dd0441d813b42bf860adcc8`, owner read
  `rcd2f17bb67b34c828677cfbba9479fed`: sequence 2, 13/13 parts, selfInflicted=1,
  shotLine=0, no killer, victim has 231 captured bones. `self-death-final.png`
  shows one labelled reconstruction beside the actual ragdoll.
- Normal admin assignment starts a new life and clears the old snapshot:
  `r4790f0f5bd8d4d44a4dc0e4b56252c21` reports snapshot=none.
- Native AI wound `reae88002687b42948cf463cd27099705` produced an external damage
  capture (console line 18975). Subsequent native Respawn
  `rf2d6d60307f44b03b8bc4f25de44e087` produced sequence 3, 13/13 parts, self=1,
  no attacker or line (`r59ac5dc8b62a4bf0a922418ccd8ebd69`).
- Direct self-damage after an external event and explicit respawn tracker tests
  produced self=1, line=0, killer=0, victim=1. Reads
  `r41293ac38bba4ff7b1fae0ea57545f53`, `r2848085a4a1d4ffc9a79507898a129bf`.

## Scope

The mounted fixture delivers explicit damage facts to the production tracker;
it is not a live ballistic fatal shot. This run does not establish dedicated
server, remote-owner, multi-turret, aircraft, explosion or vehicle collision
coverage. Snapshot pose and muzzle are observed at impact, not a recorded
historical firing-time transform. The line is straight, not ballistic replay.
Missing-map and loadout-manager messages also occur in this minimal test world.

Independent review found that empty-impact filtering also suppressed explicit
self-damage after an external hit. The guard now exempts self-attributed direct
events. Fresh Reload `241e0976a8db42b880c07bf3e7769d6a` completed at 17:06:24.734
(cycle lines 19092–19208), with no script compile errors. External capture
`r5b32d8adbdb74386bbc61ee40ecfbcc8` followed by self-damage with zero hit vectors
`r1cdab74f7f0640a1982b0f6818d9eae6` yields self=1, line=0, killer=0, victim=1,
frozen=1 (`r2415a57f52b24b8cabf69da98b9abb1b`). No UI changes followed the saved
final screenshots. Architecture guidance now describes the delivered policy.

Independent reviewer `review` approved the updated code and visual evidence with
the scope limits above. Native named-action toggle checks: down
`r600c8d2bd9194df28fbe0ceee66952db` disables the view and logs preview destruction;
release `r82279d4aa4194808a6c1e5572624434d` keeps it disabled. The next down
`r9f82c985118d4b6a8c0c8aec9e200782` re-enables it and release
`r87b07aa0d5a7463e9613ffe7125b7acb` preserves that state. Fixtures were cleaned;
Workbench, the current world, and test addon remain open. Saved-script readability
and `git diff --check` passed. Existing serialized names and compact getters were
preserved; the historical raw-aim field now explicitly stores evaluated radians.

Reproduce with GC_LobbyManager.GC_TestVehicleDeath: arrange, capture, self-empty, read,
publish, view, move-sources, self, respawn, cleanup. Use a fresh arrange after
respawn because the production tracker is terminal for that life. Existing
GC_TestPeerCommand action stats-respawn invokes the actual owner respawn entry;
action killed-from reads/toggles the owner view. Production has no test dependency.

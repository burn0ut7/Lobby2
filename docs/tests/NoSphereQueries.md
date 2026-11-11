# Sphere-query removal

User requirement: no QueryEntitiesBySphere. Production scripts contain zero
calls. Capture continues using named references and native player/AI registry
snapshots, with direct geometry checks; no substitute spatial enumeration was
added to capture.

The three existing callers now use stationary physics box traces:

- Spawner clearance: one first-hit trace per candidate with ENTS and the
  projectile layer mask. Authored clearance is a horizontal half-width and
  twice that height. No terrain collision is included; terrain placement has
  its separate sampler.
- Admin nearby vehicle repair: one on-request physics candidate trace, root
  deduplication, then the existing exact origin-distance check. The callback
  rejects hits to visit candidates instead of ending at the first collision.
- Admin crate selection: one 5m on-request physics candidate trace, followed
  by storage/replication eligibility and exact nearest-origin distance checks.

These helpers now discover collision-enabled candidates in the projectile
mask. Nonphysical custom vehicles/crates are not discovered. Neither admin
operation runs periodically. No new registry or global lifecycle override was
introduced; existing authorization and audit paths remain in place.

Installed source evidence: catalogue c52648a62fcb4229ad089610d060ea44,
core BaseWorld.c:225-230 TracePosition contract; stock
SCR_EntitySpawnerSlotComponent.c:145-158 uses an ENTS projectile-mask shape
trace for occupancy.

Workbench 1.9.0.37, existing project and embedded Arland Play, 2026-11-11:

- Compile passed; existing stale ArlandLobby resource diagnostics remain.
- 28 spawner checks passed, receipt r77ab908800de4ab6aa046bb767e85a61.
- GC_TestNoSphereHelpers passed all four result groups: nearby vehicle healed,
  out-of-range vehicle unchanged, nearest crate selected, unresolved requester
  rejected for heal and crate selection. Receipt r44d4b446fc26403380e8b03b91947850.
  Fixtures were removed and player position restored within the same call.
- 10 native vehicle/object capture, 42 core capture and 30 living/area checks
  passed before the final test-only additions.

Remote clients, actual logout/login transitions and addon-free mode switching
were not rerun. No new shared override lifecycle behavior was added. No
performance benchmark is claimed for the physics-query replacements.

Independent review approved. The unresolved-requester test booleans alone do
not establish authorization rejection (the crate controller has no controlled
entity). Correlated production audit evidence confirms rejection for this run:
`logs_2026-11-11_00-35-20/console.log` lines 25568 and 25590 both report
`rejected-not-logged-in` for heal and gear-select-crate respectively. Lines
25567 and 25569 report the authorized applied/completed outcomes.

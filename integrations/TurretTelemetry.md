# Turret direction telemetry

Schema 2 adds `turret-state` replacement snapshots for recorded vehicles and standalone `Turret` entities. The existing vehicle discovery policy applies: a tracked occupant's seat admits its vehicle/mounted gun; this does not scan the map for every unused vehicle. Once admitted, capture continues while empty. Spawn `kind` remains `vehicle` for both vehicles and standalone mounted guns.

Each record's `data` contains `entity`, `available`, and `turrets`. Each turret has:

- `managerId` (signed native int) and `slotId`: identify a compartment within this recorded entity's life. Never treat native IDs as globally persistent.
- `seatIndex` and `compartmentType`: join the existing player `seat` records using vehicle entity, compartment type, and index.
- `hasWeapon`: the native weapon manager currently lists at least one weapon. Observation turrets can exist without weapons; this flag does not imply ammunition or ability to fire.
- `directionAvailable` and, only when true, `direction`: a normalized world-space `[x,y,z]` vector from `TurretComponent.GetAimingDirectionWorld()`. This is turret aim, independent of hull orientation and gunner free-look.

For a top-down map, use X/Z and `atan2(x,z)` for a north-relative heading; adapt to the map's screen-axis convention. Do not add vehicle yaw: the vector is already in world space. Near-vertical aim has no useful map bearing. Elevation is retained in Y. Render separate gun indicators for entries with `hasWeapon`; observation-turret indicators are a separate presentation choice.

Replace the complete prior turret list on each snapshot. `available=true` with an empty array means no turret controllers were found in the native compartment list. `available=false` means the supported compartment manager is unavailable, not proof of an unarmed vehicle. Hide unavailable directions and remove indicators at despawn. Old recordings without this event have unknown turret coverage; do not substitute player head direction. On seeking, reconstruct the last snapshot at or before the cursor; do not interpolate across missing data, removed slots or entity lives.

Capture runs on the existing actor sampling cadence (nominally 5 Hz, subject to recorder budget), independently of hull movement. Changed six-decimal snapshots are emitted, with a five-second refresh when unchanged. The initial snapshot establishes presence. More than 64 turret entries stops recording explicitly. Existing byte/record quotas remain enforced. Snapshots use the L2TB JSON event section; numeric table layout and binary version are unchanged. Consumers must update to Lobby2.Contracts 1.1.3 before accepting the new record type. The browser decoder preserves it, but the playback map renderer must consume this event to draw indicators.

Source evidence: installed catalogue `c52648a62fcb4229ad089610d060ea44`, `game/data/data007.pak/scripts/Game/generated/Components/AimingComponent.c` lines 28–33; `generated/Turrets/TurretComponent.c` lines 12–19; `generated/Components/TurretControllerComponent.c` lines 50–51; `generated/Vehicle/ExtBaseCompartmentSlot.c` lines 46–49 and 73–75. These declarations establish the API; live tests establish the observed results below.

## Verification

Workbench 1.9.0.37: native BTR-70 and tripod M2 tests observed aim changes without hull changes, then world-space direction changes after rotating the hull. Unarmed M998 emits an empty available snapshot. Native manager IDs were negative; the validator intentionally accepts signed IDs. A rejected production telemetry request left capture running. Synthetic contract checks cover replacement/availability semantics, duplicate identity rejection, unit direction validation, limits, and preservation through the .NET and browser binary codecs.

Final recording `01a1c5c0-aabc-8f00-8400-00000007b002`: all 13 batches (959 records, zero drops) passed the real importer validator. Browser decoding verified BTR-70 (two native turret positions), armed UH-1H (two independent door guns), tripod M2 (one), and unarmed M998 (zero). Each produced nine snapshots. The three armed fixtures retained independent aim changes while hull heading was unchanged, then updated world aim at 90-degree hull yaw. All four emitted despawn records after fixture cleanup. These were empty native fixtures manipulated through the explicit test helper, not a remote player's gunner input. Both BTR positions reported weapons; their precise presentation roles should not be inferred from index alone.

Dedicated-server observation of remote-player turret movement, modded compartment arrangements, and the website's actual indicator rendering require separate coverage. No global gameplay overrides or network authority changes were introduced.

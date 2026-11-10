# AO limits

Place `Prefabs/Logic/AO/AOLimit.et` and author its closed polygon points. The prefab starts without points; it defaults to soft death after 20 seconds, inside permitted. A boundary needs at least three points before it can enforce. Invert to prohibit the interior. The native `Math2D.IsPointInPolygonXZ` ignores height; aircraft use their horizontal position. Use simple, non-self-intersecting polygons. Vehicle checks use the vehicle origin, not a hull collision test.

An AO enrolls each unit when it first enters that AO's permitted area during GAME:
inside a normal polygon, outside an inverted polygon. A unit starting elsewhere
is exempt until that first entry. After entry, leaving triggers the configured
hard return or soft countdown. Enrollment is per unit and per AO, survives admin
freeze and setting changes, and is separate from the live faction/item/vehicle
filters. An unenrolled unit receives neither an AO warning nor its boundary wall.
A replacement spawned unit is evaluated afresh. Setup zones retain their separate
start-inside-only enrollment; entering a setup area later does not enroll a unit.

## Client detection and authority

The owning client samples only its controlled body at 100 ms while enabled
boundary geometry exists in GAME. Polygon-side/proximity transitions and movement
of at least five metres within 15 metres of an edge notify authority. Geometry is
sent on configuration changes and owner readiness, not each sample. Hints contain
only the current body's replication ID: no client position, destination, deadline
or eligibility decision is accepted. Two bounded 250 ms follow-ups accommodate
movement replication arriving after a crossing hint; stationary clients do not
send a recurring heartbeat.

`GC_BoundaryRuntime` schedules targeted server validation, including native
occupants of the affected vehicle. Only unresolved violations receive 100 ms
server observation. An independent audit visits at most four roster entries per
250 ms slice, with at least eight seconds between cycles. Small populations sleep
between cycles; 128 entries take approximately eight seconds before scheduler
latency. No AO/setup system periodically evaluates the entire roster at 10 Hz.
Phase/configuration changes may reconcile the roster once. Native inventory and
compartment events, affiliation changes, possession and admin-freeze changes
refresh eligibility without waiting for a crossing.

Authority still owns enrollment, eligibility, grace clocks, damage and verified
return poses. Hard returns use the last server-observed safe pose, which may be
older than under the former full-roster sampler. A malicious or stalled client
can delay discovery until the audit. In particular, an initially exempt unit
that enters and leaves entirely between authority observations can remain
unenrolled if its reports are suppressed. Preserving first-entry enrollment
without continuous server spatial monitoring cannot prove that missed crossing.
This is a gameplay boundary with authoritative consequences, not a guarantee
against every manipulated movement/report stream.

## Workbench polygon authoring

Select one AO limit in an unlocked layer of the active subscene, then choose **Tools > Map / AO Polygon**. Set **Sides** (3–128), **Radius** (centre-to-vertex metres), and **Rotation** (0–360°), then press **Generate polygon**. Five sides and a 100 m radius make a regular pentagon with every vertex 100 m from the entity origin. Zero rotation places its first point along local +Z; positive rotation goes toward local +X. The entity's own transform applies afterward.

Generation replaces all point positions and per-point data, closes the shape, and is one Undo/Redo action. Use the normal Move/Rotate tools for the whole AO and the Vector Tool or `Points > Position` fields for individual vertices. Generation uses a flat local XZ plane. Save the world to keep the result. The generator exists only in the Workbench module and never runs in gameplay; it leaves normal native polyline data for the AO system.

The base prefab deliberately contains no inherited points: native removal of inherited object-array entries does not persist reliably. Keep mission geometry on the placed entity. For a custom prefab with inherited geometry, the tool refuses regeneration on an instance; edit the geometry's defining prefab instead. Previously placed instances relying entirely on the old default square need their points authored before use.

All eligibility conditions combine with AND. A condition that does not match exempts the player from **this limit only**. Faction Keys uses native player affiliation: an empty list applies to everyone, including players without a faction; a populated list applies only to those factions. There is no faction mode. Vehicle and item lists include derived prefabs. Inventory includes equipped items and nested managed storage, not vehicle cargo. HAS_ANY/HAS_ALL restrict item holders; HAS_NONE/MISSING_ANY restrict players missing the listed items. IGNORE disables a vehicle/item filter; enabled vehicle/item filters require populated lists. Driver means the vehicle's native pilot compartment; turret occupants are passengers.

All applicable limits combine as restrictions, not a union of safe areas. Two normal limits require the player to remain in their intersection. Priority is the only ranking setting: larger numbers win among breached applicable limits. Equal priorities are valid; hard return wins over soft kill, then the earliest soft deadline wins. Exact ties retain the first evaluated limit in the server's scan. Evaluation order is not an authoring control: use distinct priorities when a particular limit must win. A safe high-priority limit does not suppress another violated limit. Lower-priority soft clocks continue while suppressed; their full grace is not restarted by switching the displayed winner. Re-entry or exemption clears that limit's clock. For shared vehicles the highest-priority selected hard limit across affected occupants moves the whole vehicle once; soft death applies only to the affected character.

The trusted behavior API is `SetBehavior(behavior, graceSeconds, priority)`. The former Order argument and AO attribute are removed; external scripts must remove the fourth argument and custom AO resources should remove obsolete `m_iOrder` entries. Briefing order is unrelated and unchanged.

Hard return uses the moving body's last observed transform that satisfies all applicable hard limits for connected occupants. `GC_SetBoundaryTransform` uses native body movement with owner routing and linear/angular velocity reset, without Game Master teleport feedback or a black-screen fade. Characters keep their current facing direction. A named Return Entity can supply a safe transform for an on-foot character when no history exists (changed geometry or eligibility after enrollment). Author that anchor at a clear, supported location. Vehicles require their own observed safe pose; a generic marker is not a verified parking location. Every destination is checked against all applicable hard limits. Each active-violation sample corrects a current violation, including when continued movement crosses the edge again between samples; enforcement does not require an intervening inside sample. Unavailable destinations or rejected native moves are logged once per failure episode and reevaluated on later samples. Polygon enforcement is not a physical collision surface.

Limits apply to living, playing characters in Lobby2 GAME. Staging freeze takes precedence and suspends AO clocks and history. Death, possession changes, disconnect, phase exit, disabling/removal and mode shutdown clean up clocks/HUD. No shared base-game overrides are introduced.

Trusted server mission scripts can spawn/delete the prefab and call `SetEnabled`, `SetBehavior`, `SetPolygon` (world-space points, inversion, named return anchor), and `SetEligibility`. They return success/failure. Geometry/policy/filter changes restart that limit's countdown; these are trusted mission APIs, not client/admin request endpoints. Future admin UI must authenticate before calling them. Initial registration occurs after entity initialization; configure a newly spawned instance after that callback. Live runtime polygon changes do not edit the saved world or editor points.

The owner-only warning displays seconds and three milliseconds (`20.000`), adding minutes only when nonzero (`1:05.250`). It is input-transparent. Authority sends a deadline when warning state changes; local updates present smooth remaining time and never kill. This is a visual countdown, not millisecond-precision enforcement: actual expiry follows server evaluation and transport latency. The HUD closes at its deadline or on authoritative clear, death, possession loss or phase exit; it does not require heartbeat packets.

## Map shapes

The AO prefab includes `GC_MapShapeComponent`, disabled by default. Enable it to draw the AO's canonical polygon. Configure faction keys, outline and solid fill on that component; see [MapShapes.md](MapShapes.md). Map visibility and fill inversion are independent of AO eligibility and enforcement inversion. Empty map faction keys show everyone; copy the desired keys into the map component when restricting its audience.

The old AO map visibility attributes and `SetMapStyle` API are removed. Mission scripts now use the attached map component's `SetFactionKeys`, `SetStyle`, `SetInvertFill` and `SetEnabled`. AO geometry still changes through `GC_AOLimitEntity.SetPolygon`. Historical map test records below describe the earlier outline-only implementation.

## Validation

Priority-only regression (2026-10-25, embedded Arland): two fixtures plus the authored AO registered at priority 0. Equal-priority soft fixtures selected the earlier deadline; raising the other fixture to priority 20 changed the selected response. Equal-priority hard/soft and hard/hard fixtures returned the player after safe history was observed. Before arranging that history, switching from soft to hard while outside correctly reported no safe destination. Receipts and logs: `Art/References/ao-limit/priority-only.json`. No new standalone multiplayer coverage is claimed.

Hard-return regression (2026-10-25, embedded Arland): the optional `GC_TestAOContinuousWalk(seconds)` drives native `CharacterForward` during `OnPrepareControls`, automatically ending within 20 seconds. With a 2 m half-extent, the old evaluator issued one correction, logged `return unverified; no replay`, and allowed a 33.64 m forward offset after 12 seconds. The fixed evaluator held the same 12-second input at the edge (sampled offsets 1.97–2.16 m; final 1.97 m), handled a second 4-second attempt, and blocked 8 seconds against an inverted boundary. Three occupied M1025 crossings arranged through the native transform API also returned the vehicle each time. These are embedded movement/vehicle-return checks, not standalone multiplayer latency or sustained-driving coverage. See `Art/References/ao-limit/hard-return-{baseline,fixed,log}.json` for receipt-correlated results. The initial fixture lacks geometry and logs invalid configuration before successful setup; unrelated resource/loadout errors also remain in the session.

Use the optional McpTestAddon and named `GameMode_Lobby3` in Empty_Lobby. `GC_TestAOSpawn`, `GC_TestAOConfigure`, `GC_TestAOFilter`, `GC_TestAOToggle`, `GC_TestAOMove`, `GC_TestAOSnapshot` and `GC_TestAOTimeFormat` arrange native state and invoke production APIs. No tests run on loading and no permissions are granted. `GC_TestAOMove` arranges boundary crossings through the native editable transform; it does not prove movement-input behavior. See Art/References/ao-limit for concepts, raw MCP receipts, logs, screenshots and the final verification record. Pending verification must not be treated as acceptance.

For map checks use Arland_Lobby's `GameMode_Lobby2`; Empty_Lobby has no map entity. `GC_TestAOMapStyle`, `GC_TestAOMapSnapshot`, and `GC_TestAOMapView` exercise style, server filtering, projected commands and native pan/zoom. `GC_TestAOMapSurface` uses the inventory map gadget or existing spectator toggle handler. Peer transport action `ao-map` reports the owning client's rendered command state. See Art/References/ao-map for captures and receipts. Native edge panning can move an unattended map after centering; capture promptly after the helper's deferred pan.

Set **Warning Text** on each AO limit to replace RETURN TO AO. The selected warning uses that limit’s text, including when priority changes. Use a short heading for the compact HUD; blank text hides only the heading.

Boundary wall height is a local vertical band centered on the current view altitude. Polygon elevation and corner terrain heights do not anchor the visual. The horizontal 10-meter default range and fade still follow the character; climbing, descending and camera-height changes move the existing mesh vertically without rebuilding it. Terrain and other solid objects still occlude the wall normally.

## Client detection validation � 2026-11-10

Workbench 1.9.0.37, optional test addon, embedded authority/local owner in the
existing Arland example. Production attributes were compared with HEAD: all 13
AO, six setup, four wall, 12 manager, one freeze and two controller attributes
remain unchanged. No world/prefab authoring migration is required.

- With boundaries stopped through authenticated admin panic, counters remained
  at zero for samples, hints, audits and observations.
- A stationary enabled AO recorded 315 local samples, zero hints and four
  single-player audit observations, with no active fast callback. These are
  invocation counts, not an FPS or 128-player benchmark.
- Hard-return character and occupied M1025 crossings returned to a server-observed
  safe pose. Disabling client detection still allowed the audit to catch and
  return an already-enrolled violator.
- Soft exit started the authority clock/active observer; re-entry cleared both.
  Invalid body identity was rejected without entering the observer.
- Switching from driver to passenger outside a passenger-only AO started the
  clock with client detection disabled. Dropping both exemption field dressings
  while stationary outside a HAS_NONE AO also started the clock: zero local
  samples, zero hints and zero audits since reset, with remaining time 29.7674 s.
  Receipts: `a928afb252e04f27ad014c1369f1018c` (native removal),
  `1c7df6e1b5934d6384ca063e1dfa6690` (clock),
  `ed08fc5fc4db488a922723dfb34a9c3c` (observer counters).
- Setup hard return corrected a 40 m excursion. Freeze corrected a 5 m displacement;
  a ten-second extension rescheduled release. Expiry cleared movement restriction,
  HUD and detection polygons, leaving no active fast callback.

- Soft expiry killed the original character (`dead=1`) and normal lifecycle
  assigned a spectator body; the AO state and fast observer cleared. Evidence:
  Workbench session `logs_2026-11-09_21-18-15`, console line 23986, at
  `2026-11-10 01:04:21` local time. POSTGAME transitioned through the production
  authenticated handler and cleared all detection polygons and scheduled work.

Optional `GC_BoundaryDetectionTests.c` wraps production calls for counters and
can explicitly disable the local detector for audit/native-event tests. Existing
AO/setup/admin/native-compartment fixtures supply the rest of the scenarios.

The Workbench 1.9 native profile reports peer operations unsupported, so remote
JIP/packet-ordering, conflicting multi-player vehicle occupants and a 128-player
load run remain unverified. The shared global overrides were not changed in this
slice; an addon-free mode-switch comparison was not run. Existing Arland resource,
menu-preset, missing-loadout-manager and BlockListApi diagnostics remain.

## Wall lifecycle consolidation - 2026-11-10

The boundary runtime now routes AO and setup wall refreshes through one wall
registration lifecycle. Full and targeted wall refreshes share one audience
eligibility resolver; disconnect cleanup removes cached entries explicitly.
No attributes, RPC schemas, enforcement policies or rendering code changed.

Workbench 1.9.0.37 embedded Arland checks after reload
`fd42b6a8061c4c2cbdc55db4f5d78716`:

- The two authored walls remained registered. Spawning either fixture increased
  the count to three; deleting it returned the count to two.
- An enabled AO wall had audience 1; production ForgetPlayer reduced it to 0,
  and targeted Observe restored it to 1. A US-only filter excluded the USSR
  player and cleared the audience.
- The setup wall also passed audience 1 -> ForgetPlayer 0 -> Observe 1.
  A 40 m hard-boundary excursion returned the player to the original position.
- POSTGAME cleared the setup audience; deleting the fixture left zero detection
  polygons and active cases.

The explicit optional test helper exercises the production cleanup handler while
its player controller still exists. This proves cleanup does not depend on
controller destruction; it is not a remote-disconnect or replication test.
Remote peer coverage remains unavailable on the current Workbench profile.
Existing resource/menu diagnostics remain; no new compilation errors occurred.

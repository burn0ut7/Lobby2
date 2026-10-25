# Boundary performance review — 2026-10-25

## Result

The default small-wall case showed no clear frame-time regression in the local comparison. This is **not a blanket performance pass**: the review found avoidable work that scales with player count, map complexity, and concurrent map viewers. No production behavior or feature was changed. The only additions are optional test instrumentation and this evidence.

Reviewed production base: `c4b944c`. Other uncommitted admin-freeze work appeared during the review and was not changed or included in its conclusions. Measurements used the already compiled test session, not those later disk edits.

## Findings, ordered by importance

1. **AO enforcement scans all tracked players for every moving body, at 10 Hz.** `GC_AOLimitSystem.Evaluate` calls `EvaluateReturn` for every distinct mover; `IsSafeForMover` then scans the full player map to find that mover's occupants. This happens even with no applicable hard AO, including soft-only and disabled limits. With P separate on-foot players the occupant scan is O(P²), before applicable-rule checks: 100 players imply roughly 100,000 occupant checks/second; 200 imply 400,000. These are source-derived operation counts, not measured server milliseconds. Vehicle deduplication is good and reduces movers when players share vehicles. This is the strongest unnecessary server scaling concern.

2. **Complex inverted map fills have a measured CPU cost, and are rebuilt repeatedly.** `GC_MapShapeOverlay.Project` runs on pan, zoom, snapshots and size/readiness changes. Each projection calls `GC_MapShapeOutsideFill.Build` for each inverted fill. The actual builder took 2.23–2.44 ms per 128-point concave star on this machine, excluding projection, canvas submission and rendering. Several complex visible shapes can therefore consume meaningful map-update time. Pan and zoom callbacks can both project; unchanged snapshots also project twice a second. The builder scans edges across vertical slabs and sorts crossings, so geometry complexity matters substantially. Ordinary non-inverted fills do not use this builder.

3. **Open maps send complete geometry snapshots twice a second, even unchanged.** `GC_LobbyPlayerController_MapShapes.GC_RpcAsk_MapShapes` reconstructs arrays and JSON framing/label metadata per viewer, and sends a reliable owner RPC for remote viewers. Cost grows with visible zones × points × simultaneous viewers. A 128-point shape contains 1,536 bytes of raw XYZ float data before serialization, metadata and transport; this is a payload illustration, not a measured wire rate. Closed maps do not request snapshots. Source has a per-shape point bound but no combined shape-count/payload budget. This needs real multiplayer traffic measurement before a large-session pass.

4. **Moving walls replace native mesh resources.** The visual body is throttled to roughly 30 Hz, and horizontal movement must exceed 0.1 m before rebuilding. Each rebuild clips nearby edges, creates a new mesh resource and uploads vertices/indices; the private material and wall entity are reused. This is bounded, but resource churn and native/GPU work are not represented fully by script timers. The source explicitly records why in-place updates were unsuitable; this review does not propose reverting that behavior. The maximum-range stress sample was slower than the default-range sample; see limitations below. No long-duration memory/leak conclusion is justified by these short samples.

5. **Setup state and HUD repeat more work than their display needs.** Each active setup scans players every 100 ms and sends enrolled players reliable setup state, including the heading, every pass. Applying state refreshes the HUD immediately; its own 100 ms callback refreshes it again. Soft AO warnings also send every 100 ms while violated. This is redundant work worth investigating while preserving responsive enforcement and the intended countdown precision. Packet volume and remote-client cost were not measured.

6. **Secondary polling costs scale with population and zone count.** Wall authority refresh runs every 250 ms per wall, copies polygon data even when disabled, scans players when enabled, and compares audiences using repeated array membership searches. Applicable AO inventory filters can enumerate inventory repeatedly; default IGNORE avoids that traversal. Map overlay initialization schedules a 16 ms callback on each nondedicated Lobby2 controller instance, although nonlocal callbacks immediately return. Whether all remote controllers exist on each client depends on engine replication and needs observation. Local closed-map polling also continues. Announcement maintenance scans players every 500 ms even without an announcement to revoke lost admin map access. Admin rows refresh and remeasure each second while the tool is active. These are lower priorities than the first three findings, not demonstrated frame spikes.

7. **Admin map label placement has a separate shape-count scaling risk.** In `GC_MapShapeOverlay.Project`, each named label measures text and compares against prior labels, potentially repeating that prior-label scan once per existing label. The worst-case nested work is O(Z³) for Z colliding labels, independent of polygon point count. This is mainly relevant to admin show-all-zones maps with many nearby labels; ordinary unlabeled overlays skip it. It was source-reviewed, not stress-benchmarked.

## Existing safeguards that are working

- No custom fixed-frame callback was found in the reviewed AO/setup/wall/map paths. Server boundary enforcement is scheduled at 100 ms; wall audience refresh at 250 ms. Existing mission-freeze presentation uses a frame callback with early returns and separately throttled enforcement.
- Wall frame callbacks early-out when disabled/inapplicable; substantial visual work is gated to approximately 30 Hz. Distant walls do not create meshes, though eligible walls still inspect polygon edges for proximity.
- Only nearby wall sections are generated. Default range is 10 m. The maximum 60-edge buffer is bounded at 2,040 vertices and 1,920 triangles per wall.
- Stationary horizontal position avoids mesh rebuilding; altitude follows the view by moving the visual entity. The stationary test recorded zero rebuilds.
- Dedicated mode does not register wall FRAME rendering. Native meshes/materials are local visual resources, not replicated meshes.
- Wall replication is bumped only when geometry/audience changes; clear/delete paths release the local visual entity, material and mesh references.
- Control restrictions use the native control-preparation hook and restricted action values, without their own world/player scan.
- Map close clears projection resources/subscriptions; shutdown removes scheduled callbacks. Admin rows are reused by identity rather than continuously recreated.

## Live measurement method

Embedded Workbench Play, Arland, one authenticated US player in GAME; original AO disabled and original setup ended through the existing admin action. A runtime-only setup fixture was created through its public configuration API. The optional test addon wraps and calls the real production methods, counting calls and timing with native integer-millisecond `System.GetTickCount`. It does not substitute gameplay logic.

Hardware: AMD Ryzen 7 9800X3D (8 cores/16 threads), NVIDIA RTX 5090. This is high-end hardware. The map-fill microbenchmark uses a 2613 × 1408 canvas; no map was open during wall comparisons.

Each wall sample has approximately 10 seconds of probe data and 100 native `ProfileData` samples at 100 ms intervals. `ProfileData` frame/render/simulation fields are engine averages, **not individual frame times**; their mean is reported below. GPU time returned zero and is unavailable. Probes and profile samples start sequentially, so their windows are close but not identical. Timing quantization and test instrumentation overhead prevent interpreting tiny probe values as precise costs or zero as proof of no cost. Wall-frame timing includes nested mesh timing; do not add them together.

Prescribed movement uses the existing test fixture's native editable transform every 50 ms: X oscillates ±3 m, Z offset stays 45 m, near the north edge of a radius/half-extent 50 m zone. This is reproducible test motion, not normal player walking. The fixture origin changed when the 60-point polygon was spawned, so its scene is not controlled against the four-point scene. Default and maximum-range 60-point runs share a fixture, but were sequential and have no paired disabled baseline or randomized repeats; neither isolates GPU cost or proves causation.

| Case | Mean frame ms | Mean render ms | Mean simulation ms | Mesh rebuild calls / ~10 s | Mesh script total ms |
|---|---:|---:|---:|---:|---:|
| 4 points, moving, wall disabled | 3.219 | 1.867 | 0.962 | 0 | 0 |
| 4 points, moving, 10 m wall | 3.155 | 1.856 | 0.924 | 133 | 4 |
| 4 points, stationary, 10 m wall | 3.322 | 1.934 | 0.959 | 0 | 0 |
| 4 points, moving, disabled repeat | 3.123 | 1.848 | 0.907 | 0 | 0 |
| 60 points, moving, 10 m wall | 4.046 | 2.209 | 1.124 | 130 | 10 |
| 60 points, moving, 100 m wall | 5.899 | 3.318 | 1.446 | 127 | 46 |

The enabled four-point mean lies between disabled runs: there is no clear regression in that bounded case, and no evidence that enabling the wall improves performance. The 60-point reads confirmed 85 visible columns at 10 m and 1,020 at 100 m (all 60 edges), with visible=1, audience=1 and failed=0. Those state reads occur after the measurement window and are not every-frame geometry histories.

Outside-fill microbenchmark: one warmup, 200 builds per run, three runs of each geometry. The test uses a non-self-intersecting alternating-radius concave star and invokes the actual builder synchronously. This isolates geometry construction; it is **not a full map FPS test**.

| Points | Total ms for 200 builds, three runs | Per-build ms | Output triangles |
|---|---|---|---:|
| 4 | 2, 2, 2 | 0.010 | 12 |
| 60 | 113, 112, 113 | 0.560–0.565 | 598 |
| 128 | 446, 448, 488 | 2.230–2.440 | 2,048 |

## Coverage and remaining uncertainty

Source audit covered wall geometry/proximity, AO/setup enforcement and vehicle safety checks, setup control restrictions/HUD, map snapshots/projection/fill/framing, admin list polling and announcement maintenance. Runtime coverage is the single-player wall comparison and real fill-builder microbenchmark above. It does not establish dedicated-server capacity, remote-client/RPC costs, low-end hardware performance, many overlapping walls, full map-pan frame pacing, or long-duration memory stability. AO enrollment retains per-character entries without explicit pruning in the inspected code; lifetime growth across many respawns merits a long-session check, but this review has not demonstrated a memory leak.

To issue a large-mission performance pass, measure actual dedicated-server frame time and network traffic at representative player/zone counts, full map pan/zoom with many irregular fills, overlapping proximity walls on a lower-end client, and repeated spawn/map/zone lifecycle cycles with memory tracking. These are validation gaps, not reasons to remove features. Potential improvements should preserve server authority, live vanilla membership queries, warning precision, vehicle handling and visual behavior.

## Evidence and reproduction

- [Raw samples, probe reports and invocation receipts](evidence/boundary-performance-2026-10-25.json)
- [Optional bounded profiling hooks](McpTestAddon/scripts/Game/GC_BoundaryPerformanceTests.c)
- Load `tests/McpTestAddon/addon.gproj`, enter the normal Lobby2 GAME flow, and use the existing setup fixture helpers. Invoke `GC_TestBoundaryPerformanceStart(10)` and `GC_TestAdminProfileStart(100)` through the generic MCP runtime interface; wait for completion and read both reports. Probe indexes: 0 wall frame, 1 nested mesh, 2 map projection, 3 AO evaluator, 4 per-zone setup evaluator. The scene contained three wall components and two setup evaluators, including inactive originals.
- Invoke `GC_TestBoundaryFillBenchmark(points, 200)` for 4, 60 and 128 points, three times each.
- Test-session log review: initial probe compilation rejected array-accessor increment syntax; explicit assignments fixed it before the successful reload. A startup loadout-manager error and a shutdown leaked BlockListApi instance were also logged. Neither establishes a boundary mesh leak, and the short session is insufficient for memory clearance.
- Successful script reload: `8741a0703ce242b3a463a50c3f6eb419`; Play start: `c982419cb81844a6839d6e2eeac157a3`; Play stop confirmed: `a0b00fe4b7e64990b22cde8883e19878`.
- One move invocation was compiler-rejected because the requested return type was bool instead of string. No move ran from that rejected preparation; the corrected string call returned `accepted=1` before sampling. An oversized evidence-save request was rejected before execution; evidence was subsequently written in bounded chunks.

### Source locators

- `scripts/Game/AO/GC_AOLimitSystem.c`: Evaluate, EvaluateReturn, IsSafeForMover, EvaluatePlayer.
- `scripts/Game/AO/GC_AOLimitEntity.c`: AppliesTo, ObserveCharacter and enrollment storage.
- `scripts/Game/Setup/GC_BoundaryWallComponent.c`: RefreshAuthority, UpdateNearbyMesh, EOnFrame, Initialize, ClearMesh.
- `scripts/Game/Setup/GC_SetupZoneEntity.c`: Evaluate and Initialize.
- `scripts/Game/Setup/GC_SetupPlayerController.c`: GC_SetSetupZone, GC_ApplySetupState and GC_RefreshSetupHud.
- `scripts/Game/Player/GC_LobbyPlayerController_MapShapes.c`: OnInit, GC_UpdateMapShapes, GC_RpcAsk_MapShapes.
- `scripts/Game/Map/GC_MapShapeOverlay.c`: Update, Receive, Project, label placement and map callbacks.
- `scripts/Game/Map/GC_MapShapeOutsideFill.c`: Build.
- `scripts/Game/Admin/GC_BoundaryAdmin.c` and `scripts/Game/UI/GC_AdminBoundariesComponent.c`: announcement maintenance and boundary polling.

Independent review approved the corrected report and verified sample arithmetic. Play was stopped and normal Lobby2 reopened (PID 43968). Arland restoration was blocked by concurrent, unrelated admin-freeze compilation errors: GC_AdminControlFreeze.c:15 cannot resolve GC_ResolveAdminToolTargets; GC_AdminControlFreezeController.c:29 cannot resolve GC_SendPlayerToolResult. Those files were preserved untouched. World Editor could not open while that module failed; normal Workbench remains open with the test addon unloaded.

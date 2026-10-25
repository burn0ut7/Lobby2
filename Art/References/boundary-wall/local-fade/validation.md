# Local boundary fade validation

2026-10-25; Workbench 1.8.0.13; Lobby2McpTests embedded host, US slot 11.
Production base commit: 184e9b8. This change updates the existing boundary wall only.

## Result and captures
The wall clips each polygon edge to a horizontal 10 m player radius and fades its
ends using the existing vanilla VirtualArea texture. One entity/material is retained.
The mesh updates after more than 10 cm horizontal movement, at most about 30 times/s.
Camera turns and stationary frames do not rebuild it. No collision or per-frame RPC.

Arland authored setup polygon, same existing concept in ../concept-v1.png; this
revision intentionally removes distant perimeter visibility.
- near.png: player (1191,39,2689), heading 180; 17 columns, maximum radius 10 m.
- corner.png: move to (1227,39,2696), heading 90; both nearby edges, 34 columns.
- side.png: same position, heading 45; wall remains visible, update count stays 2.
- far.png: (1191,39,2721); visible=0, opacity=0; update count stays 2.
- reenter.png: (1191,39,2685); visible=1, update count becomes 3.
Floating point readbacks at Arland reach 10.0001 m. These captures are original MCP
window captures scaled to 1280 pixels, with the existing editor viewport/UI scale.

Empty_Lobby 60-point radius-50 m fixture:
- sixty-points.png: player offset (0,0,45), heading 0; 60 polygon points,
  only 68 columns (four clipped edges), maximum radius 10 m.
- Two repeated moving runs reuse the same entity/material and replace only local mesh.
- Final stationary readbacks both report 319 mesh updates; no further rebuilding.
Receipts are in receipts.json; runtime-log.json correlates the final performance calls.
errors.json preserves unrelated project/resource/Steam errors; this is not an error-free
world claim. Production and test scripts compiled and executed in the final process 30112.

## Performance
RTX 5090, Empty_Lobby embedded host, same camera and scripted movement for all runs:
x = sin(ticks * .05) * 3, z = 45 relative to fixture center, every 50 ms for 240 ticks.
Each run samples native ProfileData 100 times at 100 ms intervals. Values are rolling
engine averages, not individual frame timings. All runs finished with remaining=0.

| Run | Wall | Mean frame ms | Mean render ms | Mean simulation ms |
| --- | --- | ---: | ---: | ---: |
| Off1 | disabled | 1.7392 | .9479 | .4876 |
| On1 | enabled | 1.8269 | .9997 | .5132 |
| Off2 | disabled | 1.7352 | .9619 | .4682 |
| On2 | enabled | 1.7258 | .9534 | .4699 |

Raw samples: perf-Off1.csv, perf-On1.csv, perf-Off2.csv, perf-On2.csv.
Paired mean frame differences: +.0877 ms, -.0094 ms. This short test finds no
consistent large slowdown; it does not establish zero cost, low-end performance,
worst-frame latency, or long-session memory behavior. GPU time returned zero and
is unavailable, not measured zero GPU cost.

## Native reuse and observed failures
Installed source catalogue 4942ab6194e34c74bd5972e7761785e2:
Core/generated/Resources/MeshObject.c and Game/Global/SCR_Shape.c.
SCR_Shape.CreateAreaMesh closes a whole polygon and cannot describe disconnected
clipped strips. This component uses the same native MeshObject API directly.
Vanilla texture {930D6120C56C73C9}Assets/Editor/VirtualArea/VirtualArea_01.edds
supplies the gradient; no replacement texture or shader.

In-place UpdateVerts experiments changed CPU bounds but left stale rendered
geometry/UVs when moving between edges in this Workbench build. Dynamic flags did
not produce reliable output. The delivered implementation creates the bounded
local mesh before SetObject, then sets its origin and calls Update; this refreshed
both pixels and world bounds in the corner/side tests. This is an observed behavior
of the tested build, not a proven engine-wide diagnosis.

The first 60-point fixture attempt configured after deferred startup and returned
false, leaving four points. That failed attempt remains in receipts.json; it is not
the sixty-points.png run. The helper now configures before deferred startup, and
the final readback explicitly reports points=60. A Play operation timed out while
an authorization dialog was open; the dialog was resolved, Workbench closed normally
and restarted before further testing. No uncertain mutation was replayed.

## Scope
No remote client/JIP, lower-end GPU, controller input, constrained viewport, long
resource soak, or new AO-specific lifecycle run in this revision. AO uses this same
component; its previous eligibility/default-off tests are in ../validation.md.
This change does not alter authority, eligibility, setup countdown or enforcement.
Terrain interpolation still uses authored polygon vertex heights.

Parent Lobby2 project restored to Arland_Lobby.ent, Play stopped (process 50412).

Independent review: approved, no blocking findings. The 10 cm update threshold
allows small spatial lag relative to the current player position. Snapshot opacity
is calculated proximity telemetry, not the material multiplier. Captures establish
sampled transitions; continuous-motion smoothness was not recorded.

# Boundary wall validation

2026-10-25, Workbench 1.8.0.13, embedded host, Lobby2McpTests with Lobby2.
World: `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent`. Native source capture
3840x2160, saved 1920x1080. Passive indication; no new input/focus handling.

Installed source catalogue: `4942ab6194e34c74bd5972e7761785e2`.
Inspected Game Master E_EditorRestrictionZoneBase.et, SCR_BaseAreaMeshComponent,
SCR_Shape.CreateAreaMesh, VirtualArea.emat / VirtualArea_01_Danger.emat,
Material native declarations and IEntity.SetObject lifetime. Native Material.Create
with the vanilla shader/texture parameters rendered successfully in an isolated
test before production implementation. That exploratory fixture was replaced by
tests that exercise the production component.

Native full reloads `50fa5bdd6005405eaa897705f63ea839` and
`70d8d63de7e7412eb9a8bc879ac537c9` succeeded. The latter includes deferred
initialization and authored size clamping. A subsequent ten-second setup smoke
test created the wall and removed it on release: see lifecycle-receipts.json and
released.png. Configure also retains its supplied Color object so configuration
before deferred initialization is preserved. Initial failed
reload `8e718a1d9810447786ecc9416769d34e` rejected stacked attributes; fixed to
Enforce's comma-separated attribute syntax. Raw test returns and correlated
RST_MCP_BEGIN/END log records are in receipts.json. Existing test-world loadout,
map, old resource and Dedicated Server Tool diagnostics remain; no boundary-wall
runtime errors were observed in that cycle.

Observed production results (original 15 m setting; final default below):

- Setup default enabled. US player in US zone enrolled, audience=1.
- At center 20 m from edge: no mesh initially; after return from edge, mesh hidden,
  opacity=0. See far.png.
- At 10 m: opacity .0904861; at 5 m: .258533; at 2 m: .33206.
  Values follow the smooth proximity fade, maximum configured opacity ~.35.
- near-red.png shows the translucent gradient at 5 m; near-blue.png shows an
  independent configured blue material at 2 m. Ground line follows the polygon.
- A five-second setup released: audience=0, mesh=0, visible=0.
- US player in USSR-only setup: audience=0, mesh=0.
- AO prefab default disabled: enabled=0, audience=0, mesh=0.
- Enabling AO produced audience=1, visible mesh, opacity=.258533; ao-enabled.png.
- Changing AO faction eligibility to USSR removed the wall; changing back to US
  restored it. Enforcement's existing AppliesTo remains the eligibility owner.

Concept comparison: native gradient preserves the clear boundary, see-through
terrain and untouched top-center setup timer. Native texture includes a thin
ground stripe; retained as part of the requested vanilla treatment. At very close
range the wall fills much of the view, as a world-space wall naturally does.

## Final 10 m default and performance follow-up

User requested 10 m and low performance cost. Final source compiled on startup in
Workbench PID 42352, session logs_2026-10-25_03-06-19; runtime setup reported ready
and ScriptsCompiled=true. Proximity/material work is capped at approximately 30 Hz,
maximum opacity is cached, and hidden/unchanged meshes are reused. Geometry and
audience replicate only on change; fading is local. No source changes followed
these final tests.

At nominal 10 m, measured opacity was 6.3e-13 (floating-point distance rounding,
visually transparent); at 5 m it was .174509. near-10m-default.png is the final
default appearance. A 60-point AO polygon rendered successfully at 5 m; see
near-60-points.png. At 20 m its snapshot was mesh=0, visible=0. Earlier 15 m
captures remain as historical color/lifecycle evidence.

Native ProfileData sampled 100 times at 100 ms intervals per run, with 1.5 s
settling after changes, same stationary player/camera and scene. These are native
rolling averages, not individual frame timings. One 60-point zone, 10 m range,
3 m height, NVIDIA GeForce RTX 5090, 3840x2160 Workbench window. Run order:

| State | Mean frame ms | Mean render ms | Mean simulation ms |
|---|---:|---:|---:|
| Off 1 | 1.8303 | 1.0099 | .5006 |
| Near 1 | 1.8365 | 1.0168 | .5036 |
| Off 2 | 1.8433 | 1.0153 | .5103 |
| Near 2 | 1.8655 | 1.0291 | .5153 |
| Far, enabled | 1.8718 | 1.0313 | .5109 |
| Off 3 | 1.8579 | 1.0236 | .5072 |

Paired near differences were .0062 and .0222 ms (~.34% and 1.21%). The far sample
was .0140 ms above its following off control; run-to-run drift is visible. This
shows a small observed cost in this scene, not a hardware-independent guarantee or
an isolated measurement of the component. GPUTimeMs returned zero, so GPU timing
is unavailable. Raw samples are wallPerf*.csv; performance-receipts.json retains
typed calls, results and correlated logs. No boundary-wall runtime errors appeared.

Limits: embedded host only; remote/JIP replication not exercised. No physical
gamepad/mouse interaction added. No constrained-resolution or dark-terrain capture,
long-duration/multi-zone or lower-end performance benchmark, or sloped-terrain
traversal verified. Vanilla SCR_Shape supports up to 60 polygon points; exceeding this
logs an explicit rendering error and leaves enforcement unchanged. Terrain is
sampled at authored vertices, matching the vanilla area-mesh approach; use enough
vertices to follow terrain changes. No shared vanilla class overrides were added.

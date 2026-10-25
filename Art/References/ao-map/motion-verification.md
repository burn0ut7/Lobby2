# Map motion correction — 2026-10-25

User reported jitter while moving around the map after 6283de9. The original renderer projected on an independent 16 ms call queue and allocated fresh line commands every update. The map's native pan and zoom advance in its own frame callback, so the two were not synchronized.

Production now subscribes to native GetOnMapPan/GetOnMapZoom while its canvas exists and updates immediately after the map changes its transform. It reuses line commands and vertex arrays across movement and equal-sized snapshots. Snapshot arrival also projects immediately. The existing timer only discovers the map, requests server data every 500 ms and clears stale drawings; it no longer drives projection. Native close unsubscribes and clears the overlay, with existing shutdown cleanup preserved. No shared base-game override was added to production.

Installed source revision 4942ab6194e34c74bd5972e7761785e2: SCR_MapEntity.SetPan updates pan and PosChange before its invoker, SetZoom invokes after zoom assignment, and EOnFrame performs native interpolation. SCR_MapRulerUI uses these same pan/zoom subscriptions with matching cleanup.

An explicit optional probe samples every polygon vertex after the native map's EOnFrame, comparing submitted command coordinates with current WorldToScreen output. It requests three native animated pan/zoom transitions over 600 frames and counts replaced command objects. It does not update production geometry itself. Both runs used Workbench 1.8.0.13, PID 8596, Lobby2 MCP Tests/Arland_Lobby, one four-vertex AO, preview map, identical probe/fixture:

| Result | Before | After |
|---|---:|---:|
| Frames sampled | 600 | 600 |
| Simulation seconds | 2.1673 | 2.25183 |
| Misaligned frames | 599 | 0 |
| Maximum coordinate error (pixels) | 45 | 0 |
| Missing-overlay frames | 0 | 0 |
| Drawing-command replacements | 123 | 0 |

Baseline receipt r35e881b309cb4d76b31edde98e28e2bc; corrected receipt r354d6d81b4f541cf91ad5a56d6c5bb74. Full native reload 0347458d5e344280995ffdd1396a928d passed. Raw calls are in motion-evidence.json; correlated logs in motion-receipts.log. User then confirmed the visuals were much better and asked to move on, so further visual testing stopped. This is coordinate/scheduling evidence and user visual confirmation, not a measured GPU/frame-time improvement or a new multiplayer benchmark. Existing concept and layout remain unchanged. The optional probe's shared map override calls super unconditionally and does nothing unless explicitly started in Lobby2.

Independent reviewer ao_motion_review approved with no blocking findings. Saved-source readability and diff checks passed. Test Play stopped and the optional test Workbench exited normally; production Lobby2 reopened in PID 11708, with only core, ArmaReforger and Lobby2 loaded. No peer settings changed during this correction.

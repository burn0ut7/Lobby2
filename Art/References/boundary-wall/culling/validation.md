# Boundary wall camera-culling regression

2026-10-25, Workbench 1.8.0.13, embedded host, optional Lobby2McpTests addon, Arland_Lobby.

Reproduced on the existing four-point authored setup polygon at player position (1191, 38.9687, 2689). Facing east (90 degrees), the original mesh vanished; south and west remained visible. Reported mesh bounds enclosed the complete polygon. Calling IEntity.Update once restored the missing wall without moving the player or camera. Later dynamic-bounds/link flag experiments were cumulative and are not independent evidence.

Production change: call Update immediately after attaching the mesh and setting visibility. Reload succeeded. A fresh Play session, with experimental mutation hooks removed, displayed the wall at headings 0, 90, 180 and 270 degrees, including returning to 90. Before and after east screenshots show the same player pose. Read-only bounds receipt reports opacity 0.138339 and flags 2051. Moving to (1191, 39, 2721), beyond the 10 m fade range, hides the wall.

The result supports stale scene registration after dynamic mesh attachment as the cause in this implementation; it does not establish a general vanilla engine defect. The production change runs only at mesh creation and preserves ordinary culling and the existing fade loop. No new performance benchmark, remote-client test, or arbitrary-polygon guarantee is claimed. Existing feature performance measurements are in the parent directory.

receipts.json includes diagnostic experiments and fresh-session calls; action parameters used during experiments were removed from the final read-only diagnostic. Screenshots were captured through Reforger MCP. The user-authored world layer was not modified or committed.

Reentry check also passed at the original east-facing pose after leaving the fade range (reenter-east.png). runtime-log.json records matching begin/end markers for the fresh-session receipts. The reload/runtime log contains unrelated world/resource/loadout errors; no boundary-wall script exception was observed. Independent read-only review approved the change and saved evidence.

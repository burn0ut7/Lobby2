# Compact setup HUD validation

2026-10-25, Workbench 1.8.0.13 embedded host, Lobby2McpTests / Empty_Lobby. Compiled and exercised production setup/AO handlers through generic MCP runtime calls. Receipts in receipts.json; screenshots are original MCP captures.

- Inside setup: single SETUP bar, 420x44 design units (previous height 90), no guidance text.
- Setup exit: RETURN TO SETUP uses existing AO layout and independent deadline.
- Both exits: both warning cards visible; physical rectangles at y104.296 and245.096, each130.37 high; 10.43 physical/8 design-unit gap. Same width as setup bar.
- AO reentry clears only AO warning; setup reentry clears setup warning. Both absent when inside.
- AO-only violation during setup occupies first warning row.
- Setup expiry leaves AO countdown running, moves AO back to original y70. Receipt r13d4bd0f9c104ee2b6760185b3eff8af, setup inactive r05adb5f3dd8046119809fbbed4a3d01b, AO remaining53.1177 r71333376e6db4d9bb2b434cce71d2d04. See released.png.
- Hard AO during active setup returned player from25m offset to origin (ra15fab48f55b436293368c43c41ce6e1 / r19f02990bafc4e24a2c198899bbf0648).
- Soft AO3sec during active setup: live SETUP1:52 / AO2.408, then controlled entity changed from US soldier to Character_Spectator and all three HUD roots absent (r647e3fa936b04ac6a6ab895abeccc0dd / rbb1de822c2d44dcb8603ebbe0ec327f5). Spectator alive=1 is not the original soldier surviving.

Visual comparison: native screenshots retain concept's one-line bar, same-width monochrome cards and clear separate titles. Measured44px bar is about half previous height; labels fit without wrapping or clipping at tested viewport.

Limits: embedded host only, no remote-client/dedicated-server or constrained-viewport test; keyboard/controller input is unaffected passive IgnoreCursor HUD. Shared admin STAGING layout compiled but its separate live flow was not rerun. Existing mode-owned classes only, no new global vanilla override. No performance benchmark needed for two existing-style timer cards; no wall mesh algorithm change.

Errors preserved in errors.json: initial fixture invalid configuration before configure, missing-map/loadout fixture messages, resource/plugin/platform warnings/errors, and a GUI unknown keyword 'name' at transition to spectator. No setup/AO HUD script exception observed; do not claim an error-free engine log.

Saved-code readability check: tabs, Allman blocks, method separators, tagged additions to modded classes, and existing external names preserved. Parent Lobby2 Arland world restored after tests, Play stopped. Bright gameplay background tested; dark background and alternate UI scale were not rerun.

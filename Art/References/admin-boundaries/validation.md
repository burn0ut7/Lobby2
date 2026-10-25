# Boundary admin validation — 2026-10-25

Implemented in the existing Admin Tools → Boundaries. Separate AO, setup, and announcement tabs use one row per zone, with complete faction membership. Optional prefab admin names identify zones; fallback names remain stable within the mission. Coordinates are omitted. The red panic button is inside Boundaries and remains visible across its three tabs.

## Observed behavior

Tests used the optional McpTestAddon through generic Reforger MCP runtime calls, exercising production controller requests and native widget handlers. Embedded Workbench host, version 1.8.0.13; Empty_Lobby and Arland_Lobby. Raw calls and returned state are in runtime-receipts.json.

- AO disable/re-enable affected the selected zone while another AO sharing its factions remained unchanged.
- Setup extension added 300 seconds to the selected zone. End released its player immediately and rejected subsequent extension. Freeze movement was released by panic.
- Panic disabled all registered AOs, permanently ended setups, cleared announcement and warning state, and left the player alive and mobile. Final rendered button receipt r853340d5b97046d7b8e41bb823932fd2; catalog r02f7a46adf204e2183cb411aea92d76a; HUD r8b379891573749fdbd6f8044247a000b.
- Invalid requester and real admin logout rejected mutations without changing active rules/timers. Central admin logs recorded successes and rejections.
- Announcement targeting included/excluded the local player according to native faction. Multi-selection accepted US and USSR together; selecting no faction was rejected. A short timer expired and removed its HUD. During announcement alone, native firing consumed ammunition and movement/freeze/setup restriction flags remained off.
- Setup, announcement, return-to-setup, and return-to-AO HUDs stacked without overlap (stacked-hud.png). A long announcement heading remained readable (long-announcement.png).
- Personal map toggle added named outlines, removed them when off, and excluded them after logout. Final map-final.png shows overlapping AO/setup centers with separated, dark-backed labels.
- Long zone names and long faction lists wrap completely and reserve sufficient row height. Final screenshots: ao-final.png, setup-final.png, announcement-final.png. No coordinate positions or sidebar panic button remain.

## Compilation, diagnostics, and design comparison

Final scripts reloaded successfully in PID 33640, operation bfd09c9fd5fd46ac80618f8bdf2f6212. Final runtime log cycle is logs_2026-10-25_13-40-22, reload at 13:43:26 local. final-logs.json retains the full error query and central boundary audit lines; no boundary script exceptions or boundary-layout load errors appeared in this cycle. Existing environment/world diagnostics include Dedicated Server plugin, Steam stats, world resource fields/loadout manager, mouse imageset, and stock Map/ToolMenu.layout unknown `name` field. These are not claimed fixed.

Earlier iterations caught and fixed malformed layout blocks/slot, unsupported text-shadow layout fields, JSON boolean typing, and native automatic-wrap height measurement. The final implementation explicitly wraps row text so native measured height matches the displayed lines. The optional map test helper uses native SetZoom, WorldToScreenCustom without current pan, and SetPan; the smooth centering helper drifted during these tests and was replaced within the test addon only.

The active concept is concept-v3.png, amended by the user's later instructions: panic inside Boundaries and no coordinates. Actual screenshots retain the simple tabs and rows, complete faction audiences, clear zone-specific actions, and existing dark/amber admin theme.

Saved scripts were checked for tabs, Allman blocks, method separators, creator tags, readable control flow, and accidental diff changes. DTO field names intentionally match the serialized JSON contract; inherited callbacks and RPC/reflection names are preserved. No shared vanilla class override was introduced.

Independent reviewer admin_boundary_final_review approved the code, final screenshots, and available runtime evidence with no blocking findings.

## Limits

This slice was tested on an embedded authenticated host, not a separate multiplayer client or dedicated server. Native widget handler calls verify actions and resulting state, not physical mouse/gamepad focus traversal. Screenshots use the existing 3840×2160 Workbench window with an embedded viewport, returned at 1600 pixels; a constrained viewport was not tested. Widget reuse and bounded refresh intervals were inspected, but no performance benchmark was run. Panic affects zones registered when it is pressed; it does not prohibit future zone creation. One display-only announcement can run at a time.

Runtime fixtures were not saved into Arland. The pre-existing saved Arland layer and resource database changes are excluded from this feature commit.

# AO limits

Place `Prefabs/Logic/AO/AOLimit.et` and author its closed polygon points. The prefab starts without points; it defaults to soft death after 20 seconds, inside permitted. A boundary needs at least three points before it can enforce. Invert to prohibit the interior. The native `Math2D.IsPointInPolygonXZ` ignores height; aircraft use their horizontal position. Use simple, non-self-intersecting polygons. Vehicle checks use the vehicle origin, not a hull collision test.

## Workbench polygon authoring

Select one AO limit in an unlocked layer of the active subscene, then choose **Tools > AO Polygon**. Set **Sides** (3–128), **Radius** (centre-to-vertex metres), and **Rotation** (0–360°), then press **Generate AO polygon**. Five sides and a 100 m radius make a regular pentagon with every vertex 100 m from the entity origin. Zero rotation places its first point along local +Z; positive rotation goes toward local +X. The entity's own transform applies afterward.

Generation replaces all point positions and per-point data, closes the shape, and is one Undo/Redo action. Use the normal Move/Rotate tools for the whole AO and the Vector Tool or `Points > Position` fields for individual vertices. Generation uses a flat local XZ plane. Save the world to keep the result. The generator exists only in the Workbench module and never runs in gameplay; it leaves normal native polyline data for the AO system.

The base prefab deliberately contains no inherited points: native removal of inherited object-array entries does not persist reliably. Keep mission geometry on the placed entity. For a custom prefab with inherited geometry, the tool refuses regeneration on an instance; edit the geometry's defining prefab instead. Previously placed instances relying entirely on the old default square need their points authored before use.

All eligibility conditions combine with AND. A condition that does not match exempts the player from **this limit only**. Factions use native player affiliation; vehicle and item lists include derived prefabs. Inventory includes equipped items and nested managed storage, not vehicle cargo. HAS_ANY/HAS_ALL restrict item holders; HAS_NONE/MISSING_ANY restrict players missing the listed items. IGNORE disables a filter. Non-ignored lists must be populated. Driver means the vehicle's native pilot compartment; turret occupants are passengers.

All applicable limits combine as restrictions, not a union of safe areas. Two normal limits require the player to remain in their intersection. Breached limits are ordered by larger Priority, then hard before soft, then earliest soft deadline, then smaller Order. Give each limit a unique Priority/Order pair; duplicate pairs are rejected with an error. A safe high-priority limit does not suppress another violated limit. Lower-priority soft clocks continue while suppressed; their full grace is not restarted by switching the displayed winner. Re-entry or exemption clears that limit's clock. For shared vehicles the highest-priority selected hard limit across affected occupants moves the whole vehicle once; soft death applies only to the affected character.

Hard return uses the moving body's last observed transform that satisfies all applicable hard limits for connected occupants. It uses native `SCR_EditableEntityComponent.SetTransform`, including owner routing and velocity reset. A named Return Entity can supply a safe transform when no history exists (spawn outside, newly enabled limit, changed eligibility). Author that anchor at a clear, supported location suitable for the vehicle types you permit. Every destination is checked against all applicable hard limits. Impossible configurations, missing native editable support, or an unverified correction are logged, with no repeated blind teleport. Polygon enforcement samples at 100 ms, not a physical collision surface.

Limits apply to living, playing characters in Lobby2 GAME. Staging freeze takes precedence and suspends AO clocks and history. Death, possession changes, disconnect, phase exit, disabling/removal and mode shutdown clean up clocks/HUD. No shared base-game overrides are introduced.

Trusted server mission scripts can spawn/delete the prefab and call `SetEnabled`, `SetBehavior`, `SetPolygon` (world-space points, inversion, named return anchor), and `SetEligibility`. They return success/failure. Geometry/policy/filter changes restart that limit's countdown; these are trusted mission APIs, not client/admin request endpoints. Future admin UI must authenticate before calling them. Initial registration occurs after entity initialization; configure a newly spawned instance after that callback. Live runtime polygon changes do not edit the saved world or editor points.

The owner-only warning displays seconds and three milliseconds (`20.000`), adding minutes only when nonzero (`1:05.250`). It is input-transparent. Authority supplies remaining time at 10 Hz; local updates interpolate for smooth display, never kill. This is a visual countdown, not millisecond-precision enforcement: actual expiry follows server evaluation and transport latency. Missing updates expire the HUD after 1.5 seconds; server enforcement remains authoritative.

## Map outlines

Each AO has a **Map outline** category. Visibility is independent of enforcement:

| Visibility | Recipients |
|---|---|
| EVERYONE | Every player opening a map |
| AFFECTED_FACTIONS | Players matching this AO's faction filter; IGNORE means everyone and EXCEPT_MATCHING excludes the listed factions |
| APPLICABLE_PLAYERS | Living controlled characters matching all faction, inventory and vehicle filters |
| HIDDEN | Nobody |

The matching-player option tests current character eligibility, including a spectator body if it matches; it does not wait for GAME or require a boundary violation. Use faction visibility for planning before characters have their mission inventory. Disabled, invalid and removed limits are never drawn.

Set color (including opacity), width (1–20 UI pixels), and optional faction color independently per AO. Faction color uses the first listed faction only for ONLY_MATCHING; other faction modes use the configured color. Width stays fixed while zooming and follows UI scaling. The Workbench line color is separate. All visible boundaries are drawn; enforcement priority does not hide lower-priority outlines.

The overlay uses CanvasWidget/LineDrawCommand, closes each polygon, clips to the existing map frame, and captures no input. It introduces no MapItem/MapLink boundary markers or external addon dependency. The server filters a fresh owner-only snapshot every 0.5 seconds while that player's map is open. Changes to eligibility, geometry, enablement or style appear on the next refresh. Stale snapshots clear after 1.5 seconds. Closing the map releases the overlay and stops requests.

Trusted server scripts can call `SetMapStyle(visibility, packedARGB, width, useFactionColor)`. It returns false for invalid visibility/width or missing authority/registration and does not restart violation timers.

## Validation

Use the optional McpTestAddon and named `GameMode_Lobby3` in Empty_Lobby. `GC_TestAOSpawn`, `GC_TestAOConfigure`, `GC_TestAOFilter`, `GC_TestAOToggle`, `GC_TestAOMove`, `GC_TestAOSnapshot` and `GC_TestAOTimeFormat` arrange native state and invoke production APIs. No tests run on loading and no permissions are granted. `GC_TestAOMove` arranges boundary crossings through the native editable transform; it does not prove movement-input behavior. See Art/References/ao-limit for concepts, raw MCP receipts, logs, screenshots and the final verification record. Pending verification must not be treated as acceptance.

For map checks use Arland_Lobby's `GameMode_Lobby2`; Empty_Lobby has no map entity. `GC_TestAOMapStyle`, `GC_TestAOMapSnapshot`, and `GC_TestAOMapView` exercise style, server filtering, projected commands and native pan/zoom. `GC_TestAOMapSurface` uses the inventory map gadget or existing spectator toggle handler. Peer transport action `ao-map` reports the owning client's rendered command state. See Art/References/ao-map for captures and receipts. Native edge panning can move an unattended map after centering; capture promptly after the helper's deferred pan.

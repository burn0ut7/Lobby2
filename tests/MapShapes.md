# Map shapes

Place `Prefabs/Logic/Map/MapShape.et`, then author its native closed polyline points. It draws only on the map and has no AO enforcement. Select it and use **Tools > Map / AO Polygon > Generate polygon** for equally spaced sides, centre-to-vertex radius and rotation. Native Vector/Move/Rotate tools edit the resulting points. The helper is included in this addon, runs only in Workbench and supports Undo/Redo. Save the world afterward.

Use a simple, non-self-intersecting polygon with 3–128 vertices, nonzero XZ area and no holes. Concave polygons work. The prefab starts without points so each placed instance owns its geometry.

## Settings

- **Enabled:** show or hide the whole shape.
- **Faction Keys:** empty shows everyone, including unaffiliated players; otherwise only players in the listed native factions see it. There is no visibility mode. Keys belong to the map component even when attached to an AO.
- **Outline:** color/opacity and width in UI pixels (1–20). Optional faction color uses the first listed faction; absent/unknown keys use the configured outline color. Zero outline opacity hides it when faction color is off.
- **Fill Enabled:** disable for outline only.
- **Fill Color:** solid color and opacity. Around 20% keeps map detail readable.
- **Invert Fill:** off fills inside the polygon; on fills the visible map outside it, leaving the interior clear. The same outline is retained. Each shape fills independently, so overlapping translucent shapes blend normally.

The AO prefab already includes this component, disabled by default. Enabling the map component does not enable AO enforcement. Its shape comes from the AO's canonical geometry; its faction keys and fill inversion are independent of the AO's eligibility and permitted side.

## Live mission API and JIP

Trusted server mission scripts can spawn/delete the prefab and call these methods after its deferred initialization has registered it:

- `SetEnabled(bool enabled)`
- `SetFactionKeys(array<string> factions)`
- `SetStyle(int lineColor, float width, bool factionColor, bool fillEnabled, int fillColor)` — colors are packed ARGB.
- `SetInvertFill(bool inverted)`
- `SetPolygon(array<vector> worldPoints)` — standalone shapes only. For an AO, call `GC_AOLimitEntity.SetPolygon` so enforcement and drawing share geometry.

Methods return success/failure. Arrays are copied. Runtime geometry overrides do not edit saved world points. Without an override, standalone shapes read their current native points and entity transform. These are mission APIs, not a new live placement menu or client-authorized admin endpoint.

The active Lobby2 mode owns the registry. Server-filtered, owner-only snapshots refresh every 0.5 seconds while a map is open, including for joining players. Enablement, faction keys, fill inversion, geometry, style and deletion propagate through the same snapshot. Missing updates clear drawing after 1.5 seconds. Closing the map releases its canvas/listeners; mode shutdown clears the registry.

Drawing uses native canvas commands, clips to the map and captures no input. Projection follows native pan/zoom events and preserves fractional pixel positions. The native line command uses a narrow transparent edge to smooth shallow angles; configured width controls the visible stroke, with a small native width allowance for that edge. Regular fill uses `PolygonDrawCommand`; inverted fill uses non-overlapping trapezoids in `TriMeshDrawCommand` because the native polygon command exposes no hole/inversion option. No marker system or external addon is required.

## Validation

See `Art/References/map-shapes/verification.md` and receipt-correlated evidence. Optional fixtures are in `tests/McpTestAddon`; production does not depend on them. `GC_TestMapShapeFillCoverage` compares production outside-fill mesh coverage to native polygon containment, including concavity, reversed winding, viewport intersections, fully offscreen/surrounding polygons and a 128-vertex shape.

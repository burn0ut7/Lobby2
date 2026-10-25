# Setup and AO boundary walls

`SetupZone.et` includes an enabled `GC_BoundaryWallComponent`.
`AOLimit.et` includes the same component with **Enabled** off by default.
Change its **Boundary wall** settings on the placed prefab:

- **Enabled**: display the wall for players affected by this zone.
- **Range**: horizontal distance at which the wall begins fading in; default 10 m.
- **Height**: height above terrain; default 3 m.
- **Color**: RGB tint and maximum opacity; default translucent red, alpha .35.

The wall follows the enforcement polygon, fades smoothly as the player approaches
any edge, and has no collision. It appears on both sides of the edge so a player
outside a soft boundary can find the way back. Setup release removes it. AO uses
its existing faction, item and vehicle eligibility and is suspended while the
player is under setup or admin freeze.

At 10 m or farther the default wall is invisible. Proximity and material updates
are capped at about 30 per second. Geometry is built once on first approach, then
reused; distant geometry is hidden. The server publishes eligibility/geometry
changes, not per-frame fade updates.

The server replicates eligible character identities and current canonical polygon;
each client creates its own vanilla VirtualArea mesh/material. Color changes do
not alter shared vanilla materials. No separate helper prefab is required.

The vanilla mesh builder supports **3–60 polygon vertices**. More vertices produce
an explicit log error and no wall; enforcement still runs. Terrain height is
sampled at polygon vertices, so place vertices at important slope changes.
Runtime mission scripts may call the component's server-only
`Configure(enabled, range, height, packedArgbColor)`; accepted ranges are 1–100 m
and .5–20 m. This changes appearance only.

See `Art/References/boundary-wall/validation.md` for captures and coverage limits.

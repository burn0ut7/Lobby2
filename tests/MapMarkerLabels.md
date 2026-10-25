# Persistent placed-marker descriptions

In an active `GC_LobbyGameMode`, native `PLACED_CUSTOM` and `PLACED_MILITARY`
markers retain their description across mouse-leave and map zoom layers. Owner
names and military details retain native hover/focus behavior. Their layout
space is reserved while hidden, so revealing them does not shift the marker.
Native text filtering, ownership, platform-icon preferences, marker visibility
restrictions, and other marker types remain with their existing owners.

The shared widget override queries the current game mode on each operation.
It introduces no global state, frame updates, timers, or resource overrides.
Placed marker types are identified by their native category, including restored
placed markers whose original player may no longer be present.

## Reproduce through MCP

Load the optional test project per [README](README.md). On Arland's
`GameMode_Lobby2`, invoke `SCR_MapMarkerManagerComponent.GC_TestMarkerLabels`
with `(action: string, military: bool)`. Enter Lobby, join an available slot,
and enter Briefing using the existing production navigation probes.

- `place`: inserts a private marker at the open map's center through the native manager.
- `hover`, `leave`, `focus`, `blur`: invoke native widget callbacks.
- `zoom-in`, `zoom-out`: use the map's native zoom bounds and animation.
- `snapshot`: reads text/owner visibility, opacity, and screen geometry. Read this
  in a later MCP call after layout has run; a mutation's immediate result is insufficient.
- `remove`: removes the private test marker through the native manager.
- `fixture` / `remove-fixture`: create/remove the native layout and configuration
  directly for the stock Isolation world's `IsolationMode`, which has no map.
- `near` / `far`: exercise native marker layer handling at layers 0 and 3.

No probe runs automatically or grants permissions. `GC_LabelTestMarker` is a
fixture subclass, not a modification of the replicated native marker class.

## Validation — 2026-10-25

Workbench 1.8.0.13, embedded host. Final scripts compiled using Reload in PID276;
the MCP test project remains loaded, with Arland open and Play stopped.

| Check | Observation |
| --- | --- |
| Normal placed marker at near/far native zoom | Description visible; owner opacity 0 while idle |
| Normal hover/leave and focus/blur | Description remains visible; owner opacity 1/0 |
| Normal settled hover/leave geometry | Icon `(1270.5,646.333)`, description `(1155.64,708.259)`, owner `(1261.79,740.852)` unchanged |
| Normal map close/reopen through Lobby/Briefing | Description visible and owner hidden on recreated widget |
| Military placed marker at far zoom | Description visible while idle; owner and military details hover-only |
| Military settled hover/leave geometry | Icon `(1270.5,646.333)`, description `(1155.64,719.341)`, owner `(1261.79,751.933)` unchanged |
| Addon-free stock fixture, both types | Description/owner hidden on leave, visible on hover; description hidden at far layer |
| Same stock fixture after leaving Lobby2, addon still loaded | Same native visibility and opacity as addon-free baseline |

Local original MCP captures and receipts are in `Art/References/marker-labels/`:
`final-idle.png`, `final-hover.png`, `military-idle.png`, `military-hover.png`,
and `runtime-evidence.json`. Initial captures are retained separately. The first
implementation still moved the description 2.607 pixels when the native details
row appeared; reserving that row eliminated the movement in the final checks.

The initial test harness incorrectly modded a replicated class and failed
compilation; changing the fixture to a subclass fixed that before baseline Play.
An initial military fixture used an invalid config ID; the recorded later
baseline uses native config 0 and a populated description. Receipt-correlated
final calls showed no marker script exceptions. Logs retain unrelated missing
loadout-manager/resource errors; addon-free reload also logged a BlockListApi leak.

Coverage is native callbacks and rendered widgets on an embedded Windows host,
not physical mouse input, remote-client transport, console UI scaling, or an
unrelated mode's full map journey. The stock isolation comparison uses the native
widget fixture because that world has no map. The platform icon was disabled by
the existing local setting; its enabled visual state was not tested.

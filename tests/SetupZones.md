# Setup zones

Place `Prefabs/Logic/Setup/SetupZone.et` in a Lobby2 world and author a closed,
non-self-intersecting polygon with at least three points. The prefab deliberately
has no inherited points. The existing **Map / AO Polygon** tool also accepts this
entity, or use native Vector-tool editing. Geometry is horizontal XZ; height is
ignored. No helper entity or AO-limit prefab is required.

Configure Duration (seconds from GAME start), Faction Keys (empty = everyone),
Behavior, Grace Seconds and Safe Start. Each zone has an independent duration.

- Soft permits crossing, shows a return/death countdown and kills after continuous
  time outside. Re-entry resets that countdown. Zone expiry releases the player
  even if the outside grace period has not expired.
- Hard returns the character or occupied vehicle to its last observed safe
  pose through native body movement, without Game Master teleport feedback or
  a black-screen fade. Characters keep their current facing direction.
- Freeze disables native movement controls and holds the moving body's position.
- Safe Start blocks character fire, grenades, melee and turret fire through the
  character/turret input-processing hooks, and animation-driven handheld flares
  through their vanilla animation gate. It is enabled by default.

Enrollment is captured when a living, playing character is first observed during
GAME. It must start inside and match the configured faction. Walking into a zone
later does not enroll it. A replacement character is evaluated afresh and receives
only the zone's remaining time, not a new timer. Players starting outside or in an
excluded faction are unaffected. Keep matching setup polygons non-overlapping:
one player belongs to the first qualifying zone evaluated, without AO priorities.
Vehicle return moves the whole vehicle, including other occupants.
Linear and angular velocity are cleared during correction. The vehicle needs its
own safe pose observed inside the area; boarding outside cannot reuse a character's
pose. If no safe pose exists, the return is reported unavailable instead of placing
the vehicle at an unverified location. This is a sampled position correction, not
a physical collision mesh; author compatible zones and keep their edges clear.

The setup countdown is owner-only, continuously visible as a compact top-center
bar. A soft violation shows an AO-style "RETURN TO SETUP" death countdown below
it. An active AO warning occupies its own row below any setup warning. State updates
arrive at 10 Hz; local time only presents the countdown. Death, changed possession,
phase exit, zone removal, release and mode shutdown end the local restriction/HUD.
Missing owner updates expire local presentation after 1.5 seconds. Authority
continues enforcement. This is not an anti-cheat boundary.

AO enforcement continues during setup; both return deadlines remain independent.
Admin freeze still suspends AO for its affected players. The AO entity and its
rules remain independent. Author compatible setup and AO polygons, especially
when either uses hard return.

## Admin mission freeze

The existing admin feature remains separate. Its request accepts optional selected
player IDs plus matching native account identities; omitted selection means the
existing everyone scope. Selection is checked atomically on the server after the
normal live admin authentication. Extend/end retain the current audience; start
and configure choose an audience. The audience affects combat, boundary handling,
countdown visibility and AO suspension. This is a scripting request contract;
this change does not add a new admin-menu picker.

Setup and admin freeze share the player-controller combat filter, not a shared
zone prefab or duplicated global combat override. No persistent native movement
flag is set: the movement-disable callback includes setup while preserving the
base callback when setup is inactive.
While both apply to one player, setup owns the boundary and countdown until its
release; admin combat restrictions still apply. This prevents competing returns
to two different staging areas. The remaining admin freeze applies afterward.

## Validation

Optional fixtures live in `tests/McpTestAddon/scripts/Game/GC_SetupZoneTests.c`.
Invoke them through generic MCP against GC_LobbyManager. They spawn the shipped
prefab and call its trusted Configure operation, arrange crossings through native
transforms, and inspect resulting state; they do not copy enforcement logic.
Existing continuous-walk and character/turret combat fixtures exercise native
input before the production hooks. Runtime-only fixture durations include elapsed
GAME time so each explicit test receives the requested remaining duration.

See `Art/References/setup-zone` for raw receipts, screenshots and validation notes.

Set **Display Text** on each setup zone to replace SETUP beside its countdown. Use a short heading for the compact HUD; blank text hides only the heading. RETURN TO SETUP is unchanged.

The proximity wall follows the local view altitude, including seated helicopter views. Its Height setting controls the visible vertical band; polygon point elevations do not need to match terrain. Horizontal proximity/fading and setup enrollment are unchanged.

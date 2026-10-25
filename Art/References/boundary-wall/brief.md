# Boundary wall visual target

Active concept: concept-v1.png, generated with the built-in image tool from the
actual setup-zone/freeze-hud.png MCP capture. The mockup is not runtime evidence.

Journey: enrolled player approaches the polygon edge, recognizes the translucent
wall, stays inside; wall disappears after release or when no longer applicable.
Passive world-space indication; no focus or input capture, all input devices.

| Element | Player question | Placement / visibility | Owner | Decision |
|---|---|---|---|---|
| Red gradient wall | Where is the boundary? | Actual polygon edge, nearby affected players only | Server eligibility, local rendering | Add |
| Existing setup timer | When can I leave? | Existing top-center HUD during setup | Setup zone | Keep unchanged |

Use vanilla Game Master VirtualArea texture / mesh generation. Red warning color,
translucency and a vertical gradient preserve terrain visibility. No extra text,
controls, glowing outline or screen tint. AO wall defaults off; setup defaults on.
Configurable range and height. Default range is 10 meters per the user's follow-up;
the wall is invisible at or beyond that distance and gains opacity nearer the edge.
Test per-wall native material color before adopting.

Prompt: Preserve the actual Workbench gameplay screenshot, gun, soldier and setup
HUD. Add only a translucent red world-space wall along a polygon edge about five
meters ahead, three meters tall, strongest at ground and fading upward. Vanilla
Game Master style; no new labels, widgets, hexes or outline. Passive indication.

Concept critique: the edge and countdown remain distinct and legible. The generated
wall has a harder ground stripe than desired; use the native texture rather than
authoring that stripe. Ground contact and terrain visibility are acceptance criteria.
Runtime checks: near/far visibility, scope, release cleanup, AO disabled default,
enabled AO appearance, color, actual viewport screenshot, no collision effects.

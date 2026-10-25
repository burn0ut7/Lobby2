# Map shape visual target

Active reference: concept-v1.png, generated with image_gen from the existing MCP briefing-map capture Art/References/ao-map/runtime-briefing-outlines.png. Proposed overlay, not game evidence. Prompt: preserve the briefing screen; blue outlined square and amber pentagon, uniform 20% solid fills, crisp outlines, readable terrain/grid, no added controls or labels.

Journey: open any existing mission map, read the visible areas, pan/zoom, close normally. Mouse/keyboard use remains owned by the map; drawing captures no input.

| Element | Purpose | Owner | Decision |
|---|---|---|---|
| Polygon outline | Locate the exact authored boundary | Server geometry/style, client projection | Reuse existing native map canvas |
| Uniform solid fill | Distinguish the area while preserving map detail through configurable opacity | Server style | Add native polygon drawing |
| Existing map and menus | Navigation, terrain and mission context | Stock map / existing Lobby2 UI | Preserve |
| New labels, legends, controls, patterns | Not required for this slice | None | Omit |

Critique: the concept keeps the map legible and makes both regions clear; no extra UI competes with mission information. Generated text/background changes are not implementation targets. The defining traits are crisp borders, flat translucent fill, clipping to the map, and motion synchronized with the native map. Runtime tests will include convex and concave shapes, changed colors/geometry, visibility, deletion, rejoin, and pan/zoom. Editor authoring uses the shared polygon helper; this slice adds no live placement menu, only trusted mission APIs.

User refinement: no visibility selector; faction keys alone select the audience. Invert Fill changes only the filled side of the existing outline, with no added player-facing controls. Outside tint must cover the map viewport without tinting the clear polygon interior. See runtime-inverted-concave.png.

Final line critique: the native transparent edge keeps a crisp core at normal widths and smooths shallow-angle steps. Mirrored-gradient and guide-line textures were rejected after comparison for weak or uneven strokes. Native 3840 host and 1280×720 peer captures verify the chosen composition; raw pixel crops are analysis details, not generated mockups.

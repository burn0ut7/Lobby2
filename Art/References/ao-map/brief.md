# AO map outlines

Task: a player opens the existing briefing, gameplay or spectator map, sees permitted AO boundaries, pans/zooms, then returns without any new input layer. Mission makers independently set visibility, color/opacity, optional first-included-faction color, and line width on each AO.

| Element | Player purpose | Owner | Decision |
|---|---|---|---|
| Closed unfilled outline | Locate the exact AO boundary without hiding terrain | Server polygon; UI projects it | Keep |
| Color and line width | Distinguish configured boundaries and make them legible | AO authoring settings | Keep |
| Vertex markers, labels, fills, new controls | No requested task | None | Omit |

Active concept: concept-v1.png, generated using built-in imagegen on 2026-10-25. It is an illustrative topographic background, not a live game capture; baseline live-map coverage was unavailable before implementation. Prompt: existing full-screen Reforger-style paper map with two unfilled closed AO boundaries, blue five-sided line at 3 px and black irregular line at 5 px; preserve terrain detail, no vertex markers/icons/fills/labels/new controls. Caption identifies illustrative concept. Critique: clear closure and independent line styles, minimal clutter; actual runtime geometry comes from the mission and will differ. The caption is reference-only, not production UI.

Implementation target: CanvasWidget with native LineDrawCommand, clipped inside the existing map frame and ignoring cursor input. World-space positions reproject during map pan/zoom; thickness uses UI pixels and DPI scaling. Everyone / affected faction filter / currently applicable player / hidden are per-limit choices; disabled and removed limits disappear. Server sends only recipient-visible outlines while the map is open. No legacy MapItem/MapLink boundary objects and no external drawing addon.

Acceptance: compare actual MCP captures to unfilled continuous lines; verify closed geometry, color/width, map clipping, panning/zoom, visibility and live state changes, close/reopen cleanup, owner RPC delivery and no unrelated gameplay-mode integration. Record failures and unavailable coverage in verification.md.

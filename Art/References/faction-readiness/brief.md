## Briefing timer addition — 2026-10-24

Reuse runtime-icon-toggle.png for the existing screen composition. Add only one
plain `Briefing 00:00` text readout in the top-right header, immediately left of
the player count. No clock icon, panel, border or control. Minutes continue beyond
59. Visible during the authoritative BRIEFING phase, including when browsing
other lobby tabs; hidden before and after. The server records phase entry once,
and all clients subtract that timestamp from the engine's synchronized clock.
Acceptance: starts at phase entry, advances, survives menu reopen, matches remote
and late-joining clients within sampling tolerance, and disappears on phase exit.

## Icon-only revision — 2026-10-24

The user requested faction name, readiness icon and CO only. Reuse concept-v1
for placement/theme; this narrow revision removes the state words and separate
Mark ready button, moves activation onto the icon, and reduces tile height.
The icon stays visible for everyone; only the local CO can activate it. Pending
requests disable activation until the server responds. Error feedback remains
conditional. Native hover/focus styling applies to the icon's 48×48 hit area.
`runtime-icon-toggle.png` is the current rendered reference. Earlier captures and
the element table below document the superseded initial design.

# Faction readiness reference

Active concept: concept-v1.png, built-in image generation, 2026-10-23. Edited from baseline-preview.png (MCP capture of Arland Preview, not a Briefing baseline). Generated image is a concept, not game evidence.

Journey: choose the first slot of the first displayed faction group â†’ enter Briefing â†’ inspect faction states â†’ commanding officer marks own faction ready/unready â†’ read server-confirmed result. Other players see the state and who controls it. Vacant/disconnected commander means not ready. Match start remains an admin decision.

| Element | Purpose and placement | Owner | Decision |
|---|---|---|---|
| Existing map, briefing list, player/voice panel, navigation | Plan and navigate in current layout | Existing menus | Keep |
| Bottom-center faction tiles | Compare each faction's readiness without covering planning panels | Server readiness | Keep |
| Faction accent/name | Identify affected side, independent of color | Native faction | Keep |
| Check/not-ready icon and text | Distinguish state without relying on color | Server readiness | Keep |
| Mark ready / Mark not ready action | One explicit reversible action for own faction CO | Controller request, server validation | Keep |
| Commanding officer name or vacancy | Explain authority and unavailable action | Derived top slot, native connected player | Keep |
| Pending/rejection text | Distinguish request from confirmed state | Request receipt | Show only when needed |

Theme: existing dark translucent neutral surfaces, condensed white text, muted secondary text, gold hover/focus action, faction-colored accent; green ready check with text. Bottom anchored, small tiles, preserve map and both side panels. Keyboard/mouse primary; native button focus supports keyboard/gamepad but actual coverage must be recorded.

Concept critique: compact clear status and explicit outcome action. Implementation adaptation: put each faction's commander caption inside its tile rather than one ambiguous shared caption; use native check/disable icons and project button interaction. Keep action only for eligible own commander; other tiles remain readable. Constrained viewport and longer names need actual captures. No automatic start, CO reassignment UI, callsign override or per-player readiness.

Prompt: Edit actual Lobby2 MCP screenshot, preserve existing geometry and theme, select Briefing, add compact bottom-center US not-ready and USSR ready tiles with faction accents, check/status symbols, gold-focused Mark ready action and commander caption; no dashboards or extra features. Full prompt is in this chat's image-generation tool call.

Acceptance: a single designated top slot per faction; empty top slot never promotes a lower occupant; only currently connected occupying CO in BRIEFING can mutate own faction; server rejects stale/non-CO/wrong-phase requests; departure clears prior acknowledgement; states replicate including JIP; repeated open/close has no duplicate listeners; native click reaches production RPC; screenshots match compact bottom-center concept with readable captions and unobscured side panels. Missing multiplayer/input coverage remains a limitation.

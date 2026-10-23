# Faction readiness reference

Active concept: concept-v1.png, built-in image generation, 2026-10-23. Edited from baseline-preview.png (MCP capture of Arland Preview, not a Briefing baseline). Generated image is a concept, not game evidence.

Journey: choose the first slot of the first displayed faction group → enter Briefing → inspect faction states → commanding officer marks own faction ready/unready → read server-confirmed result. Other players see the state and who controls it. Vacant/disconnected commander means not ready. Match start remains an admin decision.

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

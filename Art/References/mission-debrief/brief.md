# Mission debrief reference

Active reference: concept-v2.png, generated with built-in image_gen on 2026-10-27 from concept-v1.png after the user added faction aggregates and ammunition breakdown. Both are concepts, not runtime evidence. Version 1 used an actual MCP screenshot of the Lobby2 preview as theme input.

Journey: native POSTGAME -> read own final mission summary -> Back; DEBRIEF header reopens it from lobby views. Keyboard/mouse and existing native menu input context. Nominal 1920x1080, constrained embedded viewport to be checked.

| Element | Purpose | Owner | Decision |
|---|---|---|---|
| Existing header | Mission context and familiar navigation | Lobby2 header | Reuse |
| Title and player line | Identify personal, one-mission scope | Debrief menu | Keep |
| Fifteen label/value rows in a native scroll area | Read participation, combat, aid, movement, radio and command | Server final counters | Keep |
| Personal/faction selectors | Choose private personal report or public faction aggregate | Owner-authenticated report request | Keep |
| Ammunition list | See which ammunition types were expended | Server projectile events grouped by type | Keep, scroll when long |
| Five-player note | Explain counted time | Fixed eligibility policy | Keep |
| Status | Explain pending or absent report | Server response | Show only when needed |
| Back | Leave report | Native menu | Keep |

Theme: dark teal surface, off-white primary text, muted secondary text, existing amber header selection. No charts, ranks, export controls or combat event table.

Prompt: Generate a full-screen 16:9 Lobby2 mission-only debrief using the actual lobby screenshot as theme reference, with dark teal background and existing header, DEBRIEF selected. AFTER ACTION REPORT; This mission · Burn0ut7; rows for counted play time, shots fired, grenades thrown, bandages given, player kills/deaths, travel on foot, driving/piloting, estimated radio time and command. Show the five-player rule and Back (Esc). No invented gameplay background, leaderboard or technical identifiers.

Concept critique: clear personal scope, readable hierarchy and single exit. Production intentionally uses whole metres for travel and no decorative row rules. Labels/values remain separate native text widgets. Final values arrive through an owner-only response after the mission ends; history remains independent of presentation.

Runtime resources: UI/layouts/Lobby_Debrief.layout, GC_Lobby_DebriefMenu.c.

Version 2 prompt: Preserve the dark teal surface, header and row style. Add My stats (private), USSR and US selectors; show USSR selected and a faction totals subtitle. Move totals left, add ammunition types/counts on the right; replace Command with Participants for faction reports. No individual names or roster in public faction reports. Keep five-player note and Back.

Version 2 critique: selectors make report ownership explicit; ammo has its own readable area. Production uses dynamic registered-faction buttons, scrollable ammunition, and separate Command/Participants labels. Personal identity is resolved on the server from the requesting controller; no request accepts another player's identity.

Runtime comparison, 2026-10-27: `runtime-host-populated.png` shows 30 actual shots with 25 ball/5 tracer; `runtime-peer-private.png` shows the separate client's zero-shot personal report; `runtime-peer-faction.png` shows the same client's faction aggregate with two participants. The native controls preserve the dark teal/amber theme, two-column totals/ammunition structure and explicit private/faction selection. The active concept's USSR/US labels become translated Soviet Army/US Army buttons. Native button widths fit their labels instead of stretching across the screen. No stock end-screen overlays this report. Empty data and Back/reopen were exercised.

Long-report comparison: `runtime-long-partial.png` and `runtime-long-bottom.png` use explicitly labelled synthetic values to exercise 40 ammunition types with long labels and a partial-history warning. The display bounds the list to 32 types, keeps the remaining eight in history, and allows scrolling to the final notice. The first implementation measured text before wrapping and clipped the bottom; forcing the native layout update before measurement corrected the first-render case. Independent review accepted the corrected captures. Mouse hit testing, keyboard focus traversal and gamepad navigation remain unverified.

Combat breakdown extension: keep concept-v2 composition and typography, with separate player kills, AI kills, player team kills, total deaths, deaths by players, deaths by AI and other deaths. The left summary now scrolls to retain readable row spacing; the right ammunition area stays independent. Actual MCP captures `runtime-combat-top.png` and `runtime-combat-bottom.png` show the top and bottom. Geometry and native scrolling were checked; mouse-wheel/scrollbar hit testing and keyboard/gamepad navigation remain unverified. See `tests/CombatStats.md` for state and cache evidence.

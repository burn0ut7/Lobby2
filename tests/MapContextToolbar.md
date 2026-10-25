# Briefing input and toolbar — 2026-10-25

The reported failure affected all briefing clicks. The priority-51
`GC_BriefingToggle` context suppressed the priority-50 map/menu context.
Before the fix, `IsActionActive` returned zero for MapSelect,
MapContextualMenu and MenuSelect in Briefing, while GC_ToggleBriefing remained
active. Preview had all three actions active. Installed Core inputManagerDoc.c
documents this priority behavior. Earlier callback-only tests missed it.

The toggle now belongs to the existing GC_Lobby_Preview context; its listener
still belongs to the briefing menu. The separate per-frame activation is removed.
ToolMenu is bottom-left above the scale and receives native cursor-enter events.
BriefingView's maximum height leaves clearance above the toolbar.

Embedded Arland validation used Workbench PID 276, Lobby2McpTests, and Reload
without restarting Workbench. Final reload completed at console line 3031 in
logs_2026-10-25_19-34-54. At a 2613 × 1408 game viewport:

- MapSelect, MapContextualMenu, MenuSelect and the hide action are all active.
- Widget tracing reaches the enabled Lobby button and toolbar.
- Native toolbar enter/click/leave visually selects and deselects the ruler.
- Native context-menu open → marker category → custom marker opens the editor;
  native confirm closes it. After a second Reload, a fresh run verified the native
  marker collection increases from 1 to 2 through this exact flow (receipts
  r76f3683502204a018f49d065b59d28b8 and the following count in receipts.json).
- Native D-button callbacks hide/show the panel; action availability remains intact.
- Lobby navigation and return to Briefing work. Preview navigation is phase-blocked
  during BRIEFING. Fresh PREGAME after the second Reload confirmed preview has
  no tools, toolbar, or toggle (rbe08d04725094abc91424b057fd7a223).
  Preview and spectator layouts are unchanged.
- ToolMenu occupies x=10.43, y=1267.2, width=438.04, height=73.01. Its bottom
  leaves 67.79 pixels to the map bottom. The final screenshot shows the scale clear.

Original MCP captures and raw receipts are local in
`Art/References/map-context-toolbar/`: `toolbar-final.png`,
`context-marker-options.png`, and `receipts.json`. Early baseline screenshots
were captured before radial-menu animation settled; they do not establish a
radial rendering failure. The decisive baseline is native action availability.

Receipt-correlated logs show no script errors during these map interactions.
The fixture separately logs a missing loadout manager, mouse-icon mask errors,
and world/resource warnings; this is not an error-free world assertion.
Tests invoke native callbacks through MCP and observe input gating, hit targets,
state and screenshots. Physical mouse/key dispatch, alternate resolutions,
gamepad, remote multiplayer and fresh spectator coverage remain unverified.

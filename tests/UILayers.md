# Lobby notification layers

Request: base-game notifications behind the lobby; chat and pause above it,
with the lobby still visible while paused. This is a layering fix, not a visual
redesign; the existing menu layouts and pause behavior are retained.

## Implementation

`LobbyPlayerController.et` uses `GC_LobbyHUDManagerComponent`, an addon-owned
subclass of the native HUD manager. After native initialization, the slot-based
HUD root is at -2 and the legacy HUD root at -1. Their relative order is retained,
and both are below the native menu stack at zero, independent of creation order.
The separate ALWAYS_TOP layer remains unchanged for its other native uses.

The controller registers the native PopupUI layout as an OVERLAY info display.
`SCR_PopUpNotification.ProcessInit` finds that registered layout, preserving its
native message queue, duration, text and settings rather than creating an
ALWAYS_TOP layout. Menu chat remains a child of the menu; native pause stacking,
focus and Continue are unchanged. No shared modded production classes were added.
The original base controller prefab and unrelated game modes are untouched.

Source evidence: installed game catalogue `4ee49efc58494a12b2fde693a84d62e9`,
`SCR_HUDManagerComponent.c` (OnInit/CreateHUDLayers/FindLayoutByResourceName),
`SCR_HUDManagerLayoutHandler.c` (InitializeHUDLayouts),
`SCR_PopupNotification.c` (ProcessInit), and `game.c` (OpenPauseMenu).

## Runtime evidence, 2026-10-25

Raw captures and receipts are local ignored artifacts in `Art/References/ui-layers/`.

Workbench 1.8.0.13, optional Lobby2McpTests addon, embedded Play. Final scenario:
`$Lobby2:worlds/examples/arland/Arland_Lobby.ent`, `GameMode_Lobby2`.
3840x2160 editor window; screenshots are uncropped MCP captures reduced to
1600x900. Existing viewport/UI-scale settings were preserved.

- Full native script reload succeeded (operation
  `d282be85c5db4060944effe392eaf3e4`).
- `GC_TestUILayers("notify")` invokes the native popup and PLAYER_JOINED log
  notification. A later snapshot confirms the popup text, opacity 0.824 and
  ancestry under OVERLAY / root -1, with notification log under root -2.
- Preview and slotting: ChatToggle down/up opened native chat, with `open=1`.
  See `preview-chat.png` and `lobby-chat.png`.
- With chat closed, MenuBack down/up opened PauseMenuUI. Both lobby roots stayed
  visible. Native Continue returned to the lobby. See `preview-pause.png` and
  `lobby-pause.png`.
- Closing the lobby through the native menu lifecycle exposed the same live
  popup; see `notification-uncovered.png`. Reopening through the production
  pause-menu lobby action restored the lobby above notifications.
- No SCRIPT errors in the final reload/runtime cycle. `receipts.json` records
  RST_MCP_BEGIN/END evidence; `results.json` records returned state and receipts.

Initial probes are retained separately. A script interception experiment did
not establish popup placement and was removed; the final implementation uses
native registered-layout configuration. Empty_Lobby lacks chat and a map, so
its chat-unavailable/map error observations are not passing coverage.

Limits: semantic native input/button tests, not physical mouse/gamepad tests;
one viewport/UI scale and embedded client only. No multiplayer/network logic
changed. Chat-open MenuBack probing did not establish dismissal, so chat was
closed through its native manager before testing the pause action. No claim is
made about that pre-existing input interaction or unrelated-mode runtime coverage.

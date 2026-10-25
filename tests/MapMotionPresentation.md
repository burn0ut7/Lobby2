# Map motion synchronization — 2026-10-25

The client-presentation optimization deferred map pan/zoom projection to a 16 ms
callback. That callback is not synchronized with the map's render update and can
leave shapes and labels behind the moving map. Pan and zoom events now call the
existing projection immediately. The unchanged-view guard still skips redundant
geometry work; server snapshots remain change-driven. The sizing callback remains
for opening/resizing the canvas, not for pacing map motion.

Installed `game/data/data007.pak/scripts/Game/Map/SCR_MapEntity.c`, SHA-256
`09ca0631ce2f5a196b7074e12a67c08255b7119b9600da635894097b35e4ad92`, confirms:
SetZoom updates zoom before invoking the zoom event (619–641); SetPan updates
position before invoking the pan event (760–769). WorldToScreen's `withPan=true`
includes current pan (904–917). PAK catalogue revision:
`4ee49efc58494a12b2fde693a84d62e9`.

## Live verification

Compiled through MCP operation `33df4446bf3f47e697d8788d2dc44f9d`, then ran Arland
with the optional test addon. Tests use native ZoomPanSmooth, not synthetic map
callbacks. The test probe compares a submitted polygon vertex to native
WorldToScreen before each pan/zoom callback returns. Its integer output permits
approximately one pixel of rounding plus float precision.

- Lobby motion: 520 pan and 520 zoom events, maximum difference 1.00031 px.
  Receipt `r6ba2b04de3644fc49766a1fbd46944e4`.
- In-game three-second pan/zoom: 803 pan and 803 zoom events, maximum difference
  0.999268 px, zero snapshots. There were only 411 sizing ticks over the entire
  observation, showing projection was not paced by that timer.
  Receipt `r8ccbc048c26b418fb37e851792050ae4`.
- Stationary open map: 601 sizing ticks, zero projection calls/rebuilds/snapshots.
  Receipt `rb30d740a40c14e4787568b1db33f255d`.
- Closed map after its close event: all counters zero, including sizing ticks;
  retained snapshot remains local. Receipt `r3ed8b8acae304cd2a18c89beda842520`.

The first test probe incorrectly used WorldToScreen's default `withPan=false` and
reported large differences. Those results are retained in the raw receipts but are
invalid as motion-error measurements. The corrected probe was recompiled and the
checks above rerun. No production geometry correction was needed for that test error.

Raw receipts: `evidence/map-motion-2026-10-25.json`. MCP screenshots at
`Art/References/map-motion/motion-start.png` and `motion-end.png` show the same
shapes and admin labels at two positions/scales. They are local ignored references.
Callback measurements establish update timing; still images alone cannot prove
perceived smoothness. Manual mouse/gamepad gestures, separate-client networking,
and a multi-resolution performance benchmark were not repeated for this local-only
callback change. Existing network/authority behavior is unchanged.

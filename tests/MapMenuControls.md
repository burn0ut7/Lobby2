# Map menu controls — 2026-10-25

Default briefing toggle: **D** (hold controller View). The native input button in
the briefing header also accepts clicks. Configure **Hide / show briefing** in
Controls → LOBBY2. Hiding preserves the selected page and leaves the map open.
Spectator map has its own configurable action, default **M**.

## Implementation references

Reforger Lobby Community's `PlayableSelector/PS_MissionDescriptionUI.c` uses
`SwitchMissionDescription` to toggle the description root. Lobby2 uses the native
`SCR_InputButtonComponent` for the same visibility behavior and binding hint.

Installed Reforger 1.8.0.13 `SCR_MapEntity.SetupMapConfig` caches by map mode,
not resource path. Preview, briefing and spectator therefore have distinct modes.
Spectator uses the editor map configuration without drawing/tool components;
briefing retains the fullscreen tools. Map controls must remain alive until their
own close callbacks finish. Preview/briefing waits for the previous map's close
event before replacing its configuration; menu teardown cancels that subscription.

## Embedded Workbench checks

Optional addon: `tests/McpTestAddon/addon.gproj`, Arland lobby world, local admin.
Production handlers invoked through the generic MCP runtime interface. Successful
script reload: operation `9f823ef7eec94eeabe6f084e8f77fe61`.

- Preview: map open, zero tools/drawing, toolbar and toggle hidden
  (`r21c11c9f4e024441af1c2fe5d46302ef`).
- Briefing: six visible tools; native click hides the pane, native key handler
  restores it without closing the map; selected rich-text page survives
  (`rd30dd4fd91564cf9861314ee7accca27`, `rd8ab56c957c44920ad22a65d02e906e9`).
- Native ruler button opens its tool after the native hover handler enables it.
- Normal gameplay map: seven native tool buttons, fullscreen mode
  (`r6ec487933da24db7ae8ea12833626255`).
- Close gameplay map and immediately open briefing: briefing owns an open map,
  six tools, no remaining native MapMenu (`rd7378e5e3b4a4ee9a964de697c57da45`).
- Spectator: six successive toggles alternate open/closed with no drawing/tools
  or competing MapMenu (`re5d69ee8b5f541bda7b646dad0647182` through
  `rcf21494d037f43ae9abb9afe5886f45a`). No NULL/overlapping-map errors in this interval.

Original failure: `logs_2026-10-25_18-18-47`, lines 1063–1214,
`SCR_MapDrawingUI.UpdateLineCount` null access while two map owners overlapped.
Validation log: `logs_2026-10-25_18-49-15/console.log`, final run begins line 1438;
spectator interval follows line 1899. Unrelated resource/GUI/loadout diagnostics
remain; this is not a claim of a globally error-free scenario.

Local screenshots and receipt readbacks are saved in ignored
`Art/References/map-menu-controls/` (briefing-visible, briefing-hidden,
briefing-tools, briefing-from-gameplay, gameplay-map, spectator-map, preview).

## Limits

These checks exercise production callbacks, not physical keyboard/controller
input. An attempted temporary remap through native InputBinding did not alter
the live Workbench binding; default was restored and that mutation helper removed.
Remapping in the standalone game's Controls screen remains to be verified.
No frame-time benchmark, separate-client multiplayer test, or alternate-resolution
test was performed. No new shared gameplay callback overrides ship in production.

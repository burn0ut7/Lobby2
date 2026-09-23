# UI palette refresh — 2026-09-23

## Presentation contract

The existing layout hierarchy, maps, faction artwork, role icons and phase flow remain in use. The palette is:

| Role | Authored RGBA / runtime source |
| --- | --- |
| Screen and navigation background | `0.008 0.02 0.045 1` (deep blue) |
| Selection, active text, links and fine accents | `1 0.816 0.251 1`; `GC_LobbyUIColors.ACCENT` |
| Normal text | White |
| Secondary text | `0.839 0.839 0.839 1`; `GC_LobbyUIColors.TEXT_MUTED` |
| Rows and panels | Black with 0.55 / 0.72 / 0.84 / 0.92 alpha |
| Faction accents | The base faction's `GetFactionColor()` |
| Faction surfaces | `GC_LobbyUIColors.FactionBackground`: copy the faction color, blend 82% toward black, set alpha to 0.92 |

The faction-color helper changes a copy, never the faction-owned color. Cards, squad panels, faction voice headers and spectator group headers share this rule. Selected cards have a yellow outline. Enabled options use yellow text over translucent black. Hover/click/selected button backgrounds are explicitly authored so inherited orange defaults cannot reappear. Diagnostic world effects, kill attribution colors and mission-authored map colors retain their meaning.

The Parameters faction column is 276 units wide. Its 24 units of padding leave 252 units for a 248-unit card (224 content + 24 padding); the previous 240-unit column left only 216 units.

Four inventory detail metadata files lacked their `Configurations` blocks. They now declare the same `LayoutResourceClass` platform configurations as the adjacent inventory resources, preserving every resource GUID.

## Evidence and API ledger

Official Wiki: [Layout Creation](https://community.bistudio.com/wiki/Arma_Reforger:Layout_Creation), packaged `Modding/Tutorials/Layout Creation.md`, lines 3–68, revision `ow1:d40df5e4830cab07dbc5a9c1c06beadd521c4214315d43176954e54c27f682e7`. This distinguishes menus, displays and widgets and explains that component properties control button presentation.

Game Data declarations were inspected against catalogue `gd2:470a88846014dccab8cf67efa8aa2454fbc8ba5f12fd53b9b8a8933d6f1a099d`:

| Identifier | Verified declaration |
| --- | --- |
| `Faction.GetFactionColor()` | Returns `ref Color` from the faction; `scripts/Game/generated/Faction/Faction.c:24` |
| `Color.FromInt(int)` | Static, returns `ref Color` from packed ARGB; `scripts/Core/generated/Visual/Color.c:73` |
| `Color.FromRGBA(int,int,int,int)` | Static, returns `ref Color`, 0–255 components; same file, line 77 |
| `Color.SetA(float)` | Sets alpha; same file, line 111 |
| `Color.Lerp(Color,float)` | Mutates the receiving color by interpolation; same file, line 142 |
| `Color.PackToInt()` | Returns packed integer; same file, line 149 |
| `Widget.SetColor(notnull Color)` | Native widget color setter; `scripts/Core/generated/UI/Widget.c:70` |
| `Widget.SetOpacity(float)` | Native opacity setter; same file, line 80 |
| `Widget.FindAnyWidget(string)` | Returns a widget; same file, line 103 |
| `SCR_AIGroup.GetFaction()` | Returns the authoritative `Faction`; `scripts/Game/Entities/SCR_AIGroup.c:2171` |
| `SCR_ButtonBaseComponent.m_BackgroundDefault/Hovered/Selected/SelectedHovered/Clicked` | Attributed `ref Color` fields; `scripts/Game/UI/Components/WidgetLibrary/Button/SCR_ButtonBaseComponent.c:18–30` |
| `SCR_ButtonTextComponent.m_TextDefault/m_TextToggled` | Attributed `ref Color` fields; `scripts/Game/UI/Components/WidgetLibrary/Button/SCR_ButtonTextComponent.c:14–17` |
| `GC_LobbyUIColors` and its members | Workspace-owned presentation values and color derivation |
| New `FactionAccent` and `SelectionOutline` widget names | Workspace-owned bindings in `Lobby_Faction.layout` |

The packed source reader reported a changed `data007.pak` revision for a separate `SCR_Faction` source-body request. No implementation relies on that unavailable body; the new color derivation uses the inspected native color declarations above. Refreshing the Game Data cache/MCP remains an environment maintenance item.

## Validation results

- Native `workbench_validate_scripts`: **success**, WORKBENCH profile, 14 base-game deprecation warnings, no next diagnostic cursor and no Lobby2 diagnostics.
- Native reload: **dispatched**, replacement runtime generation **1493379**. The named Arland example world was saved before reload. The first reload attempt timed out; returning from play mode and repeating validation/reload succeeded.
- Fresh Workbench status: running with scripts compiled; active world `$Lobby2:worlds/examples/arland/Arland_Lobby.ent`, loaded add-ons `core`, `ArmaReforger`, `Lobby2`.
- Native registration inspection found Preview, Slotting, Briefing, Parameters, faction card, player list panel, spectator HUD and spectator options as `LayoutResourceClass` resources.
- Inventory detail initially returned `resource-not-found` with a missing-metafile warning. After repair, detail, category, item and row all returned `found: true`, `LayoutResourceClass`.
- A fresh play run logged creation/opening of `GC_Lobby_PreviewMenu`, map opening and successful creation of a briefing marker, including its settled widget dimensions. This proves the startup path reached those widgets; it is **not visual acceptance**.
- The test play session was stopped. Logs recorded Lobby2 manager shutdown and return to editor-world creation.
- Resource brace/quote structure inspection and `git diff --check` passed.

Reload/play evidence is in `logs_2026-09-23_13-52-54/console.log`, approximately 14:17:47–14:18:38 local time.

The play run also logged base-game vehicle keyword errors (`SlidingTrackMaterial`, `Parent`), chat `EditBoxFilterComponent` warnings, a Hint prefab-member warning and an `icons_mouse.imageset` mask error. Those shared assets were not changed. Runtime cannot be described as error-free.

## Outstanding visual coverage

**No screenshots were captured.** The callable Workbench tools in this session do not expose screen capture or preview navigation; the Computer Use runtime (`node_repl`) is also unavailable. No replacement mockup is being presented as an engine screenshot.

Capture and visually inspect each of these at the user's target resolution:

- Mission preview, including compact/expanded briefing and player/voice tabs.
- Slotting, faction selection, squad rows, character preview and loadout.
- Briefing, including selected entries and inline links.
- Parameters, including faction cards, toggles, dropdown selections and Apply.
- Inventory detail, item/category rows and tooltips.
- Spectator HUD, pinned/unpinned player panel, options, hover cards and kill labels.
- Context menus and hover/focus/selected/disabled states across these screens.

Dedicated-server, remote-client, JIP and unrelated-mode runtime tests were not performed. This change edits Lobby2-owned presentation resources and components; it adds no replicated state or authority rules.

# GC Lobby

Lobby and role selection, briefing, match and after-action reporting for Arma Reforger.
Requires Arma Reforger. GC Lobby's mission framework is independent of TilW Mission Framework.

TilW is a research reference, not a runtime dependency or behavior specification.
GC Lobby uses its own mission logic and the base game's APIs; reference designs
must be evaluated against GC Lobby's requirements rather than reproduced by default.

Branding, editor categories and messages use **GC Lobby**. The internal project
ID/mount `Lobby2`, existing resource GUIDs, profile paths, database schema,
and `LOBBY2` compatibility define retain their existing contracts.
These technical names are not the displayed mod title.

## Addon contents

- `scripts/Game`, `scripts/GameCode`: gameplay and integrations with the game.
- `scripts/WorkbenchGame`: native mission-authoring menus and tools.
- `Configs`, `Prefabs`, `UI`, `Language`, `Sounds`: game resources; preserve GUIDs and metadata.
- `worlds/examples`: authoring examples, including the Arland scenario.
- `addon.gproj`: addon identity, dependencies and localization.

The [mission framework source guide](scripts/Game/MissionFramework/README.md)
maps events, conditions, actions, flags, areas, interactions and runtime execution.

`Prefabs/MP/Modes/GameMode_Lobby.et` is the single canonical game mode.
Save mission-specific settings and compositions as inherited prefabs under
`Prefabs/Templates` (or in the mission's own addon), rather than adding another
game mode. `GC_MissionSettings_US_USSR.et` supplies faction-specific parameters;
`GC_MissionTemplate_Coop.et` supplies the supporting mission managers. Both
inherit the same game mode. Their resource GUIDs are preserved.

Shared script hooks must preserve inherited behavior unless the active mode is
`GC_LobbyGameMode`. The formation source is a separate GC Lobby prefab, not an
override of the game's AI world. The bullet-base override adds only a telemetry
callback; recording requires the active GC Lobby session on the server.
Outside GC Lobby, the recovery observer only invalidates an existing GC Lobby
last-mission record so an intervening mission cannot restore an old map plan.

Open `addon.gproj` to work on the production addon. Create a mission in its own
addon, inherit a supported terrain, and use **Plugins > GC > Spawn Mission Template**.
Configure playable groups, briefing, mission events and a mission header before publishing.
Use **Plugins > GC > Open Wiki** to open the
[GC mission-making wiki](https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/wiki)
in your default browser.

## External components

`external/workbench/Preview UI.js` opens the shared GC tool-window design preview.
Its sample dependency, update and submission forms do not perform operations.
The UI is bundled with the existing Windows runtime; no separate installation is needed.

**Plugins > GC > GC Tools** combines **Submit mission**, **Update mods**, and
**Install external dependencies** in one window. **Open Wiki** remains separate.

For a source checkout, use **GC Tools > Install external dependencies**, or
open `external/workbench/Install.js` to view GC Tools. Lobby's `external` folder
contains the complete tool package and .NET runtime. Opening it does not require
GC Core, Units, Missions, Git, the dropdown DLL, or Reforger MCP. Each checkout
opens its own verified package; installed copies are not required for startup.
Do not start Workbench with `-noThrow`: it blocks protected script file/process
operations, including the GC Tools button. Remove that option and restart Workbench.
Save work and stop Play before installing the dropdown DLL.
Setup installs the dropdown DLL, profile Git helpers and Git for Windows.
Save and restart Workbench after installing; subsequent launches load it automatically.
Workbench NET API must be enabled in its saved preferences. Git installation needs internet access.
The archive includes its own .NET runtime; no developer tools are required.
Packed Workshop addons cannot launch these external Windows files.

`external/workbench/GCWorkbench.zip` is the packaged DLL and installer, with
a SHA-256 sidecar checked before extraction. Setup extracts to a unique Windows
temporary folder and installs under `%LOCALAPPDATA%/GCWorkbench`; it does not
overwrite a loaded DLL. To uninstall, extract the archive and run `Uninstall.bat`.
The package is generated from GC Lobby-owned sources in `external/source` using
`external/source/scripts/package-workbench-integration.ps1`. Reforger MCP is not
a build, install or runtime dependency.
Keep external Windows packages out of the published Workshop package.

Companion sources live in `external/development/lobby2` in this repository.
The standalone Workbench DLL installer and tools live in `external/source`:

- `tools`: Windows Git updates, mission branches and submission. The installer
  places these in the active Workbench profile under `GCWorkbench/tools`.
  Git operations require source checkouts; they cannot update packed Workshop addons.
- `integrations/StatsIngest` and `integrations/Contracts`: separately deployed
  statistics/history service and shared data contracts. Follow the service README
  for PostgreSQL setup, migrations and profile access; the mod does not host it.
- `integrations/MapDrawing`: separate optional addon requiring GC Lobby and
  Map Drawing (`656AC01634459D8D`); publish it independently if needed.
- `authoring`: detailed mission-authoring references.

Test projects, captures and historical development notes are not release inputs.
`resourceDatabase.rdb` is Workbench-generated local state, not authored content.
Publish through Workbench and inspect the produced package before release;
source-tree cleanup alone does not validate a Workshop package.

## Development content ownership

GC Tools targets `dev` for GC Core, GC Units and GC Missions. Use their local
Dev addon checkouts (GlobalConflictsCoreDev, GlobalConflictsUnitsDev and
GlobalConflictsMissionsDev); release addons have different identities.

- GC Core Dev owns shared mod-pack functionality and changes.
- GC Units Dev owns unit/loadout prefabs. The loadout authoring tools write only
  to its `Prefabs/Loadouts` tree, including gear and weapon variants. Load this
  addon in Workbench before saving; other addons' prefabs remain read-only inputs.
- GC Missions Dev owns GC mission worlds, headers and mission-specific scripts
  and data. Save GC mission worlds there. Create Mission Data intentionally follows
  the selected world's owning addon, so other addons' worlds keep their own headers.

The Units Dev project depends on Core Dev; Missions Dev depends on both. The
GC Lobby framework stays separate. Its example worlds remain development examples,
not automatically moved into GC Missions. Submission updates contributors' forks
and requests approval into dev; it does not push directly to shared branches.

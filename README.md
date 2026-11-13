# Lobby2

Lobby and role selection, briefing, match and after-action reporting for Arma Reforger.
Requires Arma Reforger. Lobby2's mission framework is independent of TilW Mission Framework.

TilW is a research reference, not a runtime dependency or behavior specification.
Lobby2 uses its own mission logic and the base game's APIs; reference designs
must be evaluated against Lobby2's requirements rather than reproduced by default.

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
`GC_LobbyGameMode`. The formation source is a separate Lobby2 prefab, not an
override of the game's AI world. The bullet-base override adds only a telemetry
callback; recording requires the active Lobby2 session on the server.
Outside Lobby2, the recovery observer only invalidates an existing Lobby2
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

For a source checkout, use **Plugins > GC > Install External Dependencies**, or
double-click `external/workbench/Install.js`. Save work and stop Play first.
Setup offers the dropdown DLL, profile Git helpers and the official Git for
Windows installer. It asks for your Workbench executable; save and restart
Workbench after installing the integration. Git needs an internet connection.
The archive includes its own .NET runtime; no developer tools are required.
Packed Workshop addons cannot launch these external Windows files.

`external/workbench/GCWorkbench.zip` is the packaged DLL and installer, with
a SHA-256 sidecar checked before extraction. Setup extracts to a unique Windows
temporary folder and installs under `%LOCALAPPDATA%/GCWorkbench`; it does not
overwrite a loaded DLL. To uninstall, extract the archive and run `Uninstall.bat`.
The package is generated from the authoritative sources in `reforger-script-tools`
using `scripts/package-workbench-integration.ps1 -LobbyDirectory <Lobby2 path>`.
Keep external Windows packages out of the published Workshop package.

Companion sources live in `companion/lobby2` in the `reforger-script-tools`
repository, alongside the standalone Workbench DLL installer:

- `tools`: Windows Git updates, mission branches and submission. The installer
  places these in the active Workbench profile under `GCWorkbench/tools`.
  Git operations require source checkouts; they cannot update packed Workshop addons.
- `integrations/StatsIngest` and `integrations/Contracts`: separately deployed
  statistics/history service and shared data contracts. Follow the service README
  for PostgreSQL setup, migrations and profile access; the mod does not host it.
- `integrations/MapDrawing`: separate optional addon requiring Lobby2 and
  Map Drawing (`656AC01634459D8D`); publish it independently if needed.
- `authoring`: detailed mission-authoring references.

Test projects, captures and historical development notes are not release inputs.
`resourceDatabase.rdb` is Workbench-generated local state, not authored content.
Publish through Workbench and inspect the produced package before release;
source-tree cleanup alone does not validate a Workshop package.

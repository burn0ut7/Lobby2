# Lobby2

Lobby and role selection, briefing, match and after-action reporting for Arma Reforger.
Requires Arma Reforger and TilW Mission Framework (`61F0E6AB2D06B1C6`).

## Addon contents

- `scripts/Game`, `scripts/GameCode`: gameplay and integrations with the game.
- `scripts/WorkbenchGame`: native mission-authoring menus and tools.
- `Configs`, `Prefabs`, `UI`, `Language`, `Sounds`: game resources; preserve GUIDs and metadata.
- `worlds/examples`: authoring examples, including the Arland scenario.
- `addon.gproj`: addon identity, dependencies and localization.

Open `addon.gproj` to work on the production addon. Create a mission in its own
addon, inherit a supported terrain, and use **Plugins > GC > Spawn Mission Template**.
Configure playable groups, briefing, mission events and a mission header before publishing.

## External components

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

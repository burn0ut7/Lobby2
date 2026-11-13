# GC Lobby companion components

These development sources belong to GC Lobby. Exclude the entire `external`
directory from Workshop releases; test addons must be copied to separate addon
workspaces before loading. They are not part of Reforger MCP's provider-independent server logic.

- Windows tools and native DLL sources live in `../../source`; they are owned
  and packaged by GC Lobby.
- `integrations/StatsIngest` and `integrations/Contracts`: external statistics
  ingestion, player history and shared data formats. Build/deploy independently
  following `integrations/StatsIngest/README.md`. Database setup and service
  activation are explicit server operations, not desktop installer side effects.
- `integrations/MapDrawing`: optional separately published Reforger addon.
  Copy its source directory into an addon workspace before opening `addon.gproj`.
- `authoring`: mission-author documentation retained from the addon source.

The Windows helper chain is `GC-ModLauncher.js` (Windows Script Host) →
`GC-ModWorkflow.ps1` → `GC-SubmitMission.ps1` when submitting a mission.
It uses Git source checkouts and opens submission/review workflows. It is not
Enforce gameplay code and cannot operate directly on files inside a PAK.

Bohemia's publishing documentation says bundling removes unneeded source files:
https://community.bistudio.com/wiki/Arma_Reforger:Mod_Publishing_Process
The exact current publisher extension whitelist was not available in the
inspected installed configuration. Do not claim `.ps1`/`.js` are always excluded;
even bundling their bytes would not make Windows execute them from a PAK.
The installer supplies real filesystem paths instead.

Source migration: GC Lobby repository, release cleanup on 2026-11-11. Existing
runtime protocols and addon GUIDs are unchanged. Historical tests and evidence
were removed from the release source and preserved in a local recovery directory.

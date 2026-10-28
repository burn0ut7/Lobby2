# Mission framework dependency

Lobby2 requires **TilW Mission Framework**, addon GUID `61F0E6AB2D06B1C6`.
Keep its existing addon files unchanged. The local checkout is the sibling
`tilw-mission-framework` directory.

The custom Lobby2 mission framework has been removed, including flags, events,
conditions, actions, interaction prefabs, authoring entities and its test suite.
Old `GC_Mission*` rules and the US-defeated example are no longer supported.
The Arland example retains its lobby setup without those rules or probe entities.

Use `tests/McpTestAddon/addon.gproj` for development. Its Lobby2 dependency now
also requires TilW; keep the existing isolation probe and MCP test addon loaded.
Confirm actual loaded addons through `workbench_project_context`; a saved project
dependency alone does not establish that the running editor has loaded it.

This change establishes the dependency only. It does not implement a Lobby2
adapter, translate old rules, or claim TilW mission execution has been validated.
Lobby2 phases, briefings, scenario parameters, administration and statistics
remain owned by Lobby2.

## Validation (2026-10-28)

Workbench 1.8.0.13 loaded TilWMissionFramework, Lobby2, Lobby2McpTests and
Lobby2IsolationProbe together. A single restart activated the new dependency;
script Reload alone left TilW available but unloaded. See Bohemia's
[dependency loading documentation](https://community.bistudio.com/wiki/Arma_Reforger%3AResource_Manager%3A_Options).

Combined Game and Workbench script Reload succeeded. The reload cycle in
`logs_2026-10-28_01-58-33/console.log` (lines 4064–4124) has zero errors and
38 warnings, including existing obsolete APIs and unused TilW variables.
The cleaned Arland world reopened and entered embedded Play with Lobby2's
preview visible. Runtime inspection found its `GC_LobbyGameMode` and manager
(receipt `rf72284dd35f2432cb6e65a54eeff9884`). No SCRIPT errors were logged through
that smoke check. This is startup coverage, not multiplayer or TilW rule-execution
validation. Existing resource/plugin diagnostics remain, including a stale
ArlandLobby world reference and unknown `SCR_DedicatedServerPlugin`; both also
occur in the prior session `logs_2026-10-28_01-21-32`.

All pre-existing TilW source/resource file hashes remained unchanged except
`resourceDatabase.rdb`, which Workbench regenerated while loading the addon.

# Mission framework dropdowns

The dynamic lists described below are a development prototype, not an addon-only
shipping feature. Lobby2 must ship without an executable helper or separate
installation; the executable packaging experiment was removed. Script-only
choices worked on an entity probe, but have not been verified for these nested
conditions. See `tests/README.md` for the scoped test results.

All seven marked flag/faction string fields also accept typed text, including
names that do not yet exist. This text-entry behavior works without the native
extension; ordinary mission validation still applies.

**Flag Is Set** and interaction **Required Flag** offer existing mission flags.
Create a flag on its Set Flag action, random flag action, flag pool, interaction,
or optional Mission Flags entry. The dropdown finds those sources automatically;
you do not need to declare the same flag twice.

**Player Casualties (%)** offers factions with playable groups containing slots.
It scans the current group sources, including inherited slot lists. Editing or
copying a playable group updates its available faction key without requiring a
matching faction-manager entry first. Mission validation still requires a defined
faction; selecting a key does not create a faction. Removing the last playable group
or its slots removes that faction from the available choices. The current Arland
example uses `US` and `USSR`; a mission using `RU` will show `RU`.

Other scalar faction fields offer keys from the world's faction manager, so AI
conditions can still target factions without player slots. Faction arrays still
use their existing editor controls.

With the native editor extension enabled, choices refresh about once a second.
Renamed or deleted sources do not rewrite conditions: their previous selection
appears as **(not found)** until you deliberately choose another value. **(None)**
stores an empty string; normal mission validation still decides whether it is
allowed for that field.

This prototype requires the matching Reforger MCP server and its installed editor
adapter, with `workbench_dynamic_choices(action="enable", processId=...)` active.
Keep the server running and enable again after restarting either process. A plain
Workbench session does not populate these choices. The extension currently checks
the exact Workbench 1.8.0.13 / Qt 6.8.3 binaries. See the tools repository's
`docs/workbench-dynamic-choices.md` for setup, status and provider authoring.

`GC_MissionChoices.c` owns Lobby2's source discovery; the extension owns native
control updates. Both are editor-only. Saved field names, flag behavior, condition
evaluation and mission authority remain unchanged.

# Spectator player jump

Faction rows request a server-resolved player position. The owner receives a vector, not an entity reference; the receiving path never resolves the target locally. Connected-player/group/slot data drives the roster independently of character replication. The server checks spectator state and phase, rejects missing targets, and uses the retained character for dead players. Arrival is two metres above eye position to avoid entering the character mesh. Old replies and replies belonging to an earlier camera are ignored.

Reference: installed Reforger Lobby Community `PS_AlivePlayerSelector.AlivePlayerButtonClicked` → `PS_PlayableControllerComponent.AskSetCameraPosition` → server `EyePosition()` → owner `SetOrigin()`.

## Verification — 2026-10-28

Workbench 8324, Lobby2 MCP Tests, Arland_Lobby. Reload `d5416faf838147deb9535ebab6983e64` stopped Play and completed. Automated log discovery was unavailable (ambiguous console files); inspected the fresh 11:56:35–42 cycle in `logs_2026-10-28_01-58-33/console.log`: Game loaded, reload completed, no script compile errors. Existing resource/plugin warnings remain.

Native lobby buttons: USSR slot 1 → Briefing → Game. Resolver rejected a playing requester. Test-only native character death entered the real spectator lifecycle. Opened Factions, moved camera 2113.96 m away, then invoked the production row click. Receipt `r8a8390105eff48a48ce65d3af0db8f5d`; subsequent observation `rb71106d891da49f5909aa8e131726a8c` reported request 1, free camera, distance 0 from the server destination. Stale response left that position unchanged (`r70e0c68378dc4e67bc31a70709996780`); missing target rejected (`r5574ef66cbc54c05900c36776f2f2d72`). No interaction script exceptions. Baseline world reports missing loadout manager on spawn.

MCP screenshot: main tooling repository `.temp/local/evidence/admin-scope-20261024/1793203031928934400.png`. Earlier exact-eye arrival clipped inside the corpse; the revised height offset was checked in this screenshot.

Test helper: `GC_TestSpectatorPlayerJump` in the optional test addon (`state`, `resolve`, `invalid`, `away`, `stale`, `die`). `die` is intended only for disposable embedded Play. Actual row click uses `GC_TestFactionRoster("select")`.

Coverage is embedded-host only. A separate client with an actually absent target proxy, live remote target, disconnect race, and camera replacement during an in-flight response still require multiplayer verification. No headless client was launched.

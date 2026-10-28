# Spectator option hotkeys

Defaults: **O** toggles Bullet Tracers; **K** toggles Killed From. Named down-edge actions are available in the spectator and spectator-map contexts. They call the existing menu toggle handlers, unregister when the menu closes, and have remapping entries under Lobby2. The native input-hint widgets display the current binding beside each option.

Verified 2026-10-28 in visible Workbench 8324, Lobby2 MCP Tests / Arland_Lobby. Reload `20a12fa8de4f465783610a4512df6d25` completed. Fresh console cycle 12:52:15–22 loaded Game without script errors; automatic console discovery was ambiguous, so the active `logs_2026-10-28_01-58-33/console.log` was read directly.

Native lobby phase buttons and character death entered actual spectator mode. Optional `GC_TestSpectatorHotkey(action, value)` uses `InputManager.SetActionValue`, not a direct toggle callback. First presses enabled tracers (`r658b26311b694d44b9efd6308e7bad3b`) and Killed From (`rf4c56e7d879a408396387aaff64f7b3b`); releases preserved state. Second presses disabled each (`rbcd77a1aa02242d58aa8c550e8cdddb4`, `raef032f00fd144dd9b32b6f7849adb4a`). No receipt-correlated script exceptions.

MCP screenshot in tooling repository `.temp/local/evidence/admin-scope-20261024/1793206393566841800.png` shows both native O/K hints and ON states. Both options were restored OFF afterward. Physical keyboard injection and remapping interaction were not exercised; the native action listener path and rendered default key mappings were verified.

# Background save investigation — 2026-11-01

## Changes

`GC_LoadoutPrefab.Register` now calls `RegisterResourceFile(path, false)`.
Saving source prefabs does not request a runtime asset build. This follows
`SCR_BaseContainerTools.c:69–81` in the installed game dataset
`d9b5a4246f9247e4be5ea20935889885`. No automatic Stop, Reload or restart is added.

Destination paths preserve existing faction/team directory capitalization by
looking at actual files. Windows accepts `Test` and `test` as one folder, but
the engine reported mismatched resource paths when a new meta file used `test`
inside the existing `Test` folder. Empty folders without prefabs do not supply
a canonical spelling through this lookup.

## Evidence

Workbench 1.8.0.13, PID 41892, optional McpTestAddon, Empty_Lobby world.
Compilation `6a06513e5f224f83ac125e80309e015a` passed with only existing
ArlandLobby, dedicated-plugin and formations diagnostics.

- Production menu save of a rifleman with a 4x20 optic created
  `US/BackgroundAudit/Weapons/M16A2_BackgroundAudit_1.et`, with the optic override,
  and `Scoped.et` referenced that weapon. Save receipt
  `e27d2fc71f8a4db78064cdb242d6a4a3`: synchronous handler 189 ms.
- `75276c385f0b46b1a8be896f8cb30fe2` verified same game mode and same character
  after save. This proves session continuity, not absence of a transient window.
- Overwrite through the production confirmation dialog succeeded:
  `6de825f1ede146d09e777473c5ac4936`.
- Saving `CaseCheck` with team entered as `backgroundaudit` retained
  `BackgroundAudit` in the file and meta identity:
  `986fb45ddaa1434f93e67ef84fca8132` (84 ms).
- Existing user `sniper.et` already referenced `M16A2_test_1.et`; its weapon file
  contained both the 4x20 optic and olive suppressor. Neither file was modified
  by this investigation. After development Reload, production Load succeeded
  (`56bd11b43dca4cceb5290d15eec1821d`) and live weapon contents confirmed both
  attachments (`067c2c136ac24b3e8e55382c8043e219`).

## Remaining limitation

Immediate Load of newly created `Scoped.et` still failed in its creation session
(`1bff7aa032c94efcb2c9667e04278ae0`). The optional
`GC_TestLoadoutResource` helper compares native Resource.Load and LoadContainer
without possessing the probe or writing files. A fresh CaseCheck source probe
contained the correct saved weapon, scope and suppressor but had an invalid
replication identity (`90304ea2bcea4aa9a264b2f7fe0c25d3`). Console lines
33889/33897 explicitly report “Attempting to insert an entity with invalid
prefab resource GUID”. Reading the source directly does not solve registration.

Therefore the reported missing weapon modifications are not reproduced as
missing disk data in these cases; same-session activation of new resource GUIDs
remains unresolved. No source replay, replacement base weapon or forced restart
was introduced. Removal of the build request is verified in source; a physical
foreground/flicker check has not been established by these tests.

Fixtures are limited to US/BackgroundAudit and are archived outside the addon
after testing. Tool receipts live under the tool repository's
`.temp/local/evidence/loadout-foundation`.

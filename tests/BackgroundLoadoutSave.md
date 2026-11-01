# Background save investigation — 2026-11-01

## Changes

`GC_LoadoutPrefab.Register` now authors the standard native `.et.meta` sidecar
without calling the interactive `ResourceManager.RegisterResourceFile` importer.
It uses `Workbench.GenerateGloballyUniqueID64`, preserves the native
`MetaFileClass` / `EntityTemplateResourceClass` PC and inherited platform
configuration structure, and reads the result through `GetMetaFile` before
returning its resource ID. It does not edit the resource database or introduce
another loadout format. Existing identities are not rewritten. No automatic
Stop, Reload or restart is added.

The earlier change to `RegisterResourceFile(path, false)` only disabled building.
The user subsequently identified the remaining popup as **Registering resource
1 of 1**, with Resource Manager coming forward. That importer call was still
interactive. Native Workbench 1.8.0.13 evidence at function `1402207f0` also shows
registration success/failure notifications independent of its build branch.
The public declaration exposes a build flag, not a quiet-registration flag.

Destination paths preserve existing faction/team directory capitalization by
looking at actual files. Windows accepts `Test` and `test` as one folder, but
the engine reported mismatched resource paths when a new meta file used `test`
inside the existing `Test` folder. Empty folders without prefabs do not supply
a canonical spelling through this lookup.

## Quiet identity creation verification

Compile `6cf5a84311544c1e91279502a9c77d23` passed. Existing ArlandLobby,
dedicated-plugin and formations diagnostics remained; this reload also logged
`ResourceBudget Entity: not consistent size`, without script compilation errors.

- Production Save `617ab42a689b4697a2f200e5efafbef2` created Silent and its native
  sidecars. `2cec528b40c4460e843877e29dcf7d2e` saved QuietScoped referencing the
  newly generated `M16A2_PopupAudit_2.et`, with an olive scope override.
- Native Qt suppression counters did not increase across either save:
  SetForegroundWindow 31, SetFocus 31, ShowWindow 606, SetWindowPos 629.
  The other counters stayed zero. Thus the tests did not merely hide newly
  requested registration windows. `882bc118ad3b49ca9fdc15029c9650d7` confirmed
  same character and session after Save.
- After stopping/starting the test session, production Load of QuietScoped
  succeeded (`c9f1a1190faf496aa92181d29193fc28`); weapon contents readback
  `ba1240dcbd1642d89bde27920d211555` confirmed the generated weapon identity and
  olive scope. The ordinary resource loader accepted the new metadata.
- Final save `44b214eff1784b1aa9e25c7eceecceae` ran with background focus suppression
  temporarily **disabled**, restored immediately afterward. It completed in
  72 ms. Captured gear-screen frames `1793526045555394500-capture_sequence.png`
  through `1793526045568410800-capture_sequence.png` showed no registration
  progress UI or interruption in the captured World Editor window.

## Earlier investigation evidence

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
was introduced. The earlier build-flag-only fix did not establish absence of the
popup; the later checks above cover the replacement identity-authoring path.

Fixtures are limited to US/BackgroundAudit and US/PopupAudit and are archived outside the addon
after testing. Tool receipts live under the tool repository's
`.temp/local/evidence/loadout-foundation`.

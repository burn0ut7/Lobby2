# Reusable weapon prefabs — 2026-11-01

Modified equipped weapons save native attachment overrides under
`Prefabs/Loadouts/<Faction>/<Team>/Weapons/<Weapon>_<Team>_<Number>.et`.
The weapon stem uses the original weapon's translated display name, restricted to
filename-safe letters/numbers/underscores/hyphens; spaces become underscores.
Team numbering is collision-safe, beginning at one. A matching existing variant
with the same immediate parent and attachment references is reused. Shared
variants are not overwritten when another unit customizes them. Unchanged saved
weapons remain direct unit-slot references; no extra copy is needed.

The weapon category uses the native item display name for authored weapons too,
so an M16A2 variant remains labeled M16A2. The numbered filename remains searchable
through the resource path. Mod provenance still comes
from native resolved source addons. On a successful weapon save, existing catalogue
instances queue new authored resources using their native meta identities. This
uses the existing metadata queue and incremental preview renderer. It does not
rebuild the page or restart Play. Initial discovery also includes these files.

## Verified behavior

Display-name follow-up: Reload `3f627114c957448ebdcec7b9e3edce46` passed.
Catalogue readback `b74472c7173546ce85fd2c223ddb21d1` found the saved
`M16A2_test_1.et` by filename and displayed **M16A2**. Screenshot
`1793523827233861900-capture_screenshot.png` confirms the visible row label.
The numbered files and native weapon references are unchanged by this label fix.

Workbench 1.8.0.13, dataset d9b5a4246f9247e4be5ea20935889885.
Original weapon-save compilation 97cf0d86e2714cfc9e183367700b9f32 passed without new errors.
Existing ArlandLobby/dedicated-plugin/formations diagnostics remain.

- Native M16A2 plus M9 bayonet saved as M16A2_WeaponAudit_1.et, inheriting the native
  rifle and writing only the bayonet slot. UnitOne referenced this new prefab.
  Menu save: a5f91f7a7aa546b29c483cf7fbcd1a1b. Repeating Save reused the same file:
  7caac2cf981d4aac9d8505ed23cefc88.
- Standard 4x20 scope saved as number 2; olive scope saved as number 3. These appeared
  in the already-open catalogue immediately after Save. No new game session was
  needed for discovery: 2518b3bb457b40f2883318c58a89a8c6.
- After a development Reload made number 2 available to native replication, native
  drag placed it in the secondary weapon slot. Live readback confirmed its 4x20
  optic, magazine, handguard and flash hider:
  7f8ef39b51da4f8cb9ec71c3858a33a3 / db15cea6ee8047eb95c399ab02440ddf.
- Saving another unit referenced number 2 directly and reused the unchanged olive
  configuration as number 3; weapon-file count stayed at three:
  2f5c66d11521463285d25133d6a86808.
- Source-label readback was Lobby2 (fa9297a11d2a4bdaa53f29d656e0c2ff). Automatic
  hover did not open the tooltip during this check; visible tooltip appearance is
  not claimed. Current screenshot shows all three filenames and equipped weapons:
  1793522437487195900-capture_screenshot.png in toolrepo evidence/loadout-foundation.

## Unfinished native limitation

New prefab GUIDs are not admitted to native inventory replication during the Play
session in which they were registered. Number 2's insertion reported invalid
rplID=0xFFFFFFFF; rebuilding the individual resource with the documented
ResourceManager.RebuildResourceFile API did not resolve it. That ineffective call
was not retained in production. No automatic Play/Workbench restart was added.

The transfer path now rejects invalid replication identities before locking the
inventory. A failed attempt with newly authored number 3 was immediately followed
by successful insertion of previously loaded number 2, verifying that the failed
attempt no longer poisons inventory interaction (4664995801644a8497fa2df66cfb1342
then 7f8ef39b51da4f8cb9ec71c3858a33a3).

Saving, numbering, discovery and later-session reuse are verified. Immediate
same-session insertion of a brand-new saved weapon remains unfinished. The earlier
inventory inheritance/stale-save findings in LoadoutInheritanceAudit.md are not
fixed by this change. Matching recursively compares customized child attachments
and verifies their immediate prefab ancestry; a separate live nested-attachment
fixture was not exercised in this pass.

## Evidence and reproduction

Existing optional catalogue/weapon/file-menu test helpers exercised production
handlers. Test source files were limited to the disposable US/WeaponAudit team;
user-authored loadouts were not overwritten. Fixture copies and receipts are in
toolrepo `.temp/local/evidence/loadout-foundation/weapon-variants`.
To reproduce, attach a bayonet or optic, Save a unit, inspect its WeaponTemplate
reference and corresponding weapon file, repeat Save, then select Weapons and
search the team name. Test a preloaded saved weapon separately from a new GUID.

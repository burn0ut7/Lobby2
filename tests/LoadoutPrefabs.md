# Workbench loadout files

Press S in the explicitly opened local gear editor. Current gear includes unsaved
inventory edits. Selecting a saved row changes the source to that file and previews
it without equipping it. Save always writes current gear to the named destination;
Load equips the selected saved row.
Inherit and Duplicate accept either source. Enter a new name for either creation
action. Saving an existing destination asks Continue/Cancel before overwriting it,
including when the saved row is selected. Cancel preserves both gear and file.
Close returns without writing. Successful actions reopen the gear editor.

The role-icon selector lists the 28 named character glyphs from the native
`LoadoutIcons.imageset`, A–Z by displayed label, with each glyph visible in its row.
A source's custom image is retained as an additional Current icon choice. The name
field also writes the editable-character UI name used by lobby roles. Faction must
be a key registered in the current world's faction manager; saving writes native
character affiliation, editable UI faction and faction labels, retaining unrelated
labels. These fields apply to Save, Inherit and Duplicate; selecting a row or Load
does not author metadata. Unchanged values remain inherited; choosing the parent's
value again clears a child override. Team remains a folder name.

Native files live at `Prefabs/Loadouts/<Faction>/<Team>/<Name>.et`. Inherit points to
the source prefab; Duplicate retains the source's existing ancestor. Gear slots
write only changed native resource references. Changed weapon attachments use native
weapon variants in the adjacent Weapons directory. Stable weapon identities preserve
attachment inheritance; replacing a weapon allocates a different variant. Variants
are owned by the exact team directory, loadout name and slot.

Workbench caches resources during embedded Play, including newly registered GUIDs.
After a successful file action the Workbench plugin briefly stops and restarts the
preview, then possesses the selected native prefab and opens its inventory. This
resets runtime-only world state. The one-shot profile handoff contains only the
prefab identity, project/world and character transform, never serialized gear.
Ordinary Play starts/stops do not activate the editor. Generation and world/project
checks cancel interrupted handoffs. This authoring service is Workbench-only and
requires the explicit local Lobby2 authoring session; network play is rejected.

All slots and contents are validated before writing existing weapon dependencies.
A disk failure while publishing multiple native files can be partial; the message
identifies this and retains current gear for retry. Newly created targets remain
selected after writer failure. Failed resource registration removes its new files
or reports the remaining path explicitly. Native array/property inheritance rules
apply; this is not a runtime state, ammunition-count or health save system.

## Reproduction

### Metadata verification (2026-11-01)

Workbench 1.8.0.13; native source revision `d9b5a4246f9247e4be5ea20935889885`.
The faction field is `SCR_CharacterFactionAffiliationComponent`'s
`faction affiliation`. `SCR_EditableCharacterComponent.m_UIInfo` supplies `Name`,
`Icon`, `IconSetName`, `m_sFaction`, `m_aAuthoredLabels` and `m_aAutoLabels`.
`SCR_Faction.GetFactionLabel()` supplies the faction label. Native array setters
accept numeric enum values, not their symbolic names.

- Production Save created a disposable USSR/MetadataQA/MetadataBase from a US
  rifleman with Medic icon (`a7de6d87ad074a20becb56fd777de39c`). Disk readback
  showed the new name, icon, USSR affiliation/UI faction and FACTION_USSR;
  GROUPSIZE_SMALL and ENTITYTYPE_CHARACTER remained. No gear overrides appeared.
- Current-gear Inherit (`5123a1d673bd4d11a82ea287203468c6`) created MetadataChild
  with only its Name override. Confirmed Save changed its icon to Engineer
  (`f100238bbc33492fb5a30a825d9ababc`); choosing Medic again cleared that override
  (`799a41371f9e4e1d822e3454de69c9f9`). Base SHA256 stayed
  `DCE33802D7A2F37EF59BF0398DF58C4A3F3CBF8D5DAC6C1016852C6091BBFC44`.
- Saved-row Duplicate (`80ef70f6fcca497088c59032ff7850b2`) retained the native
  ancestor and metadata, with the new name MetadataCopy.
- An unknown faction was rejected before file creation; the menu stayed open
  (`597e7eb8b5ce47098816e1b07649800c`). User-authored US/Test files were not saved.

`GC_TestLoadoutIcon(index)` opens the native combo and invokes its native row button;
-1 leaves it open for inspection. `GC_TestLoadoutMetadata(prefab)` reads native
source metadata. Existing runtime-refresh behavior is unchanged by this feature.
Final Reload `06952b695da94c71a9def83f53e5ec40` introduced no new script errors.
Native dropdown/closed-state captures in the tool repository's
`.temp/local/evidence/loadout-foundation` are
`1793514753052702200-capture_screenshot.png` and
`1793514763851584900-capture_screenshot.png`. The list opens above the field while
retaining top-to-bottom alphabetical order; native row selection updates the glyph.

### In-session prefab investigation (2026-11-01)

The restart is implemented by `GC_LoadoutRefresh` / `GC_LoadoutRefreshPlugin`,
not required by `WorldEditorAPI.SaveEntityTemplate` itself. A direct save and
local spawn of an already registered prefab reflected changed inventory in the
same Play session (operations `76f8f69d761843af87b171577c9a9446` and
`ac4fe20324f146759f08b96b45c6ee7a`). That does not establish support for new IDs.

Creating a new native prefab during Play reproduced an RPL invalid prefab GUID
error. `SetControlledEntity` returned true, but `GetControlledEntity` did not
resolve the new character. `ResourceManager.WaitForFile(path, 1000)` timed out;
the subsequent local spawn still failed possession (operation
`9643ed4441714fa3994bc3810cd9e80a`: acknowledged=0, assigned=1, matches=0).
Thus checking only save success, spawn success or the setter's boolean is unsafe.
The timeout does not establish whether a longer or deferred acknowledgement can
succeed; that was not verified.

Installed declarations expose registration/building and database acknowledgement,
but no inspected API establishes a safe in-Play refresh of new replication IDs.
This is a bounded finding, not proof that no native solution exists. Removing the
restart alone is insufficient. The experimental replacement was reverted; the
existing production behavior remains. A future change must preserve native weapon
identity/inheritance as well as test new-file creation, not just existing-file load.

Overwrite verification (2026-11-01, Workbench 1.8.0.13): selected a disposable
saved prefab while editing a different character. Cancel retained its SHA256
`4C18934D9CD1683FA017696295AD2AAAB2D4581B09026DFF01622844E653E4C3`
and left the files menu open (operation `813690eb46a7444abb16352e23c37b12`).
Continue wrote current gear, preserved GUID/ancestor, and returned to inventory
(`0586040ac1cb48e89786a1ddaf1789d1`, `548a57dcc3b24ebab538da8e6842ea8c`).
An added bandage was persisted without an unchanged M16 override; overwriting
again without that bandage cleared its resource entry (native empty configuration
structure remains). Popup capture: toolrepo `.temp/local/evidence/loadout-foundation/1793512325113001100-capture_screenshot.png`.
The optional `GC_TestLoadoutConfirm("confirm" | "cancel")` exercises native dialog
button dispatch. User-authored prefabs were not overwritten.
Final Reload `e8777235647245d9a606479636d4b13e` passed without new script errors;
the existing ArlandLobby, dedicated-server-plugin and formation diagnostics remain.
Disposable files were removed and the user's saved base restored to the gear screen
(`a3b157a6fbfa4bdd83b47c1e4dc03ee2`). Independent review approved the changes.

Load the optional MCP test addon in `worlds/examples/MPTest/Empty_Lobby.ent`, start
Play, call `GC_TestNativeInventoryStart`, then `GC_TestCatalogueOpen`. The test
component is `GC_LobbyManager` on `GameMode_Lobby3`.

- `GC_TestLoadoutMenu(action, faction, team, name)` opens the actual menu action and
  invokes its production button. Empty fields leave existing text unchanged.
- `GC_TestLoadoutChoose(name)` clicks the saved row.
- `GC_TestLoadoutFlowState()` reports actual/editing prefab, menu and active state.
- `GC_TestLoadoutSnapshot()` reads the native arsenal's transient inspection output.

After each successful action wait for the automatic preview restart, then inspect
flow state and native contents. Do not treat the initial button return as completion.
Use a disposable team directory and inspect source files as well as the live entity.

## Verified 2026-11-01

Workbench 1.8.0.13, PID 44368; installed-source revision
`d9b5a4246f9247e4be5ea20935889885`. Evidence receipts and unmodified MCP captures are
in the tool repository's `.temp/local/evidence/loadout-foundation`.

- Save new and overwrite, Load, Current gear Inherit/Duplicate and saved-source
  Inherit/Duplicate returned actual=editing prefab, active=1, files=0, gear=1.
- Removing a helmet then inheriting wrote only an empty Hat override. Unchanged
  M16 had no separate variant. Editing the base helmet propagated to an existing
  role whose source remained parent-only.
- A dragged jacket bandage survived Save/Load alongside the original pants/vest
  contents. A configured M16 retained its bayonet after Save/Load.
- A child weapon added an M9 bayonet while inheriting an empty magazine from its
  base. Restoring the base magazine retained the base weapon GUID, cleared its
  MagazineTemplate override, and propagated to the child while preserving its
  bayonet (`2cacc035d3bf432ca9fb8c7217df9ab4`).
- Same-name inheritance into another team created a child weapon in that team's
  directory. The original base weapon SHA256 remained
  `6819BC9489007FA4DC7CF7EDCB4DE8EE779D4093775488A4372BF88C61619ED1`.
- Replacing the customized M16 with a customized AK-74 saved and reopened correctly
  (`b49aefcaa5b046ebb8661b49791df10c`, `e0692473ec364defaacd6794d9a51b3b`).
- Reserved name CON was rejected; saved-source Save was rejected without creating
  a file; disabling authoring prevented the file menu from opening. Closing/reopening
  retained native inventory behavior. Ordinary script Reload left Play stopped.
- Large native preview and source selection inspected in captures
  `1793509234468372500-capture_screenshot.png` and
  `1793509891919609200-capture_screenshot.png`; independent review approved.

The final compilation was `7a71e740604b492e8b12662d385b440f`, with no new
script errors. Existing ArlandLobby resource, dedicated-server-plugin and formation
diagnostics remained. Disk-full/registration failure injection and a new addon-free
multiplayer baseline were not run; this slice does not implement live admin saving.

Grayscale UI refinement (2026-11-01): black modal, white headings and outlined
fields, gray native button states with a white selection edge, editable-prefab
role icons, and native blur over the retained gear screen. Current/saved source
captures are `1793510403326104600-capture_screenshot.png` and
`1793510409272432600-capture_screenshot.png` in the same evidence directory.
Reload `ec0fd465791943518c3cfafb9548cceb` introduced no script errors.
Load (`e57e72c2449a435bb4469e204cad8c4b`) and Save
(`79dfe106c1d94a9cad3dcbe90ccac7ff`) outcome checks confirmed actual=editing,
active=1, files=0, gear=1. Independent source/visual review approved.

## Native random variants (2026-11-01)

The same Save/Load screen shows native entries indented under each parent, labeled
only 01, 02, 03. Selecting an entry previews its referenced character and shows
Load/Delete below. Save appears for pending edits; Close discards them. Delete
requires the native confirmation and removes the reference, never the prefab file.
The user explicitly removed enabled/disabled, clear-all and extra cancel controls.
Existing native weights, enabled flags and default-character participation remain
preserved internally. Add variant defaults to current gear; the picker also offers
saved loadouts and references already present in that prefab.

Saving current gear creates an inherited concrete character beside its pool using
`<FACTION>_variant_<lowercase_pool_name>_<number>.et`; occupied names advance the
number instead of overwriting. The new character inherits gear normally and has
an empty local randomizer. The parent stores its resource in the native
`SCR_EditableCharacterComponent.m_VariantData.m_aVariants` list. All writes use
Workbench source authoring and `SaveEntityTemplate`; there is no custom runtime
loadout serialization. Saving variant data does not restart Play. Loading continues
to use the existing prefab-load lifecycle described above.

Installed source evidence, revision `d9b5a4246f9247e4be5ea20935889885`:

- `game/data/data007.pak/scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityVariantData.c`:
  native prefab reference, enabled flag, weights and default inclusion.
- Same directory, `SCR_EditableEntityComponentClass.c:23–97`: native weighted
  selection and validation; already consumed by Lobby2's lobby/spawn flow.
- `game/core/data.pak/scripts/Core/generated/WorkbenchAPI/WorldEditorAPI.c:247–283`:
  owning source paths and object authoring. Removing an inherited array member
  crashed Workbench 1.8.0.13 in isolated testing. The final writer replaces the
  local variant-data object instead; equality with the ancestor clears its override.
  Even equality with current memory still calls SaveEntityTemplate, so a failed
  disk save cannot be mistaken for success on retry.

Live production-handler checks used disposable `US/VariantsQA` prefabs:

- Add, replace and save references; native readback and `.et` text agreed.
- Delete confirmation cancellation left data unchanged; confirmed deletion plus
  Save shortened the list. Deleting the final entry saved count=0, while its native
  ancestor retained count=1. Returning to the ancestor's list removed the override.
- Newly created files included `US_variant_pool_2.et` and `US_variant_pool_3.et`.
  Removing reference 03 retained that file's SHA256; creating again chose `_4`.
- Replacing a reference preserved its native weight/enabled values. Those controls
  were subsequently removed from the UI per the user's final simplification.
- Close discarded pending replacement. Dropdown Back returned menu=1, picker=0,
  pending=1 (`7b03998606ab4baea0ed0c5fccfe7287`). Inactive authoring rejected and
  closed the menu without exceptions (`0c20b488104c4266aa13a5b98f1cfd90`).
- Final compilation `df184fe63d164362af537a7a2bd77002` introduced no script errors;
  existing resource/plugin/formation diagnostics remained. Final screenshot files
  in the tool evidence folder: `1793516705593576300-capture_screenshot.png` (selected),
  `1793516707003416000-capture_screenshot.png` (pending),
  `1793516724450079900-capture_screenshot.png` (confirmation).

Independent source and visual review approved after keyboard-dismissal,
early-teardown and persistence-retry fixes. Forced disk-failure injection and
multiplayer were not tested; authoring remains local Workbench-only.

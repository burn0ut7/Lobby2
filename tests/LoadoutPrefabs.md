# Workbench loadout files

Press S in the explicitly opened local gear editor. Current gear includes unsaved
inventory edits. Selecting a saved row changes the source to that file and previews
it without equipping it. Save always writes current gear to the named destination;
Load equips the selected saved row.
Inherit and Duplicate accept either source. Enter a new name for either creation
action. Saving an existing destination asks Continue/Cancel before overwriting it,
including when the saved row is selected. Cancel preserves both gear and file.
Close returns without writing. Successful actions reopen the gear editor.

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

# Mission metadata

## Author workflow

1. Edit the mission header: choose Mission Type, Objectives and Player Assets. The seven types are CO-OP, TVT, COTVT, Training, LOL, Testing and Broken. Player Count on the native header is the only authored capacity. Minimum/recommended counts and manual factions are hidden; factions and playable slots come from world export. Manual Tags, Map Name, Required Addon IDs and grouping fields are no longer shown; their old serialized values remain readable for compatibility.
2. Save the header in Resource Manager. The world action does not save separate header tabs.
3. In World Editor select the mission subscene, then choose **Plugins > GC > Save Mission and Metadata**. This saves the world first and refreshes `<world>.ent.metadata.json` beside it.

Editing, adding or resetting catalogue properties never requests a scan. Opening worlds and registering resources no longer request scans either. The legacy scanner component remains only to preserve existing prefab serialization; it does no work.

Ordinary Ctrl+S saves normally but does not refresh metadata: the installed Workbench scripting API exposes no general save callback. Use the combined action when publishing changed mission content. There is no timer, filesystem polling or global entity override.

The collector visits only the selected mission subscene and its prefab children, excluding inherited terrain scenery and other subscenes. It refuses terrain scenes and Play/prefab modes. Progress is shown during collection. Guards abort collection at 100,000 entities, 128 nesting/inheritance levels or 30 seconds; failed collection does not replace the previous snapshot. These are cooperative guards between engine calls, not a guarantee against a stalled native call. The action is synchronous; large mission performance still needs measurement.

The generated file contains content evidence, not authored objective tags or a complete dependency list. It does not edit the mission header. Include it with the published mission files if the website should use it. Packed-addon inclusion has not been verified.

## Current verification (2026-11-03)

Script reload passed. With the Arland mission and inherited terrain loaded, the combined action saved successfully and generated valid JSON for 25 mission entities. Property edit callbacks and registration callbacks have been removed. The reported native config reset/plus freeze has not yet been isolated; removal of automatic scanning is not proof that the editor issue is fixed.

The verification sections below describe historical implementations and do not describe current triggers.

## JSON contract

| Field | Meaning |
| --- | --- |
| `schemaVersion` | Currently `1`. |
| `world`, `scope`, `limitations` | Snapshot identity and explicit coverage limits. `scope` is `world_subscene`: the save action uses the selected mission subscene. Earlier exports used `active_subscene`. |
| `entityCount` | Entities visited, including nested helper entities. Not a unit count. |
| `factions` | Unique nonempty placed affiliation, playable-group and authored loadout faction keys. No friendly/enemy inference. |
| `playerSlots` | One row per authored playable-group slot: group name, zero-based index, group faction, character prefab, loadout name and loadout faction. AI-only groups are excluded. Random variants, runtime changes and inventory contents are not expanded. |
| `configuredFactions` | Faction keys configured in faction managers in the scanned subscene, reported separately from actual affiliations. |
| `assets` | Counts by detected category. Mounted vehicle subentities are not counted as independent vehicles/weapons. |
| `prefabs` | Inventory grouped by prefab, entity class, effective faction, asset and classification evidence. Each row includes `count` and `sourceAddonIds`. Entities without prefab ancestry have an empty prefab reference. |
| `referencedAddonIds` | Unique addons defining/modifying placed prefabs or their prefab ancestors, as reported by the engine. Includes vanilla content. |
| `loadedAddons` | Loaded addon IDs, GUIDs, titles, vanilla flag and `referencedByPlacedPrefab`. Merely being loaded does not imply placed content use. |

The website can consume this JSON directly. Rows and summaries are sorted for stable diffs. Existing mission-header metadata is not changed.

## Asset recognition

The exporter reads resolved sources, so ordinary prefab inheritance and instance faction/type overrides apply. It does not classify by filename substrings.

- Infantry: `ChimeraCharacter` inheritance.
- Helicopters: `VehicleHelicopterSimulation`, or the native AI usage type.
- APCs, cars, trucks, mortars and support vehicles: native `EVehicleType` values.
- Airplanes, static weapons/artillery and otherwise unclassified tracked vehicles: native AI usage type.
- Boats: verified RHIB base ancestry (`{A295678AF2AE1205}Prefabs/Vehicles/Naval/RHIB/RHIB_Base.et`), or an authored `VEHICLE_BOAT` editor label when supplied by a loaded mod.
- Tanks: an authored `VEHICLE_TANK` editor label when supplied by a loaded mod. Tracked movement alone does not imply a tank.
- Other vehicles remain `vehicle`. An empty asset category means the row is not classified as a gameplay asset.

`assetEvidence` records the rule used. Mod authors can configure incorrect types/labels; these categories describe authored configuration, not independently verified capabilities. Boat/tank enum names are resolved dynamically, so the plugin does not require those mods. The RHIB rule is an exact ancestry mapping; buoyancy alone would incorrectly classify amphibious APCs as boats.

## Deliberate limits

This is placed-content metadata, not a complete mod dependency analyzer. Script-only mods, components' resource references, loadout inventory, random variants, catalogs and runtime spawners are not recursively expanded. Playable-group slot prefab references are included. The loaded-addon list preserves context without claiming those addons were all used by placed prefabs. No era, difficulty, mission purpose or player availability is guessed. No positions or mission-header fields are exported.

## Verification

Workbench 1.8.0.13, optional `Lobby2McpTests` addon:

- `worlds/MetadataTest.ent`: two BTRs (one instance changed to US), a prefab-inherited UH-1H and US scout, and a US/USSR faction manager. Native menu export produces APC=2, helicopter=1, infantry=1; the APC rows retain separate US/USSR affiliations.
- Nested entities retain GUID-qualified prefab references, including vehicle probe children. The flat editor entity enumeration alone would miss inherited children.
- The test-addon composition is attributed to its engine-reported source addons. Loaded TilW and isolation-probe addons are not marked referenced by placed prefabs in this fixture.
- Exported JSON is parsed and checked externally. Script compilation passes. Actual mod boats/tanks have source-based rules but are not live-tested: their addons are not loaded in this Workbench project.
- Empty mission export produces empty inventory/faction/addon-use collections. Export leaves the editor undo history unchanged.

The fixture is editor-only; no Play session is needed. Test metadata JSON is generated output, not a checked-in expectation.

## Earlier native mission-config experiment (2026-11-01)

`GC_MissionHeader : SCR_MissionHeader` adds an optional `m_GC_GeneratedMetadata` container. `GC_MissionMetadataExport.CreateSnapshot` converts the existing collector's results to native attributed containers: schema version, world, capture scope, entity count, effective/configured factions, assets, prefab inventory and addon provenance. These types are included in Lobby2; no runtime scan runs. **Production Lobby2 does not automatically populate or refresh this field.** Existing JSON export behavior is unchanged.

The creation experiment lives in the optional test addon. Its plugin registration attribute is commented out, so it adds no menu or automatic callback by default. The experimental callback is restricted to the exact `Missions/MetadataAutomatic4.conf` fixture before loading any resource. It requires a nonempty World matching the open mission, preserves existing metadata, and does not assign the mission for the author.

### Observed lifecycle behavior

Workbench 1.8.0.13, installed source catalogue revision `721cd7b05eb54d3ca3591d9097af381d`:

| Action | Result |
| --- | --- |
| Add a nested container in the native config editor | The script constructor did not run. Explicit script `new` was a positive control and did run it. |
| Save the edited config with native File > Save | Persisted the container; no registration/build callback observed. |
| Add an entity and save the mission world | No registration/build callback observed. Test entity was undone and the world saved again. |
| Create a header and call native `RegisterResourceFile` | `OnRegisterResource` ran and embedded metadata without calling the exporter action. |
| Read the saved native metadata as a script instance | Nine entities, three asset summaries, eight prefab rows and six loaded addons survived readback. |

The registration callback runs before the new resource GUID is fully available. Loading the newly registered config during that callback emitted `Wrong GUID/name` diagnostics even though serialization succeeded. A path-only load did not resolve this. This is an unresolved lifecycle issue, **not a production-supported automatic creation workflow**. The experimental hook is therefore inactive by default. Saves do not refresh snapshots, and adding an empty metadata container does not initiate scanning.

Evidence: `logs_2026-11-01_17-45-51/console.log`, creation lines 31378-31384; native readback line 31536. The final fixture-restricted callback also passed creation/readback at lines 31687-31693, with its known GUID diagnostics. `MetadataAutomatic3.conf` recorded APC=2, helicopter=1, infantry=1, US/USSR affiliations and unchanged name/author. Generated outputs are local test artifacts, not published mission configs. Existing stale Arland resource and temporary reload-snapshot `Parent` diagnostics are separate from the registration GUID issue; script compilation passed.

Relevant installed declarations: `ResourceManagerPlugin.c` registration/build callbacks, `WorldEditorPlugin.c` game/drop callbacks, and `Core/proto/Types.c`'s empty `ScriptAndConfig`. Native `GameModeSetupConfig.GenerateMissionHeader` (lines 212-266) saves and registers its header explicitly. No general config/world-save callback was established by this investigation. The next integration point to evaluate is the owning mission-header creation workflow, where the header container is already available before saving; it should call the same collector directly rather than reload a resource during registration. That would cover creation, not ordinary later saves.

### Reproducing the isolated experiment

Load the optional test addon and open `$Lobby2McpTests:worlds/MetadataTest.ent`. Temporarily uncomment the single plugin attribute in `GC_MetadataCallbackProbe.c`, Reload, then invoke Resource Manager > Plugins > GC Tests > Metadata callback probe. The helper creates only `MetadataAutomatic4.conf` and refuses to overwrite an existing file. It uses the native save/register sequence, then checks native snapshot readback. The registration callback, not the creation helper, collects metadata. Expected fixture counts are APC=2, helicopter=1, infantry=1, total entities=9. The known registration GUID diagnostics remain expected evidence of the incomplete experiment. Comment the attribute again and Reload afterward. Do not publish the test addon.

## Earlier periodic prototype verification (2026-11-01)

These tests describe the preceding periodic implementation; the event-driven revision below supersedes its trigger behavior.

Workbench 1.8.0.13; catalogue revision `721cd7b05eb54d3ca3591d9097af381d`. The installed `GenericComponent` declares `_WB_AfterWorldUpdate` as an editor callback after world updating; `_WB_GetAfterWorldUpdateSpecs` controls its scheduling. That earlier version used `CALL_ALWAYS` with a ten-second timer; it has now been replaced by the event-driven dispatch described above. Enumeration is restricted to the captured subscene via `GetEntityCount(subscene)` / `GetEntity(subscene, index)` before recursively visiting prefab children; it does not traverse the inherited terrain inventory. The collector moved to the Game script module under `WORKBENCH` so this native component callback and the optional Workbench plugin share it.

- Opening the saved MetadataTest fixture automatically wrote nine entities, APC=2, helicopter=1, infantry=1, US/USSR factions and loaded/referenced addon records. No metadata action was invoked.
- Placing another BTR through native World Editor increased APCs to three and visited entities to eleven on the next automatic pass. Native Undo returned APCs to two/entities to nine. The scanner added no undo actions.
- Subsequent unchanged cycles preserved the output modification time. Embedded Play ran without metadata writes; edit-mode refresh resumed afterward. The generated file can be parsed as JSON.
- Source data remains a snapshot, not an exhaustive dependency list or a guarantee of player-available assets. Existing classifier limitations above still apply.

Evidence: `logs_2026-11-01_17-45-51/console.log`, initial/add/undo automatic writes at lines 32198-32200 and subsequent `GC | Automatic Metadata` entries. Post-optimization Reload `2231626b36c04e2e9ffb28f1747f4cbe` passed and automatic JSON readback remained nine entities, eight prefab rows, APC=2, helicopter=1, infantry=1 and US/USSR factions. The earlier registration experiment remains disabled and is not part of this path. Normal script Reload compiled the moved collector and scanner without script errors; pre-existing stale Arland resource diagnostics remain.

## Earlier event-driven revision verification (2026-11-01)

`GC_MissionMetadataScannerComponent` previously queued a single pending refresh from its native `_WB_OnInit`, `_WB_OnCreate` and `_WB_OnKeyChanged` callbacks. `GC_MissionMetadataPlugin.OnRegisterResource` also requests a refresh for root scanners in the active subscene when a `.conf` is registered. It never loads or edits that resource. `_WB_AfterWorldUpdate` dispatches queued work after editor mutation/undo finishes. `CALL_ALWAYS` is needed to service off-screen requests, but idle callbacks immediately return on the pending flag. There is no elapsed-time condition, repeated scan or file polling.

In Workbench 1.8.0.13, opening MetadataTest produced one refresh (nine entities/APC=2). Placing another BTR did not trigger collection. Creating two config resources in the same native invocation then produced one combined refresh (eleven entities/APC=3). The new configs retained their authored name, author and manual tags; the callback caused no resource-ID diagnostics. Undoing the other entity did not trigger collection either; the snapshot stayed unchanged during the following idle period. This deliberately demonstrates both event dispatch and its limited coverage, rather than claiming every edit is observed. Undo history still contained only the author's entity action.

Evidence: `logs_2026-11-01_17-45-51/console.log`, initial refresh line 33013; native registration lines 33018-33020; combined refresh line 33021. More than 50 seconds afterward, no additional refresh log appeared and the file modification time was unchanged. Disabling the scanner through the native property editor caused no scan; Undo restored it and produced one initialization refresh with nine entities/APC=2 at line 33022. Script compilation passed (pre-existing Arland resource errors remain).

Reproduce the config event with optional `GC_MetadataEventProbe.c`: temporarily uncomment its plugin attribute, Reload, open MetadataTest, place an additional BTR, then invoke Resource Manager > Plugins > GC Tests > Metadata event probe. It creates two test configs using native save/register calls and never calls the scanner. It refuses replacement of either existing output. Comment the attribute and Reload afterward; the helper adds no tool by default. Generated test configs are local artifacts, not published resources.

### Derived slot verification (2026-11-03)
The production Save Mission and Metadata action on Arland exported 25 mission entities and 28 slots across eight playable groups. Inherited group slot arrays and character UI names/faction affiliations resolved; unnamed groups receive export-local Group N labels. Website intake retained all 28 rows. Script compilation passed; loaded-world reload still reports the existing stale ArlandLobby m_World resource reference. Native inherited .conf fields are unchanged: no supported GC-only category override was established.


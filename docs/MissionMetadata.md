# Mission metadata export

In World Editor, select the mission subscene and choose **Plugins → GC → Export Mission Metadata**. The plugin writes `<world>.ent.metadata.json` beside that subscene's local world file, replacing the previous generated export. It does not change or save the mission. Save the world and export again before publishing; the snapshot includes unsaved editor changes.

Version 1 deliberately scans **all layers of the active subscene**, including nested prefab entities, but excludes inherited/other world subscenes. This avoids exporting the base terrain's scenery. Select each mission subscene separately if a scenario is split across worlds. Selecting the terrain-owning subscene is refused. There is no website upload or runtime component.

## JSON contract

| Field | Meaning |
| --- | --- |
| `schemaVersion` | Currently `1`. |
| `world`, `scope`, `limitations` | Snapshot identity and explicit coverage limits. |
| `entityCount` | Entities visited, including nested helper entities. Not a unit count. |
| `factions` | Unique nonempty effective faction-affiliation keys on placed entities. No friendly/enemy or player-side inference. |
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

This is placed-content metadata, not a complete mod dependency analyzer. Script-only mods, components' resource references, equipment/loadout slots, catalogs and runtime spawners are not recursively expanded. The loaded-addon list preserves context without claiming those addons were all used by placed prefabs. No era, difficulty, mission purpose or player availability is guessed. No positions or mission-header fields are exported.

## Verification

Workbench 1.8.0.13, optional `Lobby2McpTests` addon:

- `worlds/MetadataTest.ent`: two BTRs (one instance changed to US), a prefab-inherited UH-1H and US scout, and a US/USSR faction manager. Native menu export produces APC=2, helicopter=1, infantry=1; the APC rows retain separate US/USSR affiliations.
- Nested entities retain GUID-qualified prefab references, including vehicle probe children. The flat editor entity enumeration alone would miss inherited children.
- The test-addon composition is attributed to its engine-reported source addons. Loaded TilW and isolation-probe addons are not marked referenced by placed prefabs in this fixture.
- Exported JSON is parsed and checked externally. Script compilation passes. Actual mod boats/tanks have source-based rules but are not live-tested: their addons are not loaded in this Workbench project.
- Empty mission export produces empty inventory/faction/addon-use collections. Export leaves the editor undo history unchanged.

The fixture is editor-only; no Play session is needed. Test metadata JSON is generated output, not a checked-in expectation.

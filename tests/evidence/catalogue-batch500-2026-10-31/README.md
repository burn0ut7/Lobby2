# Catalogue batches of 500

Request: increase indexing to 500 resources per frame while keeping preview rendering spread out. Changed the metadata cap from 200 to 500 and refresh the completed results immediately instead of waiting for the periodic 0.5-second partial-results refresh. Visible previews remain one request per frame; discovery remains one addon per update.

Same Workbench process 44368, version 1.8.0.13, Empty_Lobby.ent, existing loaded addons. Existing GC_TestItemCatalogueStart/Status helper ticks the production index through the call queue. These are warm-resource measurements, not cold-start guarantees or an FPS-controlled benchmark.

| Batch | Updates | Elapsed | Maximum index update |
| --- | ---: | ---: | ---: |
| 200 | 104 | 1350 ms | 153 ms |
| 500 | 45 | 1059 ms | 126 ms |

Both runs processed 19,485 prefabs and recorded exactly 2,009 inventory-component entries, 17,467 non-items and nine unreadable resources. Existing partial-coverage warnings for two test-addon searches and nine resources remain unchanged. The UI finished with 813 portable matches and 11 visible previews. The half-second aspiration is **not achieved** in this measurement: the cap requires 39 metadata updates plus six discovery updates. No items or addon roots were excluded to improve the result.

Reload be879b4ac1aa435c8b1a66f58b03efad completed with no SCRIPT compilation errors; existing resource/plugin diagnostics remained. Browser restored and open. No input, transfer, mode ownership or layout changes.

Bacon comparison (installed archive, PAK revision d9b5a4246f9247e4be5ea20935889885): `addons/BaconLoadoutEditor_606B100247F5C709/data.pak/scripts/Game/BaconLoadoutEditor/GunBuilderUI_Cache.c` lines 36–65 initializes from `GetFilteredArsenalItems` and caches details; `GunBuilderUI_Modded_ArsenalManagerComponent.c` lines 13–54 iterates faction arsenal catalogues. This avoids our broader all-prefab discovery but would not satisfy this browser's all-inventory-resource coverage.

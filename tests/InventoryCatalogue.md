# Workbench inventory catalogue

The local development browser replaces the native Tab inventory's left Vicinity area with a larger item catalogue. The native Inventory20Menu, character preview, equipment slots, storage grids and quickbar remain. This is a Workbench-only tool, not the future multiplayer admin arsenal or saved-loadout editor.

## Use

In local Workbench Play with active `GC_LobbyGameMode` and a controlled character, call `GC_WorkbenchItemBrowser.Open()`. Production has no dependency on the test addon. `Disable()` restores the normal left panel. The opt-in survives closing/reopening for the same character; controlled-entity changes and game end release it. Packaged game builds omit the hooks. Multiplayer and non-Lobby2 modes reject activation and transfers.

The expanded panel has nine rows, each with an item preview and its category above its name. The category strip filters all portable gear, weapons, headwear, clothing, backpacks, vests, equipment, magazines and attachments. Search combines lowercase whitespace-separated terms across names, localized category, resource identity, equipment slot, contributing addon and observed faction membership. All portable candidates are shown by default. Missing display-name translations fall back to the prefab filename. Filters/sort opens faction and alphabetical controls without shrinking the result list. Faction means membership in a loaded-world item catalogue, not inferred texture allegiance.

The native arsenal `SCR_SpinBoxPagingComponent` provides Q/E action hints and paging buttons. Mouse wheel over the panel changes the same page. Paging pauses while the search field or a combo list has focus, so Q/E can be typed. Clicking native helmet/backpack/clothing or weapon slots filters matching items.

Left-click equips into a compatible equipment slot, preferring an empty slot; otherwise it replaces compatible gear or inserts into a deposit container. Native replacement preserves the displaced item on the ground. Dragging uses the native inventory drag action, threshold and cursor preview, then inserts a new instance into the selected compatible equipment slot or container. Dragging onto the character uses automatic placement. Invalid targets and cancelled drags create no gameplay item. Quickbar assignment remains native inventory behavior after insertion; a catalogue drop onto a quickbar shortcut is rejected.

Only enabled, visible, draggable inventory-bearing prefabs are candidates; vehicles and actors are excluded. Base variants can still be valid candidates. Preview requests are limited to visible rows, one per frame; a changed page cancels obsolete work. Indexing keeps the four-resource/two-ms batch bound, though one resource load is indivisible. Partial source coverage is displayed honestly. Gameplay entities are created only for a validated placement; native inventory callbacks update the screen and remove a failed uninserted instance.

## Reproduce through MCP

Use `McpTestAddon/addon.gproj`, `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent`, `GameMode_Lobby3` / `GC_LobbyManager`:

1. `GC_TestNativeInventoryStart()` creates/possesses a stock rifleman and opens native inventory; `GC_TestCatalogueOpen()` enables the production browser. Poll `GC_TestCatalogueState()` until finished.
2. `GC_TestCatalogueSearch(string)` changes the edit field. `GC_TestCatalogueButton(name)` clicks attached handlers (CategoryWeapons, CategoryHeadwear, CategoryAll, ClearFilter, Advanced). `GC_TestCatalogueOption("Faction"|"Sort", index)` opens and clicks a native combo option. `GC_TestCataloguePage()` reports result identities.
3. `GC_TestCataloguePager(bool next)` clicks the native page button; `GC_TestCatalogueWheel(int direction)` exercises the wheel handler. `GC_TestCatalogueSearchFocus(bool)` tests typing isolation. `GC_TestCatalogueSlot(0)` and `GC_TestCataloguePrimary()` report real widget hover then invoke native selection.
4. `GC_TestCatalogueClick(row)` uses the row's native button handler. `GC_TestCatalogueDrag(row, equipmentRow)` reports movement and calls production drag handlers; -1 cancels, 0 targets the head, 100 targets weapon slot 0. `GC_TestCatalogueInventory()` reports actual attached resources. `GC_TestCatalogueOldHead(bool capture)` proves replacement preserves the old item. `GC_TestCatalogueRejectDisabled()` disables ownership then invokes the production transfer service.
5. `GC_TestCatalogueClose()`, `GC_TestNativeInventoryStart()` and `GC_TestCatalogueOwnerLoss()` exercise close/reopen and ownership loss. `GC_TestCatalogueCleanup()` restores the previous controlled entity and deletes the fixture.

These are native handler/widget-dispatch tests using `WidgetManager.ReportMouse`, not physical keyboard/mouse or gamepad simulation. Native input bindings were read from installed `chimeraInputCommon.conf`: Inventory_PrevPage=Q and Inventory_NextPage=E.

For mode isolation, use `$Lobby2IsolationProbe:worlds/Isolation.ent` with `IsolationProbe` / `GC_IsolationProbeComponent`. Compare `OpenInventory()`, `InventorySnapshot()` and `InventoryInputSnapshot()` with McpTestAddon loaded and with only `IsolationProbe/addon.gproj` (Arma Reforger dependency only). `CatalogueOpenOutsideMode()` exists only with McpTestAddon and must return false.

## Row rework evidence (Workbench 1.8.0.13, 2026-10-31)

The compiled production handlers passed 26 assertions: finished indexing, nine visible rows, native next/previous buttons, wheel both ways, typing suspension/resumption, localized-category and case-insensitive multi-term search, click-to-equip, displaced-helmet preservation, incompatible/cancelled drop without inventory mutation, valid head-slot drag, weapon click/drag, consumable insertion, empty results/recovery, native helmet filtering, and disabled-session rejection without mutation. The index contains 2,009 inventory-bearing prefabs and 877 portable candidates, including 92 headwear results. Faction USSR plus descending headwear gives 15 results.

Sources: [interaction receipts](evidence/inventory-catalogue-20261031/interactions.json), [rows and equipped result](evidence/inventory-catalogue-20261031/rows.png), [advanced options](evidence/inventory-catalogue-20261031/advanced.png). Final feature Reload `ea9a690bf23b426e8e4b7332c8f99933` succeeded without new script errors. The fixture's pre-existing missing map diagnostic and unreadable/mismatched source resources remain visible; source coverage is partial. No new script exception occurred during the interaction pass.

The fresh [isolation comparison](evidence/inventory-catalogue-20261031/isolation.json) passed: addon-free PID 33488 and restored Lobby2 test project PID 57664 have identical native layout/visibility snapshots and `selected=1 dragging=1 released=1` from native select/drag/release handlers. Activation outside Lobby2 returned false. The original McpTestAddon and Empty_Lobby world were restored. Independent visual/source review approved the rework.

Coverage limits: English and the observed desktop viewport; physical Q/E/OS drag, gamepad journeys, other UI scales/languages, and every third-party item were not exercised. Native binding/handler verification does not establish physical input delivery. A single resource load or preview can exceed the frame budget.

## Native references

Installed PAK revision `d9b5a4246f9247e4be5ea20935889885`: InventoryMain.layout root Frame and native left-column hierarchy; InventoryContainerGrid.layout lines 264–315 and WLib_SpinBoxPaging.layout/SCR_SpinBoxPaging.c for the reused pager; chimeraInputCommon.conf lines 2465–2503 for Q/E; SCR_InventoryMenuUI Action_DragDown/OnDrag/DragUp and SCR_InvCallBack; generated InventoryStorageManagerComponent for owned-storage checks, insertion and replacement. Custom drag glue is necessary because a catalogue row has no live inventory source slot, and native Action_DragDown restricts slot classes by exact type. Placement still uses the native inventory manager's compatibility and operation callbacks. The native animated character preview is renewed through UpdatePreview every ten seconds while opted in.

## Historical grid slice

## Evidence (Workbench 1.8.0.13, 2026-10-29)

- Reload succeeded without new script errors; existing ArlandLobby resource GUID, SCR_DedicatedServerPlugin and m_aFormations diagnostics remain unrelated.
- Full index: 2,009 inventory-bearing prefabs; final browser filter exposed 877 enabled, portable candidates. Helmet selection: 92; empty backpack: 45. One Next activation advanced offset by exactly 35. Z-A yielded ZSh-5 first. Text no-match yielded zero and recovered after clearing. Faction USSR filtering combined with other filters. Final primary-slot selection yielded 108 candidates; adding AK74 text yielded 12. Equipment + Watch yielded 8; Truck text matched only eight trucker caps, not vehicles.
- Native button selection, actual combo option selection, search, slot filtering, paging, explicit disable, same-owner reopen and closed-menu owner loss exercised. No inventory transfer methods exist in this slice.
- Loaded-addon versus addon-free InventorySnapshot strings were identical: native preset, customPanel=0, vicinity visible, and matching native storage/character geometry. Non-Lobby2 activation returned false.
- Independent source and visual review approved. Final Reload operation: `fc05feda4a0745b98b9db493d221216d`. Original project, loaded test addons and Empty_Lobby world were restored after the addon-free comparison.
- Captures: [headwear](evidence/inventory-catalogue-20261029/headwear.png), [advanced faction/sort](evidence/inventory-catalogue-20261029/advanced.png), [addon-free inventory](evidence/inventory-catalogue-20261029/stock-baseline.png). [Compact MCP receipts](evidence/inventory-catalogue-20261029/runtime-receipts.json).

Coverage limits: one desktop viewport/UI scale and English labels; physical typing/gamepad navigation, other languages/scales, multiplayer rejection in a live peer session, and actual insertion/attachment transfers were not exercised. Individual preview requests/resource loads can still exceed the frame budget. This slice deliberately contains no transfer/admin authority surface.

# Workbench inventory catalogue

This read-only development slice replaces the native Tab inventory's left Vicinity/opened-container area. The native Inventory20Menu, character preview, equipment slots, storage grids and quickbar remain. It does not equip/spawn items, export loadouts or expose an in-game admin tool.

## Use

In local Workbench Play with an active `GC_LobbyGameMode` and a controlled character, call `GC_WorkbenchItemBrowser.Open()` from Game-script development code. This is the production entry point; it does not depend on the test addon. `Disable()` restores the normal left panel. The opt-in survives closing/reopening inventory for the same character, but native controlled-entity change and game-end events release it. It is unavailable in packaged game builds and rejects multiplayer/non-Lobby2 modes.

The category icon strip offers all gear, weapons, headwear, clothing, backpacks, vests, equipment, magazines and attachments. Advanced controls select a loaded-world faction catalogue and alphabetical direction; the header keeps those applied options visible when collapsed. Text search combines with category/slot/faction filters. Clicking native clothing or weapon slots switches to that slot's declared candidates, including empty clothing slots. Clear removes the category/slot filter. Hover/select a preview to read its name; paging shows 35 items at a time.

The browser uses native draggable/visible inventory metadata and enabled components, excluding character and vehicle owners. Watches/binoculars are equipment. This is candidate discovery, not proof that an item fits the current character's capacity or can replace occupied equipment; eventual transfers must use the native compatibility/authority owner. Usable base prefab variants are retained; some generic templates have no preview mesh. Faction means membership in currently observed faction catalogues. Unassigned does not mean a neutral faction.

One prefab picture request is issued per frame, only for the visible page; changing results cancels the obsolete queue. Indexing retains the foundation's four-resource/two-ms batch bound (one resource load itself is indivisible). Reopening builds a fresh index. A partial catalogue is explicitly marked when addon discovery or resources are unreadable. No live gameplay entities are created by the browser.

## Reproduce through MCP

Use the existing `McpTestAddon/addon.gproj`, world `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent`, entity `GameMode_Lobby3`, component `GC_LobbyManager`:

1. `GC_TestNativeInventoryStart()` creates/possesses a stock rifleman test fixture and opens native inventory. `GC_TestCatalogueOpen()` enables the production browser.
2. Poll `GC_TestCatalogueState()` until finished. `GC_TestCatalogueSlot(0)` reports native mouse movement into the helmet slot then executes the native selection action; row 1 exercises the empty backpack slot. `GC_TestCataloguePrimary()` exercises a primary weapon slot.
3. `GC_TestCatalogueSearch(string)` changes the actual edit field. `GC_TestCatalogueButton(name)` calls the attached native button handler (e.g. CategoryWeapons, CategoryHeadwear, Next, Previous, ClearFilter, Advanced). `GC_TestCatalogueOption("Faction"|"Sort", index)` opens the native combo list and clicks its actual element handler. `GC_TestCataloguePage()` exposes names/prefab identities for ordering checks.
4. Close/reopen with `GC_TestCatalogueClose()` and `GC_TestNativeInventoryStart()`. To test ownership loss while closed: close, `GC_TestCatalogueOwnerLoss()`, reopen native inventory; panel must be absent. `GC_TestCatalogueCleanup()` restores the previous controlled entity and deletes the fixture.

These are engine-native event/handler tests, not physical keyboard/mouse or gamepad simulation. Test helpers never inject the final filter result.

For isolation, use `$Lobby2IsolationProbe:worlds/Isolation.ent` with `IsolationProbe` / `GC_IsolationProbeComponent`. Run `OpenInventory()` and `InventorySnapshot()` with McpTestAddon loaded, then repeat with only `IsolationProbe/addon.gproj` (depends solely on Arma Reforger). `CatalogueOpenOutsideMode()` exists only in McpTestAddon and must return false.

## Evidence (Workbench 1.8.0.13, 2026-10-29)

- Reload succeeded without new script errors; existing ArlandLobby resource GUID, SCR_DedicatedServerPlugin and m_aFormations diagnostics remain unrelated.
- Full index: 2,009 inventory-bearing prefabs; final browser filter exposed 877 enabled, portable candidates. Helmet selection: 92; empty backpack: 45. One Next activation advanced offset by exactly 35. Z-A yielded ZSh-5 first. Text no-match yielded zero and recovered after clearing. Faction USSR filtering combined with other filters. Final primary-slot selection yielded 108 candidates; adding AK74 text yielded 12. Equipment + Watch yielded 8; Truck text matched only eight trucker caps, not vehicles.
- Native button selection, actual combo option selection, search, slot filtering, paging, explicit disable, same-owner reopen and closed-menu owner loss exercised. No inventory transfer methods exist in this slice.
- Loaded-addon versus addon-free InventorySnapshot strings were identical: native preset, customPanel=0, vicinity visible, and matching native storage/character geometry. Non-Lobby2 activation returned false.
- Independent source and visual review approved. Final Reload operation: `fc05feda4a0745b98b9db493d221216d`. Original project, loaded test addons and Empty_Lobby world were restored after the addon-free comparison.
- Captures: [headwear](evidence/inventory-catalogue-20261029/headwear.png), [advanced faction/sort](evidence/inventory-catalogue-20261029/advanced.png), [addon-free inventory](evidence/inventory-catalogue-20261029/stock-baseline.png). [Compact MCP receipts](evidence/inventory-catalogue-20261029/runtime-receipts.json).

Coverage limits: one desktop viewport/UI scale and English labels; physical typing/gamepad navigation, other languages/scales, multiplayer rejection in a live peer session, and actual insertion/attachment transfers were not exercised. Individual preview requests/resource loads can still exceed the frame budget. This slice deliberately contains no transfer/admin authority surface.

## Native references

Installed PAK revision `3e84dd3e4386408097aaf18a7a8b6372`: `InventoryMain.layout` left-area hierarchy; `SCR_InventoryMenuUI` native update/selection/close handlers; `SCR_InventoryStoragesListUI.FillWithEmptySlots` and `Configs/Inventory/InventoryUI.conf` for equipment rows/icons; `SCR_InventorySlotWeaponSlotsUI` for weapon slot types; `SCR_ItemAttributeCollection` for draggable/visible metadata; native `SCR_ButtonBaseComponent`, combo-box and edit-box resources for input/state styling. The native animated character preview expires while idle in Workbench, so the opt-in session renews it through the existing UpdatePreview owner every ten seconds.

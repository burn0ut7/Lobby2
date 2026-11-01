# Workbench inventory catalogue

## Explosives category and pager keycaps (2026-11-01)

The grenade icon after Ammunition filters Explosives & throwables. Classification
uses resolved WeaponComponent WeaponSlotType values `grenade` and `throwable`,
including native grenades, smoke, demolition blocks and mines. These no longer
appear under Weapons; All Items remains unchanged. Source examples at installed
revision `d9b5a4246f9247e4be5ea20935889885`: Grenade_Base.et line 106 and
SmokeGrenade_Base.et line 47 in Prefabs/Weapons/Core.

Native WLib_PagingButton.layout clips its fixed 30x30 bounds. Catalogue-local
overrides reserve 48x32, remove forced aspect ratio/clipping, and give the native
pager 40px height. The item counter reserves room for the wider E control.
Reload `3958a2157fe14bebba7f5c940ee59df3` passed without new script errors.
Live category callbacks showed 24 explosives and 144 weapons (formerly 168),
forward paging moved to offset 10 and back returned to zero. Capture
`1793511760943386700-capture_screenshot.png` in the toolrepo's
`.temp/local/evidence/loadout-foundation` shows localized labels and complete Q/E
keycaps at 20 / 24. Transfer and slot-compatibility authority remain unchanged.

## Single-mod hover and category order (2026-11-01)

Ammunition follows Weapons; Headwear, Clothing, Vests and Backpacks are consecutive.
Hover now shows only the resolved resource provider, following installed
`SCR_AddonTool.GetResourceLastAddon` semantics (last source addon, PAK revision
`d9b5a4246f9247e4be5ea20935889885`, SCR_AddonTools.c lines 59–68). It does not list
inherited contributors. Native items resolve to Arma Reforger; the Lobby2 radio
resolves to Lobby2 (`338af8fe6d7a46708928cb1c96917f12`). The dedicated tooltip is
black with white text; shared admin tooltip styling is unchanged.

Actual hover capture, after user mouse input:
toolrepo `.temp/local/evidence/loadout-foundation/1793512987649435600-capture_screenshot.png`.
Native mouse exit dismissed it (`eb58bdb538d14e95ae6eb647b38a04c3`). Automated
mouse reporting reached the row but did not initially dispatch Show in the background;
physical hover was confirmed by the user and captured through MCP. Independent
source/visual review approved. Coverage remains the observed desktop viewport.

## Search options and source hover (2026-11-01, superseded hover wording)

The compact row above Search offers A–Z/Z–A and All mods/loaded source addons.
Only addons contributing portable items are listed, using GameProject titles.
The mod filter combines with text, category and weapon compatibility filters;
changing an option resets to page one. Ten rows fit below the selectors. Source
membership includes inherited contributions, so an item can match multiple mods.
Hovering a row uses the native tooltip lifecycle and displays only source mod
titles. Changing results or closing the catalogue hides its tooltip.

Eden's documented precedent is a mod dropdown beside asset search:
https://dev.arma3.com/post/spotrep-00059. Native source revision remains
`d9b5a4246f9247e4be5ea20935889885`; SCR_ComboBoxComponent and GameProject supply
controls and titles. The tooltip reuses the existing plain-text native tooltip
preset rather than adding an inventory footer.

Reload `f6a77df5f6624dbba2f0949ee285ec03` passed with no new script errors.
Production option callbacks verified both orders (DH-132A versus ZSh-5 for helmet
search), 813 all-mod candidates versus one Lobby2 radio, combined category/text
filters, no matches, clear-search recovery and page-one reset. Native paging and
wheel advanced by ten; wheel left offset unchanged while a dropdown was open.
Native tooltip Show/Hide with explicit hover context returned Arma Reforger and
Lobby2 for the radio, then disappeared when All mods changed the result page.
Receipts and captures are in toolrepo `.temp/local/evidence/loadout-foundation`:
`1793510784103683200-capture_screenshot.png` (mod choices),
`1793511016865539600-capture_screenshot.png` (tooltip), and
`1793511039501999700-capture_screenshot.png` (page change hides tooltip).
Native hover dispatch was then exercised through WidgetManager.ReportMouse:
entry `8045b0eafabb48898e7b9d4302f1c0fe` displayed the layout-attached tooltip
(`1793511101432572600-capture_screenshot.png`); exit
`ca68535a864e4bf7b0aed0d794b794d3` dismissed it
(`1793511121770865900-capture_screenshot.png`). The retained optional fixture uses
native mouse reporting, not Show or injected hover context.

The bottom-left native navigation bar now includes a clickable S / Save / Load
button. Its action belongs to InventoryMenuContext (priority 120, activated by
SCR_InventoryMenuUI), rather than lower-priority InventoryContext. The native
input button owns key and click dispatch; there is no second manual listener.
Typing focus suppresses the key action, while an explicit click clears typing
focus and opens the menu. The control exists only during Workbench authoring.

Final Reload `8edb7adece1b4a83b88baa8c13b12fd4` passed. Key callback with search
focused left files=0 (`32497880f3c440dba26b20050bed3fc8`); clicking from search
opened files=1 (`a51ffbb418554c2d8e2f9d90342ddec3`). Native key callback outside
search opened files=1 (`0bfa41f9809f41c6853b81fd31084f46`). Disable removed the
button, and reactivation restored exactly one; footer captures
`1793511296569113400-capture_screenshot.png` and
`1793511410409327200-capture_screenshot.png`. Final gameplay SCRIPT errors only
contained the existing missing-map fixture diagnostic.

Physical OS mouse/keyboard input, gamepad navigation, many-mod scrolling, other
UI scales and a new addon-free baseline were not exercised. Existing catalogue
coverage and eligibility limits remain.

## Weapon-slot browsing (2026-10-31)

Click an empty weapon slot to browse that slot's weapon type. Click an equipped weapon to browse its magazines and attachments, including ammunition for attached grenade launchers. Left-click or right-click a result to attach/load it into that selected weapon, replacing a compatible occupied slot when necessary. Transfers include nested attachment storage (for example M203 ammunition) and never fall back to another weapon or clothing. Search narrows this list; a category button or another equipment slot leaves the weapon context. The selected slot is observed every 250 ms, so replacing/removing its weapon updates the filter and resets paging. Dragging still uses its explicit destination; the click following a drag is suppressed.

Matching uses the live weapon's muzzle magazine wells and inspection-visible attachment types, with the same inheritance direction as native `SCR_MagazinePredicate` and `SCR_CompatibleAttachmentPredicate`. This is compatible-type browsing, not a guarantee that an occupied/obstructed slot can accept a transfer immediately; native transfer checks still decide that. No candidate entities are spawned for filtering, and previews remain bounded to one visible item per frame. Empty slots retain the existing declared weapon-slot-type filter. Existing catalogue coverage/eligibility limits remain.

Native references at revision `d9b5a4246f9247e4be5ea20935889885`: `game/data/data007.pak/scripts/Game/Inventory/SCR_InventoryStorageManagerComponent.c` lines 156–237; generated `Weapon/BaseWeaponComponent.c` lines 17/35, `BaseMuzzleComponent.c` line 41, and `AttachmentSlotComponent.c` lines 16/26; `Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_Base.et` lines 26–28 establish the resource field.

Reload `bbdb6f2f535441eabaa4890ae96a532e` passed. [Live results](evidence/catalogue-weapons-2026-10-31/results.json) cover M16 (22 matches), AK-74N/GP-25 (19, including launcher rounds), RPG-7 (5), M9 (2), empty primary/secondary slots, paging, search/no matches, category/helmet reset, insertion into a selected empty slot and deletion returning to weapon browsing. [M16 screenshot](evidence/catalogue-weapons-2026-10-31/m16.png). Tests use actual WidgetManager targeting and production handlers, not physical OS input. Final gameplay SCRIPT diagnostics contained only the existing missing-map fixture error.

## Right-click and target drops (2026-10-31)

The subsequent attach/load extension passed Reload `78b74d58681d42dcb42138ee3788ed60`; final test-helper Reload `3746b12b62f8430f94c74527c83ad5b8` also passed. [Attachment receipts](evidence/catalogue-weapons-2026-10-31/attach.json) prove left-click optic installation, right-click optic/magazine installation, nested M203 loading and no change to the other equipped rifle. [Gesture receipts](evidence/catalogue-weapons-2026-10-31/gestures.json) prove left-click magazine loading, shirt-targeted drag, suppression of trailing clicks after successful/cancelled drags, and no left-click insertion outside weapon context. [Rendered result](evidence/catalogue-weapons-2026-10-31/attached.png). [Final unrelated-mode check](evidence/catalogue-weapons-2026-10-31/attach-isolation.json) matches the same-session [addon-free baseline](evidence/catalogue-actions-2026-10-31/isolation.json): native geometry/select/drag/drop preserved and browser activation rejected. Original project/world restored afterward.

Historical interaction pass below: the later weapon-context exception above allows left-click attachment/loading. Outside that context, left-click still does not create an item.

Verified on Workbench 1.8.0.13 after Reload `fc358247a7c84afa8c6319fb4ac3c002`. [Receipts](evidence/catalogue-actions-2026-10-31/results.json), [mode comparison](evidence/catalogue-actions-2026-10-31/isolation.json), and [rendered browser](evidence/catalogue-actions-2026-10-31/browser.png).

Single catalogue right-click equips; left-click does not create an item. Dragging a beret equips the head slot. Dragging morphine onto the shirt icon, header, or grid inserts into that exact shirt. Cancelled and incompatible drops preserve inventory. Right-click removes equipped and stored items; deleting the shirt also removes its remaining contents. Unowned and browser-disabled deletion attempts are rejected without deleting the item. Disabling removes the input listener; reopening restores it. Native completion clears stale slot focus before rebuilding storage widgets.

Unrelated-mode selection, dragging, geometry and Inventory_Drop behavior match the addon-free probe exactly: the native shortcut drops the helmet, leaving its entity alive (`exists=1 contained=0`). Final gameplay logs contained no new script exception; existing BlockListApi leak and missing map fixture diagnostics remain.

Native evidence: installed revision `d9b5a4246f9247e4be5ea20935889885`, `Configs/System/chimeraInputCommon.conf` lines 4099–4121 (`Inventory_Drop`, RMB/X/gamepad X); `SCR_InventoryMenuUI.c` lines 1270–1303 (owning OnAction), 111–260 (completion), and 1719 (focused storage refresh); generated `InventoryStorageManagerComponent.c` lines 38–49 (TryDeleteItem, GetStorages, Contains). Pointer targeting walks only the actual hit widget's ancestors to their native slot/storage handlers: native focused-slot state can remain on the source during a catalogue drag, so it cannot identify the destination reliably.

Coverage uses WidgetManager mouse reporting and production action handlers through MCP, not physical OS clicks or gamepad input. Quickbar shortcuts are not deletion targets. Container-header drops are covered; deletion targets native item cells/equipment icons. No layout redesign in this slice.

The local development browser replaces the native Tab inventory's left Vicinity area with a larger item catalogue. The native Inventory20Menu, character preview, equipment slots, storage grids and quickbar remain. This is a Workbench-only tool, not the future multiplayer admin arsenal or saved-loadout editor.

## Use

In local Workbench Play with active `GC_LobbyGameMode` and a controlled character, call `GC_WorkbenchItemBrowser.Open()`. Production has no dependency on the test addon. `Disable()` restores the normal left panel. The opt-in survives closing/reopening for the same character; controlled-entity changes and game end release it. Packaged game builds omit the hooks. Multiplayer and non-Lobby2 modes reject activation and transfers.

The expanded panel has eleven rows, each with an item preview and its category above its name. The category strip filters all portable gear, weapons, headwear, clothing, backpacks, vests, equipment, magazines and attachments. Search combines lowercase whitespace-separated terms across names, localized category, resource identity, equipment slot, contributing addon and observed faction membership. All portable candidates are shown by default. Missing display-name translations fall back to the prefab filename. Search is at the top edge; the redundant category heading and advanced dropdown are removed. Results use alphabetical order. Faction text remains searchable and means membership in a loaded-world item catalogue, not inferred texture allegiance.

The native arsenal `SCR_SpinBoxPagingComponent` provides Q/E action hints and paging buttons in the bottom corners. Page/total-page and visible-range/total-item counts sit beside E at the bottom right. Mouse wheel over the panel changes the same page. Paging pauses while the search field has focus, so Q/E can be typed. Clicking native helmet/backpack/clothing or weapon slots filters matching items.

Outside a selected weapon's attachment list, right-click on a catalogue row equips into a compatible equipment slot, preferring an empty slot; otherwise it replaces compatible gear or inserts into a deposit container. Left-click only focuses/selects these ordinary rows. In a selected weapon's attachment list, both clicks attach/load into that weapon. Native replacement preserves displaced gear on the ground. Dragging uses the native inventory drag action, threshold and cursor preview, then inserts a new instance into the targeted compatible equipment slot or container. A shirt dragged onto the shirt equipment icon is equipped; a consumable dropped onto that icon, the shirt header or its grid goes inside that shirt. Dragging onto the character uses automatic placement. Invalid targets and cancelled drags create no gameplay item. Quickbar assignment remains native inventory behavior after insertion; catalogue drops and browser deletion shortcuts on quickbar shortcuts are rejected.

Right-click on an owned native inventory item deletes it through the inventory manager instead of moving it to the ground. Deleting clothing/containers also deletes their contents. Ownership, browser mode and inventory-lock checks apply at execution; unowned items and disabled sessions cannot delete. The scoped listener uses the game's `Inventory_Drop` action (default RMB, X, gamepad X), consumes the old menu action while active, and is removed on close or mode exit. Native behavior remains inherited outside the opted-in browser. Placement/deletion feedback briefly occupies the footer. Native refresh clears obsolete focused-slot pointers before rebuilding equipment widgets.

Only enabled, visible, draggable inventory-bearing prefabs are candidates; vehicles and actors are excluded. The resolved inventory UIInfo must have an authored name, and MeshObject must supply an Object resource other than the engine's `Common/Models/Default.xob` placeholder. Inherited names/models count; a missing translation is not a missing authored name. Filenames, parent names and the word Base have no role in eligibility. These component-data checks reject empty/placeholder templates across item families, not every resource used as an inheritance parent. A fully configured base can have exactly the same resolved components as its concrete child; distinguishing authoring intent then requires explicit catalogue membership or additional metadata. Items built entirely from child/runtime models are outside the current criterion.

Metadata indexing processes up to 200 discovered resources per frame, with one addon discovery per frame and no former two-ms cutoff. `Metrics()` reports batch size, elapsed time and maximum indexing tick. Preview requests remain limited to visible rows, one per frame; a changed page cancels obsolete work. Resource loads are indivisible and a batch can hitch, especially on a cold cache. Partial source coverage is displayed honestly. Gameplay entities are created only for a validated placement; native inventory callbacks update the screen and remove a failed uninserted instance.

## Reproduce through MCP

Use `McpTestAddon/addon.gproj`, `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent`, `GameMode_Lobby3` / `GC_LobbyManager`:

1. `GC_TestNativeInventoryStart()` creates/possesses a stock rifleman and opens native inventory; `GC_TestCatalogueOpen()` enables the production browser. Poll `GC_TestCatalogueState()` until finished.
2. `GC_TestCatalogueSearch(string)` changes the edit field. `GC_TestCatalogueButton(name)` clicks attached handlers (CategoryWeapons, CategoryHeadwear, CategoryAll). `GC_TestCataloguePage()` reports result identities.
3. `GC_TestCataloguePager(bool next)` clicks the native page button; `GC_TestCatalogueWheel(int direction)` exercises the wheel handler. `GC_TestCatalogueSearchFocus(bool)` tests typing isolation. `GC_TestCatalogueSlot(0)` and `GC_TestCataloguePrimary()` report real widget hover then invoke native selection.
4. `GC_TestCatalogueClick(row)` exercises left-click without placement. `GC_TestCatalogueRightClick(row, equipmentRow)` invokes the actual shortcut handler and native action dispatch to check suppression: row >= 0 selects the catalogue row, row -1 targets equipmentRow, and equipmentRow 200 targets a stored shirt item. `GC_TestCatalogueDeletionState()` reports actual entity/content survival and lock/listener state. `GC_TestCatalogueDrag(row, equipmentRow)` reports movement and calls production drag handlers with native focus cleared; -1 cancels, 0 targets the head, 4 the shirt, 100 weapon slot 0. `GC_TestCatalogueContainerDrop(row, header)` targets the shirt header or grid. `GC_TestCatalogueOwnedItems()` reports exact parent containers. `GC_TestCatalogueRejectDelete(disabled)` checks unowned/disabled deletion. `GC_TestCatalogueInventory()` reports actual attached resources. `GC_TestCatalogueOldHead(bool capture)` proves replacement preserves the old item. `GC_TestCatalogueRejectDisabled()` disables ownership then invokes the production transfer service.
5. `GC_TestCatalogueClose()`, `GC_TestNativeInventoryStart()` and `GC_TestCatalogueOwnerLoss()` exercise close/reopen and ownership loss. `GC_TestCatalogueCleanup()` restores the previous controlled entity and deletes the fixture.

These are native handler/widget-dispatch tests using `WidgetManager.ReportMouse`, not physical keyboard/mouse or gamepad simulation. Native input bindings were read from installed `chimeraInputCommon.conf`: Inventory_PrevPage=Q and Inventory_NextPage=E.

For mode isolation, use `$Lobby2IsolationProbe:worlds/Isolation.ent` with `IsolationProbe` / `GC_IsolationProbeComponent`. Compare `OpenInventory()`, `InventorySnapshot()`, `InventoryInputSnapshot()` and `InventoryShortcut(true)` followed by `InventoryShortcut(false)` with McpTestAddon loaded and with only `IsolationProbe/addon.gproj` (Arma Reforger dependency only). `CatalogueOpenOutsideMode()` exists only with McpTestAddon and must return false.

## Faster indexing and component eligibility (Workbench 1.8.0.13, 2026-10-31)

[Live receipts](evidence/inventory-catalogue-indexing-20261031/checks.json) compare the same 19,485-resource scan: the former four-resource batch took 4,882 test ticks (maximum tick 69ms); the final 200-resource batch took 104 ticks, 1,033ms elapsed and maximum tick 78ms with the UI already indexed. Both used the existing warm Workbench process/cache; this is not a cold-start benchmark. A separate run with simultaneous UI indexing took 1,891ms. The test callback's tick count is indexing calls, not a measured render-frame count. Preview scheduling remains one visible item per frame.

Live UI verification: 813 portable results, down from 843 after rejecting missing authored names/default models. Generic `Binoculars_base.et` and `Weapons/Core/Magazine_Base.et` return zero; the configured `Magazine_545x39_AK_30rnd_Base.et` still returns one. Headwear retains 90 results. Binoculars show six configured B12/B8/M22 resources, including both parents and concrete variants. These retained parent variants are intentional: their resolved components qualify as usable gear, and no filename exclusion is applied. Reload `248c33d7eeb74607bb0bbe0911c34dd7` succeeded; no new script exception occurred in these checks. Existing BlockListApi reload-leak and missing-map fixture diagnostics remain, and source coverage is still partial.

Inspected the installed `BaconLoadoutEditor_606B100247F5C709/data.pak`, revision `d9b5a4246f9247e4be5ea20935889885`. `scripts/Game/BaconLoadoutEditor/GunBuilderUI_Cache.c` line 39 gets `SCR_ArsenalComponent.GetFilteredArsenalItems`; lines 196–218 check clothing area types and lines 242–268 check attachment mode, InventoryItemComponent and CanSetAttachment. Thus Bacon's item universe is an authored arsenal catalogue, not a general abstract-prefab detector. We retain broader resource discovery, with native slot compatibility at placement. No Bacon code/assets/dependency were added.

Component evidence at the same installed revision: `Prefabs/Items/Equipment/Binoculars/Binoculars_base.et` lines 4–27 contains inventory attributes and the Default.xob placeholder; lines 48–50 still define a clothing area. `Binoculars_M22/Binoculars_M22_base.et` lines 6–9 and 39–40 supply a real display name/model, and `Binoculars_M22.et` lines 1–3 inherits without changing components. `Prefabs/Items/Core/Item_Base.et` lines 6–15 leaves the inventory UIInfo name empty. This supports the resource-data checks and their limits.

## Compact panel refinement (Workbench 1.8.0.13, 2026-10-31)

Historical model-only filter evidence below predates the authored-name/placeholder checks and 200-resource indexing batch.

Search is flush with the top padding; Q/E and counts share the bottom edge. Eleven 80px rows replace nine 84px rows. The duplicated hover-name footer is removed. Placement feedback temporarily occupies the count line for two seconds, then counts return. Native character/storage geometry remains identical to the prior verified baseline. No shared inventory overrides or placement handlers changed.

The resolved-model filter removes 34 empty candidates (877 to 843); headwear changes from 92 to 90. Live search excludes `Prefabs/Weapons/Core/Magazine_Base.et` but retains `Magazine_545x39_AK_30rnd_Base.et` and its inherited `_Ball.et` variant. Twenty handler assertions cover indexing, eleven rows, native next/previous and wheel paging, search-focus protection, removed header, these model distinctions, category/multi-term/empty search, and click/drag with actual inventory readback. See [receipts](evidence/inventory-catalogue-compact-20261031/interactions.json) and [final layout](evidence/inventory-catalogue-compact-20261031/rows.png). Final geometry and paging receipts in the same interaction file cover the bottom pager and eleven-row list. Final Reload `76c0d64a308a464f8c4b42254d585047` compiled; existing resource diagnostics and a `BlockListApi` leak diagnostic during Reload remain. Physical input, alternate UI scales and every third-party prefab remain unverified.

Installed source evidence at the revision below: `Prefabs/Weapons/Core/Magazine_Base.et` lines 10–35 (generic inventory metadata, no mesh Object), `Prefabs/Weapons/Magazines/Magazine_545x39_AK_30rnd_Base.et` lines 29–44 (configured usable magazine/model), and its `_Ball.et` lines 1–3 (inherited variant). `SCR_BaseContainerTools.c` lines 140–149 provides the engine-resolved component lookup. A malformed pager override was caught during the first live load, corrected, and the same project/world restored; final evidence uses the corrected layout.

The following row-rework evidence predates the compact header/footer and model filter.

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

## Clothing categories (2026-11-01)

Wearables are classified from BaseLoadoutClothComponent.AreaType, including derived
slot types. Clothing contains Jacket, Pants, HandwearSlot and Boots areas only.
HeadCover belongs to Headwear; Vest and ArmoredVestSlot to Vests; Backpack to
Backpacks. Other worn equipment stays under Equipment and All. Category buttons
query these categories; selecting a character slot still uses its specific slot.

Workbench 1.8.0.13, installed-source revision d9b5a4246f9247e4be5ea20935889885:
Reload 7904b22f6e334cfab3ef41c747b4602c introduced no new errors. Live native-button
checks found 106 Clothing results (shirts/pants shown; six boots), zero Clothing
results for helmet or vest, 43 Vests including 6B2/6B3, ALICE and PASGT, 90 Headwear
and 40 Backpacks. No portable gloves were specifically verified in the installed
catalogue; their native HandwearSlot type is included by the classifier.

Receipts and unmodified screenshot are in the tool repository's
.temp/local/evidence/loadout-foundation: 202bd93d83d744c48f9231a519c4f0e9,
2b4bdf66a90a404286788a8cb3de0af9, bf968afa51d84df2b7dd55653f08f3a8,
598d1cabc90d4f06821ff19ee86b4068, fd05a6dc60ea415aa7bc445d438f214c,
fdde650866d14ea3814696500fdc7f52; 1793518176441626200-capture_screenshot.png.

## Carryable equipment filtering (2026-11-01)

Inventory display/drag attributes alone also describe world storage interfaces.
Portable results now require ItemPhysicalAttributes and reject native
SLOT_LOOT_STORAGE markers. Clothing classification ignores disabled
BaseLoadoutClothComponent instances. No filename/category-name blacklist is used.

Installed-source revision d9b5a4246f9247e4be5ea20935889885: AmmoBox_Base.et has
storage display attributes without item physics; Sign_SupplyStorage_Base.et uses
SLOT_LOOT_STORAGE; Part_Sandbag_Base.et disables its inherited cloth component.
The 81/82mm mortar boxes deliberately define item physics, a gadget and pickup
action, so these remain available.

Workbench 1.8.0.13 Reload 9bb4184c30ba4f73a67a39b0cb081206 introduced no new
errors. Live Equipment sign search returned zero (f37001c6be1341d0953e32df5789951b).
Ammo-box search retained five carryable mortar-box resources (d99a7fb45a4e46089fa47545b7cd07da).
Three sandbags appeared in Equipment (faa5b98ce4094d1488107a69e4c27315), zero in
Backpacks (c145b36bf64b484ebd99b8177051bd33). Native drag put a burlap sandbag
inside the BDU blouse (0cf7c86699d34e3785a78d61f39c5368) and a mortar box inside
an ALICE medium backpack (ecfc89adce7e42e48ae61ef9d2fdaf7b); owned-item readback
confirmed both destinations. Automatic right-click placement of the sandbag was
rejected in this test; that transfer behavior was not changed by this filtering fix.

Evidence is in toolrepo .temp/local/evidence/loadout-foundation, including screenshot
1793518509489472400-capture_screenshot.png. Independent source/UI review approved.

## Stable previews and alphabetical names (2026-11-01)

Trim localized display names before indexing; the SVD Magazine Pouch had leading
whitespace. Alphabetical sort groups ASCII punctuation-prefixed and empty names
after ordinary names in both directions. Retain each visible prefab's row widget
and preview binding across result changes, including row movement. Only new
bindings enter the one-preview-per-frame queue.

Workbench Reload 25cea6c4fc264d0e81e6f6836e575192 compiled without new errors.
Optional GC_CatalogueRefreshTests exercises production Refresh and GetSortKey.
Live retention checks passed unchanged=10/10 and shifted=9/9 in both directions
(7f2dea9cc9514322a3236641470b815d, 4a3242b1287a472f96f63161cad49168).
Sort fixtures put the trimmed SVD name between Alpha and Zulu and kept punctuation
last in A-Z and Z-A (381d36fa1f7d458d8439121e8f3af32e,
c65809286d274c0f8dcf5c7a09e47340). Actual SVD search returned its trimmed name
(38a22b20815644a4a04f742e54023e70); the full A-Z list starts with 105mm ammunition.
Screenshot 1793518926007556000-capture_screenshot.png confirms descending row
order and matching previews. State checks establish retained preview visibility;
the screenshot is not a temporal flashing measurement. Independent review approved.

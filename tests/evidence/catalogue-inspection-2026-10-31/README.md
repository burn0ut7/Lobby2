# Native inventory and weapon inspection

Verified in embedded Workbench 1.8.0.13, PID 44368, Lobby2 MCP Tests project, Empty_Lobby.ent. Final Reload: a2951f37082e40238a4f97c15265866d. No new SCRIPT errors; the fixture still reports its missing map entity. Existing resource/plugin diagnostics are unchanged.

## Final behavior

- Middle mouse is an additive action in the existing InventoryContext. The abandoned priority-101 context blocked native inventory input; it has been removed. Live readback: nativeContext=1, blockingContext=0, mouse:button2 binding present.
- Owned items use native drag start/update/release, preview, highlighting, container movement, weapon movement and quick-slot handling. A captured source slot prevents hover changes from replacing the pressed source. The Workbench-only animation-readiness exception enables the posed character's native transfers; native lock/deleted-item checks remain.
- Only empty-background drops and exact weapon-inspection attachment destinations extend native drop handling. Character preview and hit-zone destinations remain native. Background drops retain the actual item on the ground. X/RMB retains the editor's owned-item delete behavior; in weapon inspection, attachments use available weapon slots first and deposit storage second. Weapon RMB still removes the weapon.
- Native weapon inspection opens and retains its attachment gizmos; selecting a gizmo filters the catalogue, left/RMB inserts there, and existing attachments drag into and out of it. Changing the inspection point resets pagination.

## Evidence and limits

`native-final.json` contains the final native-path tests and unrelated-mode checks: rifle 0→1→0; bandage trousers→shirt; magazine shirt/trousers; optic catalogue placement, drag out/in, and RMB from shirt back to weapon; native context activation; ground drop (`alive=1 owned=0`) with empty-slot readback. The deliberately invalid paging call was compiler-rejected before invocation. It is not a passing paging test.

`native-drag-final.png` captures the native dragged magazine and highlighting. `inspection.png` records the native weapon preview composition. `runtime.json` preserves earlier diagnostic tests before native drag restoration and must not be treated as final input validation.

Tests report real widget cursor positions and call production handlers. They do **not** simulate physical mouse buttons or X through the OS. The earlier handler-only tests missed the blocking input context; final context readback and installed input-system documentation establish its removal, but physical input delivery remains manual coverage.

Unrelated-mode layout, selection/drag/release and shortcut outcomes match the addon-free baseline in ../catalogue-actions-2026-10-31/isolation.json: customPanel=0, selected=1 dragging=1 released=1, removed item exists=1 contained=0. Original world and open catalogue restored afterward.

## Installed-source provenance

PAK revision d9b5a4246f9247e4be5ea20935889885:

- game/core/data.pak/scripts/GameLib/inputManagerDoc.c:155–178: priority and overlay behavior; higher non-overlay context excludes lower contexts.
- game/data/data007.pak/Configs/System/chimeraInputCommon.conf:8047–8108: InventoryContext priority 50, native drag/drop/inspection refs; 4099–4120: Inventory_Drop binds RMB and X.
- game/data/data007.pak/scripts/Game/UI/Inventory/SCR_InventoryMenuUI.c:3238–3313: native destination handling; 3363–3538: container and weapon transfers; 4850–4964: native drag lifecycle.
- game/data/data007.pak/scripts/Game/Inventory/SCR_InventoryStorageManagerComponent.c:874–879,1936–1940: movement gates and animation readiness.

Independent review identified and resolved inspection pagination carryover and accidental interception of the character-preview equip destination.

# Gear UI fixes — 2026-11-11

## Missing tooltip and dialog resources

Restore `GC_CatalogueTooltips.conf`, `GC_GearQuantityDialog.conf`,
`GC_LoadoutDialogs.conf` and their resource metadata from `42425c7^`.
The selector still referenced these deleted resources. The native tooltip loader
dereferenced its missing configuration, raising an exception when hovering an item.
The quantity and loadout confirmation dialogs had the same missing-resource cause.

Workbench session `logs_2026-11-11_00-35-20`, lines 27313–27332, records the
missing tooltip resource and `GC_CatalogueTooltip.Show` null `rsc` exceptions.
Reload `aeeff339106441dcad9f14f92716f881` completed without compilation errors.
The user then verified a real item hover: the tooltip appeared without errors.
Quantity and overwrite/delete confirmation journeys were not exercised.

## Inventory session lifetime

The admin gear session already closes with inventory. However, loading a prefab
can activate the separate Workbench item browser, which previously stayed active
after closing inventory. A subsequent ordinary inventory opening rebuilt the
catalogue. Native close/open reproduced this before the change (runtime operation
`e7289a80bef24ce5931dae9ab3cff120`).

Inventory close now releases the Workbench browser only when its owner matches
that inventory's player. This leaves the new owner's session intact when the old
character's inventory closes during prefab replacement. No admin permission,
inventory content, or prefab file is changed by the cleanup.

Reload `d3641a263c0e4847bc26d07d0e054cc4` completed without compilation errors;
Play `c86fafb484b74cb5b57dca66974ea14f` started normally. The user confirmed the
close/reopen behavior was fixed. The automated final reopen observation was
inconclusive because the active character/menu changed during the check.
Existing Arland world-reference diagnostics remain unrelated.

Fresh addon-free mode-isolation, multiplayer, and prefab-replacement handoff
coverage were not run in this pass. The additional close hook is Workbench-only;
it delegates to the existing browser teardown and preserves inherited close.

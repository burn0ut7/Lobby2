# Loadout inheritance audit — 2026-11-01

Review only: no production repair was made. Workbench 1.8.0.13, installed-source
revision d9b5a4246f9247e4be5ea20935889885. Disposable fixtures were created through
production GC_LoadoutPrefab.Create/Save. Add Variant and overwrite confirmation
were exercised through the existing menu handlers. User loadouts were not saved.

## Findings

### P1 — inheriting an immediately saved base duplicates inventory

GC_LoadoutPrefab.Save spawns m_Resource as its comparison reference (line 658).
WriteContents then reads that runtime inventory and the destination's resolved
InitialInventoryItems (lines 455–536). These can lag the source just written in
the same Play session. A newly saved base had four jacket bandages plus two
inherited pants bandages. Immediately creating/saving its unchanged child wrote
another four-bandage jacket configuration with a different identity.

After development Reload, the base spawned with six bandages and the child with
ten. This is persisted duplication, not only a misleading preview. Operations:
69df8ed9dc0f4a37a224970e73a64a63 (base save), c1c3b9c3ab6646cfa3c75641e069c8f9
(child save), 05940288f1074b7db172b4b12eca8a9c (base six),
f4b6022549154be394e070ac0ba655f1 (child ten).

### P1 — a repeated save can report success without saving restored contents

With the refreshed base, an unchanged freshChild serialized only its parent and
entity ID; it spawned with six bandages. Removing one and saving wrote three
jacket bandages, correctly producing five overall. Loading the base again restored
six in the editing character. Saving freshChild then returned success but left
the three-bandage override unchanged. This is consistent with a stale six-bandage
comparison reference taking the SameItems(wanted, original) early return (line 488);
that branch was not directly instrumented during this save.

After Reload, freshChild spawned with five. Operations: cdcd6c1e96b246c398a5d143a9b2bb28
(clean child), b371432684144d93808ee92a0437cc2f (six),
06d85f03e00f4c9abc7b243232f209fc (five save), b24be0a5792e4211a9f0146406cea60c
(unsuccessful restoration reported saved), a42ce76da72f4e3984627469b08286a1 (five
after Reload).

### P2 — random-variant readback is stale until resource refresh

Add Variant from unchanged current gear wrote US_variant_base_2.et with a parent,
name and empty local randomizer; it did not duplicate inventory. Same-session
GC_LoadoutVariants.Load reported two inherited entries, including the new variant
itself, despite that empty local randomizer. After Reload it correctly returned
zero. Operations: ef2b04d8a23d4b1d94a41d487cd8898e (menu save),
352a120123174527bd1ba82f24825210 (stale two),
d877e66945354acd9e05677603fcdf7c (fresh zero).

This establishes stale metadata readback, not a persisted cycle or recursion.
Native SCR_EditableEntityComponentClass.GetRandomVariant (installed lines 23–73)
selects one level; no recursive execution was claimed or tested.

## Passing cases and corrected hypothesis

- A child created from the refreshed base stored no bandage list and spawned six.
- Duplicate retained the original rifleman ancestor and copied the base's local
  inventory configuration, as intended for native duplication.
- After refreshing the five-bandage child, saving the base's six-bandage contents
  serialized `PrefabsToSpawn +{ }`. The native writer reduced equal contents to an
  empty additive change. Increasing the base to seven then flowed through to the
  child after Reload (9a27dfe247f7440ea05f2acd8a0fde45).
- Therefore absence of an explicit ClearVariableValue call in WriteContents does
  not establish broken inheritance restoration. The earlier source-only diagnosis
  was too broad. The confirmed failure is same-session stale comparison state.
- Existing weapon/slot and variant-list tests remain in LoadoutPrefabs.md; this
  audit did not rerun every weapon, nested-container or modded randomizer case.

## Repair direction

Use authoritative current authoring data for destination/ancestor comparisons,
preserving inventory configuration identities. Resolve native resource freshness
in its owning implementation; do not hide this with automatic Play/Workbench
reloads. Verify persisted source and refreshed outcomes, not only Save's boolean.
Repeat the failing cases before claiming save correctness. Recheck matching-base
normalization and randomizer readback once freshness is repaired.

## Reproduction and evidence

GC_LoadoutInheritanceAudit.c is optional test-addon code. Start the existing native
inventory fixture and catalogue. It starts with two US bandages; drag four more
into the jacket. `GC_TestInheritanceAudit` actions: count, base, child, freshChild,
duplicate, saveChild, removeOne, probeBase, probeChild, probeCount. Probe spawning
and counting use separate calls. Create refuses existing fixture filenames.
Resume takes base/child resource IDs, restricted to the InheritanceAudit directory
and known fixture filenames. Invalid user-base reference rejection was checked
in 8138abadb7b04ac88ccb7fea306571b1. Each Reload resets helper references.

Fresh compilation passed through de86135c6f6f46748599cbfa06c35b60; only the existing
ArlandLobby, dedicated-server-plugin and formation diagnostics remained.
Receipts and before/after fixture copies are in the tool repository's
`.temp/local/evidence/loadout-foundation/inheritance-audit`. Disposable runtime
fixtures are not shipped with Lobby2. Development Reloads were diagnostic steps,
not a proposed user workflow or production workaround.

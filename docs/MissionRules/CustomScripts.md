# Custom scripts

Both entries use one hook: derive from `GC_MissionScript` and override
`bool Run(GC_LobbyManager manager)`. Add `[BaseContainerProps()]` to your class
so it appears in the Script picker. Put it in your addon's `scripts/Game`.
Run can call your existing compiled functions.

- **Custom Script Action:** select the script in an event's Actions. True means
  success; false reports rejection. Event settings own delay, chance and
  repetition. False does not undo side effects, undo the event run, stop later
  actions or retry it.
- **Custom Script Condition:** select the script in event/flag Conditions.
  True passes; false fails. All/Any/None/count groups work normally. Keep the
  implementation quick and read-only.

One dispatcher and one hook contract serve both. There are no numbered slots,
function-name reflection, client RPCs or runtime text compilation.

## Minimal condition

```c
[BaseContainerProps()]
class MY_DepotExists : GC_MissionScript
{
    override bool Run(GC_LobbyManager manager)
    {
        return manager.GetOwner().GetWorld().FindEntityByName("Depot") != null;
    }
}
```

Select **MY_DepotExists** in **Custom Script Condition > Script**. To check the
opposite, place it inside a **None** group.

## Action: activate a placed spawner

```c
[BaseContainerProps()]
class MY_SpawnReinforcements : GC_MissionScript
{
    override bool Run(GC_LobbyManager manager)
    {
        IEntity entity = manager.GetOwner().GetWorld().FindEntityByName("ReinforcementArea");
        if (!entity)
            return false;
        GC_AreaPrefabSpawnerComponent spawner = GC_AreaPrefabSpawnerComponent.Cast(entity.FindComponent(GC_AreaPrefabSpawnerComponent));
        if (!spawner)
            return false;
        spawner.Spawn();
        return spawner.IsComplete();
    }
}
```

Disable Spawn On Init on that placed spawner. Select this script in a Custom
Script Action. The spawner keeps its once-per-instance behavior. This example
changes the world: do not use it as a condition.

For configurable values, add ordinary attributed fields to your subclass.
Override `IsValid()` to validate them, without side effects. Do not give
attributed fields explicit script initializers that override saved values.
The base hook itself is hidden and rejected if supplied directly.

## Timing and ownership

Conditions sample on Match entry and once per second afterward through the
existing framework tick. Events, flags and delayed actions reuse the cached
answer. There is no catch-up loop after a stall. Result/availability changes
notify the existing event system; continuously true does not itself repeatedly
trigger a repeatable event. Separate compiled conditions sample separately.
The limit is 256 script conditions per engine; overflow makes all script
conditions unavailable and logs an error.

Before Match, after Match or without a valid sample, a script condition is
unavailable and cannot pass through inversion. A valid false can be inverted.
Missing/invalid script settings fail closed. Actions run when their event runs.

The shared dispatcher requires the current, running, enabled framework on its
active server owner. Foreign/stopped engines cannot invoke hooks. Results are
discarded if a hook stops its owner. Conditions must not control the engine,
mutate the world or schedule callbacks. Keep custom logic bounded: compiled
mission scripts are trusted code, not isolated execution. Sphere queries remain
forbidden. No spatial queries or new global lifecycle overrides were added.

## Reference and tests

TilW installed commit `4a5300f94fb9323ce4942ef296bcffcdda922626`:
`TILW_Instructions.c:501` and `TILW_MissionFrameworkEntity.c:170` use numbered,
overridable framework methods. This keeps that small hook philosophy with typed
script objects, shared by actions and conditions. TilW's client-broadcast option
is not reproduced; these scripts are server-owned like existing mission actions.

Optional test addon: `GC_TestCustomScripts` loads a serialized CustomScripts
config and uses the real dispatcher and scheduler. Its temporary runtime engine
restores the original engine/enabled setting without changing mission resources
or the game-mode phase.

Custom script objects are authored settings reused by the framework. It does
not reset your own fields on an engine restart; keep runtime state in its
proper mission-owned system and use the hook to call it.

Live validation (Workbench 1.9.0.37, embedded Arland Play, 2026-11-11):
21 custom-script checks passed (r05459bd3122c47ca8e50f877b23a91cc), including
serialized subclass settings, event/flag execution, caching, repeats, inversion,
invalid/foreign/stopped rejection, delayed cancellation and shutdown during a
hook. Existing 42 core capture and 10 native vehicle/object checks passed.

The broader legacy suites reported the same three failures with both the new
engine and a control run using the unmodified pre-change engine: disabled rule,
event loop stops safely, flag feedback loop is bounded. Control receipts:
raaa01877fa80406383b53a3efca53034 (148 checks, two failures) and
r16ac8c836865403eb49940290ebbe33e (21 checks, one failure). The control replaced
only the engine implementation; other workspace files were retained. These
failures were not repaired in this change. Existing Arland resource diagnostics
remain. Dedicated-server peers and remote clients were not tested.
Final restored-engine verification: 21 script checks passed again
(`r5c7bfa15d5014613958be14b188c9c65`). Independent review approved the shared
hook, authority gate, sampling integration and documented limits.

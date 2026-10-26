# Mission Framework

An **Event** runs its **Actions** when its **Conditions** are met.

## Configure the game mode

1. Select your existing **GC_LobbyGameMode** entity in World Editor.
2. Select its **GC_LobbyManager** component, then expand **Mission Framework / Mission Events**.
3. Add a `GC_MissionEvent` with a unique Name, such as `US defeated`.
4. Choose **All conditions** or **Any condition**. Add **Living Players** and set
   Faction to a native key (`US`), Comparison to **At most**, **At least**, or
   **Exactly**, and Number to the desired count.
5. Add **End Mission**, set Winning Faction, and set **Delay after event starts**.

An empty Mission Events list disables automatic mission rules. No extra entity
or component is needed. Editing a world instance changes that mission; editing
its prefab changes inheriting missions unless they override the values.

`Prefabs/MP/Modes/Examples/GameMode_USDefeated.et` is a complete game-mode example:
US reaching zero living players starts a five-second delay, then ends the mission
with USSR victorious. It inherits the existing Lobby2 example game mode. Use it
as the mission's game mode, not as an additional entity beside another game mode.

The earlier standalone Mission Setup entity and prefab have been removed. If
used in a locally authored mission, transfer its event definitions into the game
mode's GC_LobbyManager / Mission Events array and remove that entity. No tracked production worlds
referenced the removed prefab. This is an explicit authoring-location change,
not a parallel compatibility executor.

If you authored events directly on the game-mode entity in the previous slice,
move that array into its existing GC_LobbyManager component. The included example
has been migrated; there is only one authoring location and executor.

## Exact behavior

- Events run once, only during GAME. Preview, slotting and briefing do not observe
  populations or start events. Evaluation runs every 250 ms of the game call queue.
- Living Players counts native connected players possessing living characters,
  by native player faction. Unconscious players count. AI, disconnected players,
  missing characters and Lobby2 spectator bodies do not.
- A faction must have a living player observed during GAME before its conditions
  can match. An unused faction cannot be treated as defeated. Disconnects and
  possession gaps can reduce the count; respawns can increase it. This is not a
  death counter or casualty percentage.
- Conditions share one fresh population snapshot per evaluation. The framework
  stores no player membership authority.
- Each action's delay is measured independently from its event's start in mission
  world time. Zero delay executes in that evaluation. Other actions execute on
  the first evaluation at or after their deadline. Actions due together follow
  event-list and action-list order.
- Conditions are not rechecked for queued actions. Ending the match or losing the
  owning lobby component cancels pending work. Actions are attempted once; failures
  are logged rather than retried.
- The complete nonempty event list is validated before execution. Missing types,
  unknown factions, invalid comparisons/counts/delays, empty event definitions
  and duplicate event names reject the whole configuration.
- Execution is server-only. Native game-mode replication publishes the mission
  outcome; no new client RPC or JIP execution history is introduced.

## Module ownership

The existing lobby component contains only the authored array, module ownership, and lifecycle
calls. All rule behavior remains in `scripts/Game/MissionFramework/`:

| File | Responsibility |
|---|---|
| `GC_MissionFramework.c` | Deferred initialization, update callback and teardown |
| `GC_MissionRuntime.c` | Validation, one-shot history, deadlines and execution |
| `GC_MissionEvent.c` | Event definition and All/Any composition |
| `GC_MissionCondition.c` | Condition contract and Living Players |
| `GC_MissionAction.c` | Action contract and End Mission |
| `GC_LobbyMissionContext.c` | Native observations, phase/authority checks, `EnterPostGame` integration |

New conditions/actions reuse the same executor. No scheduler or rule behavior
belongs in slotting, briefing, voice, admin code or the game-mode root.

This slice does not implement the full TilW catalog, nested groups, variables,
rearming, player rule-editing UI or entity pickers. See
[validation and coverage](../tests/MissionFramework.md).

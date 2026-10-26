# Mission Framework: first slice

An **Event** runs its **Actions** when its **Conditions** are met.

## Make a mission

1. Place `Prefabs/Logic/MissionFramework/MissionSetup.et` in a Lobby2 world.
   Place exactly one. Existing worlds are unchanged until they opt in.
2. Under **Mission Setup / Events**, add a `GC_MissionEvent` and give it a unique
   Name, such as `US defeated`.
3. Choose **All conditions** or **Any condition**. Add **Living Players**.
4. Set Faction to the native faction key (for example `US`), Comparison to
   **At most**, **At least**, or **Exactly**, and Number to the desired count.
5. Add **End Mission**, select the Winning Faction key, and set
   **Delay after event starts** in seconds.

`Prefabs/Logic/MissionFramework/Examples/USDefeated.et` is a complete example:
after US has had a living player observed during GAME, reaching zero living US
players starts a five-second delay, then ends the mission with USSR victorious.
Place the example instead of the empty setup. It requires US and USSR factions.

## Exact behavior

- Events run once and only during GAME. Preview, slotting and briefing do not
  observe populations or start events. Evaluation occurs every 250 ms of the
  game call queue; it is not an instantaneous death callback.
- Living Players counts native connected players possessing living characters,
  by their native player faction. Unconscious players count. AI, disconnected
  players, players without a character, and Lobby2 spectator bodies do not.
- A faction must have at least one living player observed during GAME before its
  conditions can match. An unused faction cannot be treated as defeated. This is
  a current-population condition, not a death counter or casualty percentage.
  Disconnects and temporary possession gaps can reduce it; respawns can increase it.
- All conditions in an evaluation use the same freshly captured population.
  No player membership is stored by the framework.
- Action delays are independent and measured from the event's start in mission
  world time. A zero delay executes in that evaluation. Actions execute on the
  first evaluation at or after their deadline. Actions due together are processed
  in event-list and action-list order.
- Once an event starts, changing its conditions does not cancel its actions.
  Ending the match, deleting Mission Setup, or losing the owning Lobby2 mode
  cancels pending work. An End Mission action prevents further gameplay actions
  after the phase changes.
- Actions are attempted once. Failures are logged, never silently retried.
- The entire setup is validated before it runs. Empty events, missing types,
  invalid factions/comparisons/counts/delays, and duplicate event names are rejected.
  Multiple setup entities stop execution rather than running competing rules.
- Execution is server-only. There is no new RPC, player permission, phase state,
  or JIP history to replay. Native game-mode replication publishes the result.

## Module ownership

All production behavior is under `scripts/Game/MissionFramework/`:

| File | Responsibility |
|---|---|
| `GC_MissionSetupEntity.c` | Workbench configuration, one setup per world, update callback and teardown |
| `GC_MissionRuntime.c` | Complete validation, one-shot history, deadlines, execution and cancellation |
| `GC_MissionEvent.c` | Authored event definition and All/Any composition |
| `GC_MissionCondition.c` | Condition contract and Living Players test |
| `GC_MissionAction.c` | Action contract and End Mission action |
| `GC_LobbyMissionContext.c` | Sole Lobby2 integration: native player observations, ownership/phase checks and `EnterPostGame` |

The runtime's lifecycle is `Start`, `Update`, `Stop`; `Describe` exposes a bounded
diagnostic summary. Setup owns the only repeating callback. Configuration objects
do not store execution history. No slotting, briefing, voice, admin, shared engine
override or existing game-mode source was changed.

Add future condition/action types to this module and reuse the same executor.
Keep native ownership and Lobby2 completion semantics in the context. Do not add
alternate schedulers or route mission endings around `EnterPostGame`.

This slice does not yet implement the remaining TilW catalog, nested condition
groups, mission variables, event rearming, player rule-editing UI, or entity pickers.
It does not migrate or modify TilW missions. The optional test addon is not a
production dependency. See [validation](../tests/MissionFramework.md).

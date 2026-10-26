# Mission Framework

An **Event** runs its **Actions** when its **Conditions** are met.

## Make a basic rule

1. Select your **GC_LobbyGameMode** entity.
2. Select its existing **GC_LobbyManager** component → **Mission Framework**.
3. Under **Mission Events**, add an event named **US defeated**.
4. Add **Player Casualties (%)**: Faction **US**, Comparison **At least**, Percentage **75**.
5. Add **End Mission**: Winning Faction **USSR**, Delay After Event Starts **5**.

This ends the mission five seconds after US losses reach 75%. The included
GameMode_USDefeated example uses 100%. Briefing Catalog and Parameter Catalog
remain in the same category. No additional framework entity is needed.

## Conditions

| Name | What it checks |
|---|---|
| Always | Eligible immediately when the event's phase permits it |
| Player Casualties (%) | Percentage of the faction's player strength lost |
| AI Casualties (%) | AI deaths as a percentage of observed AI lives |
| Flag Is Set | A named Mission Flag is on |
| Entity Damage State | A named entity or hit zone has the chosen damage state |
| AI Group State | A named AI group has the chosen native control state |
| Faction In Area (%) | Percentage of that faction's living members inside an area |
| Area Control (%) | That faction's share of the living members inside an area |
| Entities In Area (%) | Percentage of listed entities inside an area |
| Condition Held For | Stays true for a duration; false resets the timer |
| Capture Progress | Capture/loss takes time; interrupted progress gradually reverses |
| Condition Group | All, Any, At Least, At Most, Exactly, or All Same; supports nesting |

**All Same** means all true or all false. **Invert** reverses the result.
Top-level events keep the simple **All Conditions / Any Condition** choice.

Area conditions use a named center and radius in meters. A named polyline with
three or more points instead uses its X/Z polygon. Empty populations do not
match; missing named entities count as outside. Inversion deliberately reverses
these false results too.

Player casualties use **(peak living players − current living players) / peak**.
The peak is the highest simultaneous living-player count observed during GAME.
With a peak of eight and two still alive, casualties are 75%. Native factions and
possession determine membership. Unconscious players count as living; spectator
bodies do not. Disconnects and possession gaps can raise the percentage; respawns
can lower it. A faction with no observed living players does not match.

AI casualties count observed native AI lives and death callbacks during GAME,
including reinforcements. Deleting an entity is not a death.

## Actions

| Name | What it does |
|---|---|
| End Mission | Uses Lobby2's normal mission completion |
| Show Message / Show Chat Message | Displays a native hint or chat message |
| Spawn Prefab | Spawns at a named entity's position and rotation |
| Spawn Prefabs In Area | Distributes a chosen number of prefabs inside a circle or polygon |
| Spawn AI Group | Spawns a group and assigns named waypoints |
| Spawn Crewed Vehicle | Spawns a vehicle and fills selected seats |
| Set AI Hold Position | Disables named AI characters' movement controls |
| Set Deletion Protection | Protects named entities and their children from framework deletion |
| Set Cleanup Protection | Adds or removes native automatic-cleanup protection |
| Delete Entities In Area | Deletes objects selected by area and optional prefab filter |
| Rebuild Navigation | Requests a native navigation rebuild around changed objects |
| Assign Waypoints | Adds waypoints; optionally clears existing ones |
| Delete Entities | Deletes named entities and their children |
| Set Flag | Sets or clears a flag |
| Randomize Flag (%) | Sets on a successful roll; clears otherwise |
| Choose Random Flags | Draws from a pool; optional repeated draws and clearing first |
| Control Events | Run, Rearm, or Disable named events |
| Delete Empty Slots | Removes unclaimed/unreserved slots |
| Change Respawn Tickets | Changes a faction budget; optionally respawns dead players now |
| Set Briefing Visibility / Set Marker Visibility | Changes visibility and optionally faction audience |
| Run Custom Script | Runs a named entity's GC_MissionScriptComponent subclass |
| Service Vehicles | Repairs, sets fuel percentage, or sets supplies |
| Set Time / Set Weather | Uses the native time/weather manager |
| Set Entity Health (%) | Sets a default or named hit zone's health |

Spawns may receive a unique name for later rules. Existing names reject a spawn.
Crew groups support seat selection, custom characters, group names, delayed
waypoints, and separate idle gunners. Boarding completes asynchronously.

Tickets start at zero and cost one per successful respawn through the existing
slot spawn/possession path. **Respawn Now** is an explicit wave: unused tickets
do not automatically respawn future deaths. Arithmetic supports Set/Add/Subtract/
Multiply/Divide, rounds division down, clamps subtraction at zero, and rejects
budgets above 100000.

Briefing indexes are 1–32. Visibility changes retain existing phase gates.
Custom client scripts run on currently connected clients, not again for late
joiners. Messages can target factions or logged-in administrators.

## Flags and interactions

Basic events need no flags. Add a **Mission Flag** when several rules or an
interaction share a named state, such as **Radio destroyed**.

- Empty automatic condition: actions and interactions control the flag.
- Assigned automatic condition: it updates the flag every evaluation.
- **Keep Set**: an automatic flag remains set after first becoming true.
- **Randomize Initially**: rolls its starting value using Initial Chance.

For capture/loss messages, put **Capture Progress** in a flag's automatic
condition. Its six optional messages are named by status. Other events check
that flag; Keep Set makes a completed objective permanent.

The prefabs under Prefabs/Logic/MissionFramework provide **Set Flag** and
**Teleport** interactions. Configure title, factions, optional required flag,
and target flag/destination. They require a living possessed player during GAME.
The server checks ownership, faction, required flag and distance again before
applying the action. The prefab interaction radius is two meters; the authority
rejects users farther than five meters. Single Use hides a set-flag interaction
while its flag is set; clearing that flag makes it available again.

## Exact execution behavior

- Events run once by default, during GAME. **Allow Before Game** also permits
  preview, slotting and briefing. Casualties still wait for GAME.
- **Run** bypasses conditions on the next evaluation. **Rearm** resets trigger
  history and condition timers. **Disable** blocks triggers and cancels queued
  actions, including crew follow-ups. Rearm does not undo applied actions or
  cancel an earlier run's pending actions.
- Evaluation runs every 250 ms. Event chains advance on subsequent evaluations;
  circular chains cannot recurse in one call.
- Every action's delay starts independently at the event trigger. Conditions
  are not checked again for queued work. Due actions follow authored order.
- Automatic flags read the previous snapshot and publish together. Dependent
  automatic flags observe a change on the following evaluation.
- Mission end or loss of the owning component stops the scheduler. Failed
  actions are logged and never automatically retried.
- Event triggers, flag changes, capture changes, queued actions, results and
  cancellation always print under **GC | Mission Framework**.
- Configuration validation rejects the entire invalid rule set before running.
  Named entities resolve at execution so later spawns can be targets. Multi-target
  actions can partially apply before reporting failure.
- **Preview Mission End** logs End Mission without ending GAME. Disable this
  testing option for published missions.

The server owns rules and mutations. Flag snapshots support late joiners.
Existing catalog snapshots publish visibility changes. Native replication owns
spawned entities, player possession and the mission outcome.

## Module ownership

All behavior lives in scripts/Game/MissionFramework.

| Module | Responsibility |
|---|---|
| Framework / Runtime | Lifecycle, validation, scheduling, flags and event controls |
| Event / Condition / Action / Logic | Authored definitions and composition |
| Population / CaptureProgress | Observations and timed objective state |
| WorldConditions / WorldActions / SpawnActions | Entity, area, AI and world operations |
| Area / AreaSpawning / UtilityActions | Shared area geometry, distributed spawning and entity utilities |
| AISettings / TurretPolicy | AI movement restrictions and optional gunner dismount rule |
| LobbyContext / LobbyIntegration / LobbyActions | Lobby, catalog, slot and replication boundaries |
| Interactions | Native player actions and owner-delivered teleport |

New conditions/actions reuse this executor. Production has no test-addon dependency.
See [validation](../tests/MissionFramework.md) and [TILW mapping](MissionFramework-TILW.md).

## Migration

Earlier standalone Mission Setup or game-mode-root configuration must move into
the existing lobby component. **Living Players** is replaced by **Player
Casualties (%)**: choose a percentage intentionally rather than copying a count.
The tracked example is migrated. There is no second compatibility executor.

## AI hold behavior

**Set AI Hold Position** stops walking; it leaves perception and weapon controls
alone. Turn Hold off to permit movement again, then use **Assign Waypoints** if
the native AI abandoned its previous movement request while held. Existing group
orders are not rewritten automatically. A player taking control releases the
mission hold on the next framework evaluation (within 250 ms).

The action preserves movement restrictions present before it ran. Reforger exposes
one shared movement-disabled boolean, so do not have another script independently
write that same property during a hold: release cannot identify a later writer.
# Lobby2 mission events

Select your game mode, then **GC_LobbyManager > Lobby2**. The top settings are **Mission Events**, **Mission Flags**, **Briefing Catalog**, and **Parameter Catalog**. Mission Events Enabled and Parameter Lock Seconds remain in Unsorted.

## Start with one mission event

1. Add a **Mission Event** and give it a name.
2. Add its **Event**, then choose **Type** from the dropdown: Match started, Timer elapsed, Named event received, Flag turned On, or Flag turned Off. An empty Event means Match started.
3. Add its **Actions**. They normally run in order.
4. Leave **Conditions**, **Advanced**, and **Random** empty unless needed.

A mission event runs once by default. **Advanced > Run > Every time** allows later matching events to run it again. **Wait Seconds** adds a fixed delay. Disabled keeps the mission event saved without running it.

**Flags are optional.** Match-start, timer and named-event mission events work without flags, flag definitions or conditions. The no-flags example below demonstrates this.

## The terms

| Setting | Meaning |
| --- | --- |
| Mission Event | An event, optional conditions, and actions to run. |
| Event | Something that happens now and starts a mission event. It is not remembered. |
| Condition | A requirement checked when the event happens, and again after any wait. |
| Action | Something the mission event does. |
| Flag | A named On/Off value the mission remembers. Use it only when useful. |

**Mission Flags** optionally gives names starting values. Initial values are applied before Match started mission events and do not fire Flag changed events. Undeclared flags start Off and can still be set by actions. Names are exact and case-sensitive. Setting a flag to its current value does not send another event. Duplicate or invalid initial flag definitions prevent mission events from starting and report an error.

## Conditions: standard logic objects

Conditions are an **object list**. Use **+** to choose an operation. Each group contains its own Conditions list, so groups can be nested and combined. There is no per-condition Type dropdown.

| Condition object | Plain meaning |
| --- | --- |
| All are true (AND) | Every child must be true. |
| At least one is true (OR) | One or more children must be true. |
| None are true (NOR) | Every child must be false. With one child, this reverses its answer. |
| At least one is false (NAND) | Passes unless every child is true. |

**None replaces Not.** Any group can contain other groups. XOR and XNOR have been removed.

Count comparisons have **Required Count** and their own **Conditions** list:

| Condition object | Meaning |
| --- | --- |
| Exactly this many are true (=) | The number of true children equals Required Count. |
| A different number are true (!=) | The number differs from Required Count. |
| Fewer than this many are true (<) | The number is below Required Count. |
| At most this many are true (<=) | The number is no greater than Required Count. |
| More than this many are true (>) | The number exceeds Required Count. |
| At least this many are true (>=) | The number reaches or exceeds Required Count. |

For **two out of three**, add **At least this many are true**, set Required Count to **2**, and add three conditions. Each child can itself be an All, None, another count comparison, or any other condition group. Each child contributes one true-or-false answer, regardless of how many conditions it contains.

For example, **At least 2** can contain **All [A, B]**, **None [C]**, and **D**. It passes when two or three of those branches pass. This is the nested layout demonstrated by the no-flags example. Another valid layout is **Any [All [A, B], All [C, D]]**.

These comparisons count **satisfied conditions**, not people or entities. Future area, faction-survival and prefab checks will decide their own true-or-false answers before the enclosing group counts them. Mission events are not condition inputs yet. A **True or false value** supplies a fixed input for testing: checked = true, unchecked = false. It is not a flag or a live check.

Every condition directly under a mission event must pass, so ordinary lists do not need an extra All group. An empty event Conditions list means no restrictions. An explicit group needs at least one child. Empty groups, missing children and invalid required counts disable the event rather than becoming true under a negative operator. Nesting is limited to 16 levels and lists to 256 children.

Flag On/Off condition objects have been removed from this authoring slice. Special conditions and live value sources will be added later. Existing Mission Flags, flag actions and starting-event options remain separate. Moving events, timing and randomness into conditions is also a future slice.

The shared serialized base is marked non-insertable using the engine convention. Some Workbench class pickers may still expose **GC_MissionCondition**, the incomplete shared base. Choose a concrete operation instead. Incomplete base entries fail closed.

## Optional randomness

Add **Random** only to mission events that need it:

| Mission-maker intent | Setting |
| --- | --- |
| Sometimes run this event | Chance Percent, for example 25. |
| Wait an unpredictable time after an event | Minimum Seconds and Maximum Seconds, for example 10–30. |
| Pick one outcome without flags | Action Selection > One random action. |
| Sometimes turn a flag On | Random flag action with Chance Percent. Failure sets it Off. |
| Choose a route or several objectives | Select random flags: a list and Selection Count. Selected flags turn On; others in that list turn Off. |

Chance is checked once per matching event after conditions pass. Zero never runs; 100 always runs. A failed chance waits for a new event. A delay is sampled once, added to the fixed wait, and conditions are rechecked when it ends. Events received while waiting are ignored. Repeat timers skip missed ticks instead of executing a burst.

Random action selection chooses one whole action uniformly when execution begins. Random flag selection chooses distinct entries without modifying the configured list. No weighted selection or full gameplay action catalog is included yet.

Use Match started plus 50% chance for an uncertain opening event; Named event received plus a random delay for a response to an earlier event; or a repeating Timer with a chance for occasional activity. Use flags only when later mission events must remember or react to a selected outcome.

## Included examples

- `Prefabs/MP/Modes/GameMode_Lobby_NoFlagsExample.et`: no flags; it demonstrates a two-out-of-three comparison containing All/None groups. After a two-second timer and a random extra 1–3 seconds, one of two test log messages runs once.
- `tests/McpTestAddon/worlds/StandardLogicExample.ent`: optional editor scene for inspecting the nested count example.
- `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`: two mission events connected by a flag. First sets test_ready; second observes that change and sets test_complete.

Existing actions are Log message, Set flag, Send test event, Random flag, and Select random flags. Named events can be sent by the sample action or the server-side manager SendTestEvent method. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts mission events, after briefing at GAME. Shutdown and POSTGAME cancel pending work. Mission events/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. Limits are 256 mission events, initial flags, actions, or children per group; nesting is limited to 16. A bounded event queue stops runaway chains. Malformed mission events fail closed and report their index.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. The preceding GC_MissionCondition + m_iType format also changed: All/Any become their list objects, and None becomes the None group. Old On/Off flag checks require a deliberate update when special conditions return; they are not replaced with fixed values. Count comparisons now count satisfied child conditions. The known project example and open Arland condition were migrated. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: 439 checks passed with zero failures (251 standard logic, 148 interpreter, 30 authoring, six serialized nested-example and four flag-action example checks). Tests cover Boolean truth tables, count comparison boundaries, empty/malformed groups, cycles, nested groups and production execution. Known unrelated Arland world GUID and missing menu-preset diagnostics remain in this test project.


TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework also supports direct random action selection without flags and event-based random delays.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

The editor label Mission Rules is now Mission Events. The project's saved list and enable fields were migrated to m_aMissionEvents and m_bMissionEventsEnabled; external resources using the old names need the same rename. Internal class names remain unchanged.

### Updating the preceding condition-object slice

GC_NotCondition with one m_Condition becomes GC_NoneConditions with that condition as the sole item in m_aConditions. This preserves NOT behavior. Flag On/Off authoring classes have been removed; external missions using them require a deliberate update when special conditions return. No flag check is silently converted to a fixed value. The shipped example's redundant initial flag check was removed; its match-start outcomes remain covered by tests. The user's saved Arland scene had no remaining flag or Not conditions at this update.

### Updating the count-comparison slice

The six comparison classes now use m_iRequiredCount and m_aConditions instead of fixed m_fFirstNumber/m_fSecondNumber values. Rebuild external fixed-number comparisons deliberately; those values cannot be converted into live checks automatically. Empty comparison groups fail closed. The project example was migrated; the saved Arland comparison had no explicit number values. Removed XOR/XNOR classes require deliberate replacement in external resources; old internal mode values 8 and 9 are rejected rather than reused.

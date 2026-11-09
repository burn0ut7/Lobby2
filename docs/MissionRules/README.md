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

Conditions are an **object list**. Use **+** to choose **Condition Group** or **Condition Count**. Both contain their own Conditions list, so they can be nested inside each other. A group has no number field; a count has a numeric threshold.

**Condition Group > Match** offers four choices:

| Match | Plain meaning |
| --- | --- |
| All are true (AND) | Every child must be true. |
| Any are true (OR) | One or more children must be true. |
| None are true (NOR) | Every child must be false. With one child, this reverses its answer. |
| Not all are true (NAND) | Passes unless every child is true. |

**None replaces Not.** Any group can contain other groups. XOR and XNOR have been removed.

Add one **Condition Count** object for number comparisons. It has three settings:

1. **Comparison**: Greater than (>), Less than (<), or Equal to (=).
2. **Required Count**: the number to compare against.
3. **Conditions**: the checks being counted. Each can contain nested groups.

| Comparison with Required Count = 2 | Passes when |
| --- | --- |
| Greater than | 3 or more listed conditions are true. |
| Less than | 0 or 1 listed conditions are true. |
| Equal to | Exactly 2 listed conditions are true. |

For **at least two out of three**, use **Greater than**, set Required Count to **1**, and add three conditions. Each child contributes one true-or-false answer, regardless of how many conditions it contains. For exactly two, choose Equal to and set the count to 2.

For example, **Greater than 1** can contain **All [A, B]**, **None [C]**, and **D**. It passes when two or three of those branches pass. The optional script tests exercise this nested layout. Another valid layout is **Any [All [A, B], All [C, D]]**.

These comparisons count **satisfied conditions**, not people or entities. Future area, faction-survival and prefab checks will decide their own true-or-false answers before the enclosing group counts them. Mission events are not condition inputs yet. The fixed **True or false value** placeholder has been removed. Script-only test inputs live in the optional test addon and are not authored as mission conditions. Until special checks arrive, leave Conditions empty for unrestricted events; empty explicit groups still fail closed.

Every condition directly under a mission event must pass, so ordinary lists do not need an extra All group. An empty event Conditions list means no restrictions. An explicit group needs at least one child. Empty groups, missing children and invalid required counts disable the event rather than becoming true under a negative operator. Nesting is limited to 16 levels and lists to 256 children.

Flag On/Off condition objects have been removed from this authoring slice. Special conditions and live value sources will be added later. Existing Mission Flags, flag actions and starting-event options remain separate. Moving events, timing and randomness into conditions is also a future slice.

The shared serialized base is marked non-insertable using the engine convention. Some Workbench class pickers may still expose **GC_MissionCondition**, the incomplete shared base. Choose a concrete operation instead. Incomplete base entries fail closed.

## Optional randomness

Add **Random** only to mission events that need it:

| Mission-maker intent | Setting |
| --- | --- |
| Sometimes run this event | Chance Percent, for example 25. |
| Wait an unpredictable time after an event | Minimum Seconds and Maximum Seconds, for example 10-30. |
| Pick one outcome without flags | Action Selection > One random action. |
| Sometimes turn a flag On | Random flag action with Chance Percent. Failure sets it Off. |
| Choose a route or several objectives | Select random flags: a list and Selection Count. Selected flags turn On; others in that list turn Off. |

Chance is checked once per matching event after conditions pass. Zero never runs; 100 always runs. A failed chance waits for a new event. A delay is sampled once, added to the fixed wait, and conditions are rechecked when it ends. Events received while waiting are ignored. Repeat timers skip missed ticks instead of executing a burst.

Random action selection chooses one whole action uniformly when execution begins. Random flag selection chooses distinct entries without modifying the configured list. No weighted selection or full gameplay action catalog is included yet.

Use Match started plus 50% chance for an uncertain opening event; Named event received plus a random delay for a response to an earlier event; or a repeating Timer with a chance for occasional activity. Use flags only when later mission events must remember or react to a selected outcome.

## Included examples

- `Prefabs/MP/Modes/GameMode_Lobby_NoFlagsExample.et`: no flags and no conditions; it demonstrates an unrestricted timer event. After a two-second timer and a random extra 1-3 seconds, one of two test log messages runs once.
- `tests/McpTestAddon/worlds/StandardLogicExample.ent`: optional editor-only scene for inspecting the no-flags timer example. Run runtime tests in Arland; this minimal inspector scene lacks a faction manager required by Play.
- `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`: two mission events connected by a flag. First sets test_ready; second observes that change and sets test_complete.

Existing actions are Log message, Set flag, Send test event, Random flag, and Select random flags. Named events can be sent by the sample action or the server-side manager SendTestEvent method. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts mission events, after briefing at GAME. Shutdown and POSTGAME cancel pending work. Mission events/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. Limits are 256 mission events, initial flags, actions, or children per group; nesting is limited to 16. A bounded event queue stops runaway chains. Malformed mission events fail closed and report their index.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. The preceding GC_MissionCondition + m_iType format also changed: recreate groups using Condition Group and its Match setting. Old On/Off flag checks require a deliberate update when special conditions return; they are not replaced with fixed values. Count comparisons now count satisfied child conditions. The known project example and open Arland condition were migrated. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: 377 checks passed with zero failures in Arland (189 standard logic, 148 interpreter, 30 authoring, six serialized no-flags example and four flag-action example checks). Tests cover Boolean truth tables, count comparison boundaries, empty/malformed groups, cycles, nested groups and production execution. Known unrelated Arland world GUID and missing menu-preset diagnostics remain in this test project.


TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework also supports direct random action selection without flags and event-based random delays.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

The editor label Mission Rules is now Mission Events. The project's saved list and enable fields were migrated to m_aMissionEvents and m_bMissionEventsEnabled; external resources using the old names need the same rename. Internal class names remain unchanged.

### Migrating saved conditions to this version

The editor now uses two composite types:

- **GC_ConditionGroup**: m_iMatch (0 All, 1 Any, 2 None, 3 Not all) and m_aConditions. No number field.
- **GC_ConditionCount**: m_iComparison (0 Greater than, 1 Less than, 2 Equal to), m_iRequiredCount and m_aConditions.

Replace the former All/Any/None/NotAll classes with Condition Group and the matching mode, keeping their children. An old GC_NotCondition becomes a None group with its former m_Condition as the sole child.

Replace separate comparison classes with Condition Count. For integer counts, >= N can become > N-1 when N is positive; <= N can become < N+1 when the new threshold remains within 0-256. Boundary cases require a deliberate equivalent group; do not enter an invalid threshold. Not-equal can be expressed as a None group containing an Equal to count. Old fixed m_fFirstNumber/m_fSecondNumber comparisons need deliberate rebuilding; they do not represent live checks automatically.

The shipped no-flags example now has no conditions, preserving its timer and random action behavior without fixed Boolean placeholders. The user's empty Greater than object was renamed without changing its meaning. External resources need the same conversions before use. Removed XOR/XNOR classes require deliberate replacement; internal values 8 and 9 are rejected rather than reused. Internal legacy count modes remain for historical interpreter behavior, not as editor choices.

Flag On/Off authoring classes remain removed; external missions using them need a deliberate update when special conditions return. No flag check is silently converted to a fixed value.

The GC_BooleanCondition authoring class has been removed. External resources using it need deliberate removal or replacement with appropriate real checks when available; do not delete a false restriction without considering how that changes the event. No saved world in the inspected project referenced this class at removal.

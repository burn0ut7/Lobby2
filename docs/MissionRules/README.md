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
| All conditions (AND) | Every child must be true. |
| Any condition (OR) | At least one child must be true. |
| None of the conditions (NOR) | Every child must be false. With one child, this is NOT. |
| Not all conditions (NAND) | At least one child must be false. |
| Odd number true (XOR) | An odd number of children must be true: 1, 3, 5, and so on. With two children, exactly one. |
| Even number true (XNOR) | An even number of children must be true: 0, 2, 4, and so on. With two children, their answers must match. |

**None replaces Not.** Put one condition inside None to reverse its answer. Put several inside None to require that none is true. A group is itself a condition, so it can go inside any other group.

**XOR is standard Boolean parity**, not a general "exactly one" counter. With three true children XOR is true, while XNOR is false. XNOR does not mean "all agree" when there are more than two children.

Numeric operations are separate objects with **First Number** and **Second Number**:

| Object | Example that is true |
| --- | --- |
| Equal to (=) | 3 = 3 |
| Not equal to (!=) | 3 != 4 |
| Less than (<) | 3 < 4 |
| Less than or equal (<=) | 3 <= 3 |
| Greater than (>) | 4 > 3 |
| Greater than or equal (>=) | 3 >= 3 |

These inputs are **fixed numbers**, not live mission values. Decimals and negative numbers are supported, from -1 billion to 1 billion. Equality compares the entered engine float values exactly; there is no hidden tolerance. A **True or false value** object supplies a fixed Boolean input: checked = true, unchecked = false. It is not a flag.

For example, an All group can contain **Greater than: 5, 3** and a **None** group containing **Less than: 5, 3**. Both branches pass. The no-flags example demonstrates this exact nested layout.

Every condition directly under a mission event must pass, so ordinary lists do not need an extra All group. An empty event Conditions list means no restrictions. An explicit group needs at least one child. Empty groups, missing children and invalid numeric inputs disable the event rather than becoming true under a negative operator. Nesting is limited to 16 levels and lists to 256 children.

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

- `Prefabs/MP/Modes/GameMode_Lobby_NoFlagsExample.et`: no flags; it demonstrates nested All/None and numeric comparisons. After a two-second timer and a random extra 1–3 seconds, one of two test log messages runs once.
- `tests/McpTestAddon/worlds/StandardLogicExample.ent`: optional editor scene for inspecting the nested numeric example.
- `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`: two mission events connected by a flag. First sets test_ready; second observes that change and sets test_complete.

Existing actions are Log message, Set flag, Send test event, Random flag, and Select random flags. Named events can be sent by the sample action or the server-side manager SendTestEvent method. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts mission events, after briefing at GAME. Shutdown and POSTGAME cancel pending work. Mission events/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. Limits are 256 mission events, initial flags, actions, or children per group; nesting is limited to 16. A bounded event queue stops runaway chains. Malformed mission events fail closed and report their index.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. The preceding GC_MissionCondition + m_iType format also changed: All/Any become their list objects, and None becomes the None group. Old On/Off flag checks require a deliberate update when special conditions return; they are not replaced with fixed values. Specialized count conditions are deferred; they are not silently converted. The known project example and open Arland condition were migrated. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: this slice passed 309 checks: 148 interpreter, 30 authoring, 121 standard-logic, four serialized flag-action example checks, and six serialized nested-logic example checks. Standard-logic checks cover every one-, two- and three-input Boolean truth table, numeric comparison outcomes, empty/malformed groups, cycles, mixed nesting and production execution. Known unrelated Arland world GUID and missing menu-preset diagnostics remain in this test project.


TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework also supports direct random action selection without flags and event-based random delays.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

The editor label Mission Rules is now Mission Events. The project's saved list and enable fields were migrated to m_aMissionEvents and m_bMissionEventsEnabled; external resources using the old names need the same rename. Internal class names remain unchanged.

### Updating the preceding condition-object slice

GC_NotCondition with one m_Condition becomes GC_NoneConditions with that condition as the sole item in m_aConditions. This preserves NOT behavior. Flag On/Off authoring classes have been removed; external missions using them require a deliberate update when special conditions return. No flag check is silently converted to a fixed value. The shipped example's redundant initial flag check was removed; its match-start outcomes remain covered by tests. The user's saved Arland scene had no remaining flag or Not conditions at this update.

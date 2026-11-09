# Lobby2 mission events

Select your game mode, then **GC_LobbyManager > Lobby2**. The top settings are **Mission Events**, **Mission Flags**, **Briefing Catalog**, and **Parameter Catalog**. Mission Events Enabled and Parameter Lock Seconds remain in Unsorted.

## Start with one mission event

1. Add a **GC_Event** and give it a name.
2. In **Conditions**, add **Game Mode State**. Choose a phase and **Is in this state** or **Is not in this state**.
3. Add **Actions**. They normally run in order.
4. Leave **Advanced** empty unless needed.

For game start, choose **Match / Is in this state**. The field order is Name, Conditions, Actions, Advanced. There is no separate Event object or event-type selector.

A mission event runs once by default. **Advanced > Run > Every time** permits a new run on each later matching state change. Staying in the same state does not repeat it. **Wait Seconds** adds a delay; Disabled keeps the event saved without running it.

**Flags are optional.** They are named On/Off values remembered by the mission, not prerequisites for state-based events. Mission Flags supplies optional starting values before the first condition check. Names are exact and case-sensitive; undeclared flags start Off. Invalid or duplicate initial flags prevent startup. Setting flags does not recheck Game Mode State conditions.

## Game Mode State

Choose **Mission preview**, **Role selection**, **Briefing**, **Match**, or **After action**. These are the existing authoritative Lobby2 phases; this feature does not introduce another phase system.

- **Is in this state** passes when that phase is active.
- **Is not in this state** passes when a different valid phase is active.

The framework checks the initial state after the world loads, then listens through GC_LobbyManager's existing OnGameStateChanged callback. Duplicate notifications and ordinary ticks do not trigger another check. A late startup observes the current state; it does not invent a transition. The initial state can satisfy an Is not in condition too.

State conditions can appear anywhere inside Condition Group or Condition Count. A rule containing a state condition is checked on startup and every distinct state change. All top-level conditions must pass. A rule with no state conditions keeps the simple default: check once when Match begins.

Actions waiting on a delay are rechecked when due. A state change that makes their conditions false cancels the pending run. If the conditions remain true, the original wait continues. Explicit After action conditions and their delays may run after the match; shutdown cancels everything. Conditions with no valid state context fail closed, even inside None.

## The terms

| Setting | Meaning |
| --- | --- |
| Mission Event | A name, conditions, and actions to run when the conditions pass. |
| Condition | A requirement checked on relevant changes, and again after a wait. |
| Action | Something the mission event does. |
| Flag | An optional named On/Off value the mission remembers. |

## Conditions: standard logic objects

Conditions are an **object list**. Use **+** to choose **Game Mode State**, **Condition Group**, or **Condition Count**. Group and Count contain their own Conditions list, so they can be nested inside each other. A group has no number field; a count has a numeric threshold.

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

These comparisons count **satisfied conditions**, not people or entities. Future area, faction-survival and prefab checks will decide their own true-or-false answers before the enclosing group counts them. Mission events are not condition inputs yet. The fixed **True or false value** placeholder has been removed. Script-only test inputs live in the optional test addon and are not authored as mission conditions. Game Mode State is the first live check. Empty explicit groups still fail closed.

Every condition directly under a mission event must pass, so ordinary lists do not need an extra All group. **No conditions means always true:** an empty event Conditions list has no requirements to meet. Top-level events and random groups with no conditions run at match start; a child with no conditions is eligible whenever its group makes a selection. Chance, delay, disabled and run-once settings still apply. An explicit group needs at least one child. Empty groups, missing children and invalid required counts disable the event rather than becoming true under a negative operator. Nesting is limited to 16 levels and lists to 256 children.

Flag On/Off condition objects have been removed from this authoring slice. Area, faction, prefab, flag and timer conditions remain future slices. Mission Flags and flag actions remain available; timing and randomness have not moved into conditions yet.

The shared serialized base is marked non-insertable using the engine convention. Some Workbench class pickers may still expose **GC_MissionCondition**, the incomplete shared base. Choose a concrete operation instead. Incomplete base entries fail closed.

## Random chance and random groups

**Advanced > Random Chance** is a number from 0 to 1: **0.8 means 80%**, **1 means 100%**, and **0 means never**. The default is 1. Leaving Advanced unset or omitting the saved chance value means 100%. Chance controls whether actions run; it only spawns something if an action does so. The old separate Random object is removed.

Chance is checked once after conditions pass, before the wait. A failed chance waits for a new matching state change; it is not retried every tick. Conditions are checked again after the wait.

In **Mission Events**, choose either **GC_Event** or **GC_RandomEventGroup**:

- **GC_Event**: Name, Conditions, Actions, Advanced.
- **GC_RandomEventGroup**: Name, Conditions, Pick Count, Events, Advanced.

A random group chooses the requested number of different child events whose conditions currently pass. Disabled, waiting and completed once-only children are excluded. If fewer qualify, it picks those available. It never picks the same list entry twice in one selection. If none qualify, a once-only group stays available for a later matching state change.

The group's conditions decide when selection happens. Child conditions filter that selection; they do not trigger the group independently. Each selected child's own chance and delay still apply, so Pick Count is the number selected, not a guarantee that that many actions execute. A child that fails its chance is not replaced by another pick.

The group's delay happens before selection; child delays happen afterward. Waiting children must continue to satisfy both their own and their parent's conditions. A once-only group uses its one selection even if a selected child's chance fails or its later wait is canceled. Set the group's Run to Every time for later matching state changes; children retain their own Once/Every time settings.

Groups contain regular events, not other random groups. Conditions still nest normally. No weighted selection or full gameplay catalog is included. Random flag and Select random flags remain optional test actions; flags are not needed for event selection.

## Included examples

- `Prefabs/MP/Modes/GameMode_Lobby_NoFlagsExample.et`: no flags; Match / Is in this state starts a random group with a fixed two-second wait and Pick Count 1, selecting one of two regular test-log events. There is no extra random delay.
- `tests/McpTestAddon/worlds/StandardLogicExample.ent`: optional editor-only scene for inspecting the no-flags state example. Run runtime tests in Arland; this minimal inspector scene lacks a faction manager required by Play.
- `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`: two independent state-driven mission events. Match sets test_ready; After action sets test_complete. They demonstrate optional flag actions, not flag-triggered events.

Existing actions are Log message, Set flag, Send test event, Random flag, and Select random flags. Send test event remains an interpreter/testing action; this slice has no authorable named-event condition to receive it. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts the framework, after world load, and supplies state observations. It uses the existing component callback with no additional global subscription. Shutdown and loss of active mode ownership stop pending work. Explicit After action conditions can run in POSTGAME; old non-state delayed/timer work is canceled there. Mission events/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. The total event limit is 256, including random groups and their children. Each random group holds up to 255 regular events, with Pick Count from 1 to its configured child count. Initial flags, actions and condition lists are limited to 256; condition nesting is limited to 16. A bounded event queue stops runaway chains. Malformed mission events fail closed and report an error.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. The preceding GC_MissionCondition + m_iType format also changed: recreate groups using Condition Group and its Match setting. Old On/Off flag checks require a deliberate update when special conditions return; they are not replaced with fixed values. Count comparisons now count satisfied child conditions. The known project example and open Arland condition were migrated. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: 509 checks passed (30 event-group/chance, 85 state conditions, 189 standard logic, 148 interpreter, 22 authoring, six no-flags resource, four state-example resource, five historical-fixture and 20 randomness checks). Real Lobby2 transitions were also tested through Preview -> Role selection -> Briefing -> Match -> After action: each action fired once, duplicate notifications were ignored, unauthenticated phase changes were rejected, and stopping prevented further execution. The temporary test configuration was discarded by stopping Play. Dedicated/remote-client behavior was not newly tested. Known unrelated Arland GUID, missing menu-preset and Workbench tool-leak diagnostics remain.

Installed engine source: game/data/data007.pak/scripts/Game/GameMode/SCR_BaseGameMode.c (SHA256 c02bf603991e03e956d880ffaf643cfbde46f87a7d31f2fb1fc0918e95c16652), lines 434-465, dispatches OnGameStateChanged to components on server and clients. The manager gates mission execution to the active authoritative Lobby2 owner.


TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework supports random selection of regular events without flags.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

The editor label Mission Rules is now Mission Events. The project's saved list and enable fields were migrated to m_aMissionEvents and m_bMissionEventsEnabled; external resources using the old names need the same rename. The regular authoring class is now GC_Event; internal interpreter names retain MissionRule.

### Migrating saved conditions to this version

The editor now uses two composite types:

- **GC_ConditionGroup**: m_iMatch (0 All, 1 Any, 2 None, 3 Not all) and m_aConditions. No number field.
- **GC_ConditionCount**: m_iComparison (0 Greater than, 1 Less than, 2 Equal to), m_iRequiredCount and m_aConditions.

Replace the former All/Any/None/NotAll classes with Condition Group and the matching mode, keeping their children. An old GC_NotCondition becomes a None group with its former m_Condition as the sole child.

Replace separate comparison classes with Condition Count. For integer counts, >= N can become > N-1 when N is positive; <= N can become < N+1 when the new threshold remains within 0-256. Boundary cases require a deliberate equivalent group; do not enter an invalid threshold. Not-equal can be expressed as a None group containing an Equal to count. Old fixed m_fFirstNumber/m_fSecondNumber comparisons need deliberate rebuilding; they do not represent live checks automatically.

The shipped no-flags example now uses a Match state condition and a fixed wait, using a once-only random group without Boolean placeholders. The user's empty Greater than object was renamed without changing its meaning. External resources need the same conversions before use. Removed XOR/XNOR classes require deliberate replacement; internal values 8 and 9 are rejected rather than reused. Internal legacy count modes remain for historical interpreter behavior, not as editor choices.

Flag On/Off authoring classes remain removed; external missions using them need a deliberate update when special conditions return. No flag check is silently converted to a fixed value.

The GC_BooleanCondition authoring class has been removed. External resources using it need deliberate removal or replacement with appropriate real checks when available; do not delete a false restriction without considering how that changes the event. No saved world in the inspected project referenced this class at removal.

### Migrating the former Event selector

Remove the obsolete GC_MissionEvent.c file when installing a source snapshot. GC_MissionRule.m_Event and the GC_MissionEvent authoring type are removed. Match started becomes a Game Mode State condition: m_eGameModeState = 1 (GAME), m_iMatch = 0 (Is in). Invert uses m_iMatch = 1. The saved empty Match-start selector in the inspected Arland scene was converted.

A once-only Timer after match start can use Match / Is in with Advanced > Wait Seconds; this is how the no-flags example is migrated. Repeating timers, named-event receivers and flag-change triggers need deliberate migration when those conditions are added; they are not silently converted to unconditional events. The old two-event flag-chain sample is deliberately replaced with independent Match and After action examples. The compiled interpreter still supports its historical triggers for regression/extension callers, but they are no longer editor options.

### Migrating regular events and randomness

Rename saved GC_MissionRule objects to **GC_Event** and remove obsolete GC_MissionRule.c when installing a source snapshot. The Mission Events list now accepts GC_Event and GC_RandomEventGroup through its shared serialized base GC_EventDefinition. Incomplete base entries fail closed; choose a concrete event type if a picker exposes the base.

Move old Random > Chance Percent into Advanced > Random Chance, dividing by 100. Remove m_Random from authored events. Convert One random action into a random group with Pick Count 1 and one regular child event per former action. Old additional random delays require deliberate redesign; they are not silently preserved. The shipped no-flags example deliberately changes its former fixed-plus-random delay to a fixed two seconds. Historical random behavior remains available only to compiled interpreter callers and regression tests.

This slice's live checks include saved example deserialization, default chance when omitted, chance boundaries, filtering, distinct picks, insufficient eligible children, delays, cancellation and repeat behavior. Full native object-class picker options and dedicated/remote-client behavior were not newly verified.

Script callers constructing authoring containers directly must set their fields explicitly (including Random Chance = 1). Attribute defaults apply when Workbench loads serialized containers. Compiled runtime rules default to full chance. Field initializers are intentionally avoided on attributed fields because they can overwrite serialized settings.

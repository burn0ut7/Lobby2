# Lobby2 mission events

Select your game mode, then **GC_LobbyManager > Lobby2**. The top settings are **Mission Events**, **Mission Flags**, **Briefing Catalog**, and **Parameter Catalog**. Mission Events Enabled and Parameter Lock Seconds remain in Unsorted.

## Start with one mission event

1. Add a **GC_Event** and give it a name.
2. In **Conditions**, add **Game Mode State**. Choose a phase and **Is in this state** or **Is not in this state**.
3. Add **Actions**. They normally run in order.
4. Leave **Advanced** empty unless needed.

For game start, choose **Match / Is in this state**. The field order is Name, Conditions, Actions, Advanced. There is no separate Event object or event-type selector.

A mission event runs once by default. **Advanced > Run > Every time** permits a new run on each later matching state change. Staying in the same state does not repeat it. **Delay** adds a wait measured in seconds; Disabled keeps the event saved without running it.

**Flags are optional.** Use them when a mission needs to remember something or notify other events. Each declared Mission Flag remembers its On/Off state and the number of accepted calls.

## Mission Flags: memory and calls

A Mission Flag has **Name**, **Starting Value**, **Repeatable**, and **Conditions**. Use the **Call Flag** action with the same exact name.

- **Repeatable off** (default): accepts one call, becomes On and stays On. Later calls do nothing.
- **Repeatable on**: becomes On while dependent events and flags are checked, then resets to Off. Its count is remembered, so a later call can activate it again.
- **Conditions**: requirements for accepting a call. No conditions means always true. Conditions are checked before increasing the count; blocked calls do not count or notify anything.

For **at most five activations**, name the flag `wave`, turn on Repeatable, and add **Count Condition** to its Conditions: Count From **Mission flag calls**, Flag Name `wave`, Comparison **Less than**, Required Count **5**. Counts 0 through 4 allow a call; each accepted call adds one. The fifth succeeds, and later calls are blocked.

Mission events and flags can use two condition objects, nested inside the usual groups:

| Condition | Meaning |
| --- | --- |
| Mission Flag Condition | Check whether a named flag is On or Off. |
| Count Condition | Count passing conditions or a named flag's accepted calls, then compare using Greater than, Less than, Equal or Not equal. |

**Count Condition** handles both uses. Choose **Count From** to select what is counted. There is no separate Flag Call Count condition.

When a flag changes, events and flags that reference it are checked. A flag's conditions can therefore form a chain: `second` with Mission Flag Condition `first` Is true activates when `first` is called. A flag checking only its own count does not call itself; that condition limits incoming calls. A Game Mode State condition on a flag also lets a matching startup/state change call it. No conditions means no automatic trigger: call the flag with an action.

Each flag can activate at most once in one check cycle, and each eligible dependent mission event gets one attempt in that cycle. This prevents circular chains from repeatedly firing. A later external call starts a fresh cycle. Repeatable flags remain On throughout the chain, then reset together; that reset does not trigger another cycle. Mission events still respect their own Once/Every time, chance and delay settings. Use **Every time** on an event that should react to successive calls.

For delayed actions, conditions are rechecked when the wait ends. A repeatable flag will already be Off then, so a delayed **Mission Flag Condition / Is true** condition will fail. Use **Count Condition > Mission flag calls** when the requirement must survive the brief pulse.

Starting Value supplies initial memory without adding a count or sending a notification. A non-repeatable flag starting On is already latched. All counts start at zero and reset on fresh framework startup. Flag names are case-sensitive. Unknown names read as Off/count zero, but Call Flag requires a declared Mission Flag. Invalid/duplicate definitions prevent startup.

For compatibility, **Set flag / On** calls declared flags through the same conditions/count/repeat policy. **Off** clears the state without resetting the count or rearming a used non-repeatable flag. Undeclared Set flag names retain the earlier simple On/Off behavior. Random flag actions use that same Set flag policy. Prefer Call Flag for the new workflow.

## Referencing mission flags and mission events

**Mission Flag Condition** has Flag Name and Match (**Is true** / **Is false**). It reads the flag's current result. The conditions held inside the flag control its activation; referencing it does not evaluate those conditions again or call the flag. This replaces the former Flag State name, with the same true/false behavior. Counts stay in Count Condition.

**Mission Event Condition** has Event Name and the same Match choices. It reads whether the named event ran, without evaluating that event's conditions again or executing its actions:

- **Advanced > Run > Once**: starts false and remembers true after running.
- **Advanced > Run > Every time**: becomes true for the resulting check cycle, then silently resets to false. A later run sends another notification.

Events are checked at initial setup and relevant state/flag/event notifications, not by continuously polling every condition. Event-only references do not recheck on unrelated phase changes; add Game Mode State when a phase change should also matter. An Is false reference can pass at initial setup, before the named event has run. Referenced events still run through their normal framework conditions, chance and delay. Scheduling a delayed event does not mark it true; executing it does. If actions queue flag calls, those calls are processed in the same notification cycle.

Event names must be exact and unique across regular events, random groups and their children. Missing, duplicate or invalid event names fail closed, even under Is false or a None group. An event with a disabled setting stays false until it runs (normally never). Empty conditions still mean true conditions, but do not make an event's execution result true before it runs.

A random group's result becomes true when it selects children. A selected child's result becomes true only if its own actions run; chance failure and pending delay do not count as a child run. A group with no eligible children does not publish a result.

These references nest inside Group Condition and Count Condition. Flags may also contain Mission Event Condition to activate in response to an event. A dependent event gets at most one eligible attempt per notification chain, avoiding repeatable feedback loops. Pulse resets do not send a second false notification. A delayed condition that requires a repeatable event to remain true will fail after that pulse resets; use a once-only source or a non-repeatable flag when the result must persist. Stopping clears pulses, and fresh startup clears all event results.

## Game Mode State

Choose **Mission preview**, **Role selection**, **Briefing**, **Match**, or **After action**. These are the existing authoritative Lobby2 phases; this feature does not introduce another phase system.

- **Is in this state** passes when that phase is active.
- **Is not in this state** passes when a different valid phase is active.

The framework checks the initial state after the world loads, then listens through GC_LobbyManager's existing OnGameStateChanged callback. Duplicate notifications and ordinary ticks do not trigger another check. A late startup observes the current state; it does not invent a transition. The initial state can satisfy an Is not in condition too.

State conditions can appear anywhere inside Group Condition or Count Condition. A rule containing a state, flag or event-reference condition is checked at initial setup. Subsequently, event-reference rules check on referenced event notifications, and on phase changes only when they also contain Game Mode State. Flag-dependent rules check on referenced flag changes; existing flag-only rules also retain phase-change checks. Rules with Game Mode State check distinct phase changes. All top-level conditions must pass. A rule with no state, flag or event-reference conditions keeps the simple default: check once when Match begins.

Actions waiting on a delay are rechecked when due. A state change that makes their conditions false cancels the pending run. If the conditions remain true, the original wait continues. Explicit After action conditions and their delays may run after the match; shutdown cancels everything. Conditions with no valid state context fail closed, even inside None.

## The terms

| Setting | Meaning |
| --- | --- |
| Mission Event | A name, conditions, and actions to run when the conditions pass. |
| Condition | A requirement checked on relevant changes, and again after a wait. |
| Action | Something the mission event does. |
| Flag | Optional memory: On/Off state and an accepted-call count. |

## Conditions: standard logic objects

Conditions are an **object list**. Use **+** to choose **Game Mode State**, **Mission Flag Condition**, **Mission Event Condition**, **Random Chance**, **Casualties**, **Total Deaths**, **Group Condition**, or **Count Condition**. Group and Count contain their own Conditions list, so they can be nested inside each other. A group has no number field; a count has a numeric threshold.

**Group Condition > Match** offers four choices:

| Match | Plain meaning |
| --- | --- |
| All are true (AND) | Every child must be true. |
| Any are true (OR) | One or more children must be true. |
| None are true (NOR) | Every child must be false. With one child, this reverses its answer. |
| Not all are true (NAND) | Passes unless every child is true. |

**None replaces Not.** Any group can contain other groups. XOR and XNOR have been removed.

Add one **Count Condition** object for number comparisons. Choose what to count, then set the comparison:

1. **Count From**: Conditions that are true, or Mission flag calls.
2. **Comparison**: Greater than, Less than, Equal, or Not equal.
3. **Required Count**: the number to compare against.
4. **Flag Name**: used only for Mission flag calls. Enter the exact Mission Flag name, including when checking the flag containing this condition.
5. **Conditions**: used only for Conditions that are true. Each child can contain nested groups.

| Comparison with Required Count = 2 | Passes when |
| --- | --- |
| Greater than | 3 or more listed conditions are true. |
| Less than | 0 or 1 listed conditions are true. |
| Equal | Exactly 2 listed conditions are true. |
| Not equal | Any number except 2 listed conditions is true. |

For **at least two out of three**, use **Greater than**, set Required Count to **1**, and add three conditions. Each child contributes one true-or-false answer, regardless of how many conditions it contains. For exactly two, choose Equal and set the count to 2.

For example, **Greater than 1** can contain **All [A, B]**, **None [C]**, and **D**. It passes when two or three of those branches pass. The optional script tests exercise this nested layout. Another valid layout is **Any [All [A, B], All [C, D]]**.

With Count From set to **Conditions that are true**, these comparisons count satisfied conditions, not people or entities. With **Mission flag calls**, they count accepted calls remembered by the named flag. Area Condition, Casualties and Entity Damage each provide their own true-or-false answer before the enclosing group counts them. Mission Event Condition can reference a named event's execution result. The fixed **True or false value** placeholder has been removed. Script-only test inputs live in the optional test addon and are not authored as mission conditions. Game Mode State is the first live check. Empty explicit groups still fail closed.

Every condition directly under a mission event must pass, so ordinary lists do not need an extra All group. **No conditions means always true:** an empty event Conditions list has no requirements to meet. Top-level events and random groups with no conditions run at match start; a child with no conditions is eligible whenever its group makes a selection. Chance, delay, disabled and run-once settings still apply. An explicit group needs at least one child. Empty groups, missing children and invalid required counts disable the event rather than becoming true under a negative operator. Nesting is limited to 16 levels and lists to 256 children.

Mission Flag Condition is one condition object with Is true/Is false choices. Area Condition and Entity Damage are available; broad prefab-population searches and general timer conditions remain future slices. Casualties is available with its own faction filter. Scheduled Random Chance is now available as a condition; Advanced chance remains a separate action gate.

The shared serialized base is marked non-insertable using the engine convention. Some Workbench class pickers may still expose **GC_ConditionBase**, the required shared base that lets one list hold different condition types. Its title is **Do not use**, and its description explains what to choose instead. It does not perform a check. Replace base entries with a specific condition; incomplete base entries fail closed. The class name identifies it as a base; the display-title attribute says Do not use.

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

Existing actions are Log message, Call Flag, Set flag, Send test event, Random flag, and Select random flags. Send test event remains an interpreter/testing action; this slice has no authorable named-event condition to receive it. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts the framework, after world load, and supplies state observations. It uses the existing component callback with no additional global subscription. Shutdown and loss of active mode ownership stop pending work. Explicit After action conditions can run in POSTGAME; old non-state delayed/timer work is canceled there. Mission events/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. The total event limit is 256, including random groups and their children. Each random group holds up to 255 regular events, with Pick Count from 1 to its configured child count. Initial flags, actions and condition lists are limited to 256; condition nesting is limited to 16. A bounded event queue stops runaway chains. Malformed mission events fail closed and report an error.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. The preceding GC_MissionCondition + m_iType format also changed: recreate groups using Group Condition and its Match setting. Old separate On/Off flag checks can be recreated using Mission Flag Condition and its Is true/Is false choice. Count comparisons now count satisfied child conditions. The known project example and open Arland condition were migrated. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: 560 checks passed before the final action-order correction (48 flag memory, three serialized flag defaults, 30 event-group/chance, 85 state conditions, 189 standard logic, 148 interpreter, 22 authoring, six no-flags resource, four state-example resource, five historical-fixture and 20 randomness checks). Real Lobby2 transitions were also tested through Preview -> Role selection -> Briefing -> Match -> After action: each action fired once, duplicate notifications were ignored, unauthenticated phase changes were rejected, and stopping prevented further execution. The production manager CallFlag path also passed count/pulse and disabled/shutdown/stopped rejection checks. The temporary test configuration was discarded by stopping Play. Dedicated/remote-client behavior was not newly tested. Known unrelated Arland GUID, missing menu-preset and Workbench tool-leak diagnostics remain.

Installed engine source: game/data/data007.pak/scripts/Game/GameMode/SCR_BaseGameMode.c (SHA256 c02bf603991e03e956d880ffaf643cfbde46f87a7d31f2fb1fc0918e95c16652), lines 434-465, dispatches OnGameStateChanged to components on server and clients. The manager gates mission execution to the active authoritative Lobby2 owner.


TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework supports random selection of regular events without flags.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

The editor label Mission Rules is now Mission Events. The project's saved list and enable fields were migrated to m_aMissionEvents and m_bMissionEventsEnabled; external resources using the old names need the same rename. The regular authoring class is now GC_Event; internal interpreter names retain MissionRule.

### Migrating saved conditions to this version

The editor now uses two composite types:

- **GC_ConditionGroup**: m_iMatch (0 All, 1 Any, 2 None, 3 Not all) and m_aConditions. No number field.
- **GC_ConditionCount**: m_iCountFrom (0 true conditions, 1 mission flag calls), m_iComparison (0 Greater than, 1 Less than, 2 Equal, 3 Not equal), m_iRequiredCount, m_sFlagName and m_aConditions.

Replace the former All/Any/None/NotAll classes with Group Condition and the matching mode, keeping their children. An old GC_NotCondition becomes a None group with its former m_Condition as the sole child.

Replace separate comparison classes with Count Condition. For integer counts, >= N can become > N-1 when N is positive; <= N can become < N+1 when the new threshold remains within 0-256. Boundary cases require a deliberate equivalent group; do not enter an invalid threshold. Not equal is now a direct Count Condition choice; the older None group containing an Equal count remains equivalent. Old fixed m_fFirstNumber/m_fSecondNumber comparisons need deliberate rebuilding; they do not represent live checks automatically.

The shipped no-flags example now uses a Match state condition and a fixed wait, using a once-only random group without Boolean placeholders. The user's empty Greater than object was renamed without changing its meaning. External resources need the same conversions before use. Removed XOR/XNOR classes require deliberate replacement; internal values 8 and 9 are rejected rather than reused. Internal legacy count modes remain for historical interpreter behavior, not as editor choices.

The former separate Flag On/Off classes remain removed. Recreate them with the single Mission Flag Condition condition and the matching choice.

The GC_BooleanCondition authoring class has been removed. External resources using it need deliberate removal or replacement with appropriate real checks when available; do not delete a false restriction without considering how that changes the event. No saved world in the inspected project referenced this class at removal.

### Migrating the former Event selector

Remove the obsolete GC_MissionEvent.c file when installing a source snapshot. GC_MissionRule.m_Event and the GC_MissionEvent authoring type are removed. Match started becomes a Game Mode State condition: m_eGameModeState = 1 (GAME), m_iMatch = 0 (Is in). Invert uses m_iMatch = 1. The saved empty Match-start selector in the inspected Arland scene was converted.

A once-only Timer after match start can use Match / Is in with Advanced > Delay; this is how the no-flags example is migrated. Repeating timers and named-event receivers need deliberate migration when those conditions are added; they are not silently converted to unconditional events. The old two-event flag-chain sample is deliberately replaced with independent Match and After action examples. The compiled interpreter still supports its historical triggers for regression/extension callers, but they are no longer editor options.

### Migrating regular events and randomness

Rename saved GC_MissionRule objects to **GC_Event** and remove obsolete GC_MissionRule.c when installing a source snapshot. The Mission Events list now accepts GC_Event and GC_RandomEventGroup through its shared serialized base GC_EventDefinition. Incomplete base entries fail closed; choose a concrete event type if a picker exposes the base.

Move old Random > Chance Percent into Advanced > Random Chance, dividing by 100. Remove m_Random from authored events. Convert One random action into a random group with Pick Count 1 and one regular child event per former action. Old additional random delays require deliberate redesign; they are not silently preserved. The shipped no-flags example deliberately changes its former fixed-plus-random delay to a fixed two seconds. Historical random behavior remains available only to compiled interpreter callers and regression tests.

This slice's live checks include saved example deserialization, default chance when omitted, chance boundaries, filtering, distinct picks, insufficient eligible children, delays, cancellation and repeat behavior. Full native object-class picker options and dedicated/remote-client behavior were not newly verified.

Script callers constructing authoring containers directly must set their fields explicitly (including Random Chance = 1). Attribute defaults apply when Workbench loads serialized containers. Compiled runtime rules default to full chance. Field initializers are intentionally avoided on attributed fields because they can overwrite serialized settings.

### Flag-memory migration and example

Existing declared Mission Flags default to non-repeatable. Set flag / On now respects their conditions and counts an accepted call; it no longer bypasses declared policy. If an older mission repeatedly toggles a declared flag, turn on Repeatable and account for its brief On pulse, or deliberately retain an undeclared simple memory name. Clearing a non-repeatable flag does not rearm it. No old external mission is silently assumed to need repeatable behavior.

`tests/McpTestAddon/Prefabs/FlagMemoryExample.et` is an optional editor/test fixture. It demonstrates a repeatable `wave` flag limited to five calls and a regular event reacting on every accepted call. A second `remember` flag demonstrates the default non-repeatable setting. It is not installed into the user's Arland scene.

Server extensions can call GC_LobbyManager.CallFlag(name) and read GetFlagCallCount(name). Calls require the active authoritative Lobby2 owner, an enabled framework and a running engine. This adds no client RPC. True from CallFlag means queued; a call queued by another action is checked again when processed. Counts are server-local, not a replicated general-purpose state API. The existing bounded queue still stops oversized event chains.

Final verification scope: at the user's request, the action-order correction was checked by script compilation and source-level logic review, without another in-game run. Three added ordering regressions compiled but were not executed. The earlier 560 passing checks and manager checks predate that correction.

The Advanced field formerly named Wait Seconds is now Delay, still measured in seconds. Saved GC_RuleOptions objects use m_fDelay instead of m_fWaitSeconds; the shipped example was migrated with the same numeric value. External saved Advanced settings need the same field rename. Internal compiled/legacy fixtures retain their original field name.

Count Condition now offers Not equal (saved comparison value 3), implemented as the inverse of a validated Equal check. Existing values 0/1/2 retain their meaning. Empty or malformed condition lists still fail closed. Boundary and nested-group test cases were added; this update was verified by source-level logic review and production script compilation, without an in-game test run.

### Combined Count Condition

The separate GC_FlagCallCountCondition authoring class is removed. Convert saved entries to GC_ConditionCount with m_iCountFrom = 1, keep m_sFlagName and m_iComparison, and rename m_iCount to m_iRequiredCount. Preserve the former omitted defaults explicitly: comparison 1 (Less than), required count 5. The shipped FlagMemoryExample is migrated. Existing child-condition counts keep m_iCountFrom = 0 by default. Both sources support all four comparisons. Counts of true conditions retain their 0-256 threshold limit; flag-call thresholds allow 0-1000000. Settings for the unselected source are ignored. This consolidation received script compilation and source review only, as requested.

### Named-reference migration

Rename saved GC_FlagStateCondition entries to GC_MissionFlagCondition; m_sFlagName and m_iMatch retain their values (0 true, 1 false). The project fixture and source callers are migrated. The new GC_MissionEventCondition uses m_sEventName and m_iMatch. This slice received production script compilation and source review only; no in-game tests or project restarts were performed.

Design boundary: each mission event and flag owns its conditions, repeat behavior, delay and activation rules. Outside references read published results only. They never invoke the referenced object or re-evaluate its internal conditions. Duplicate-name detection includes authored entries that fail to compile, so incomplete duplicates cannot become ambiguous valid references.

The former GC_MissionCondition shared base is renamed GC_ConditionBase, in GC_ConditionBase.c. Remove the old GC_MissionCondition.c when installing a source snapshot. Update script references and any saved base entries to the new name; the current project was migrated. Base entries still do not perform a check and must be replaced with a specific condition before use.


## Scheduled Random Chance condition

Add **Random Chance** inside an event or flag's Conditions list, including inside Group Condition and Count Condition.

- **Check Interval**: seconds between rolls; default 60, allowed 0.1 to 86400. The first roll waits one full interval after Match starts (or after the framework starts if Match is already active). No rolls run in preview, role selection, briefing or after action.
- **Chance**: a 0-to-1 float, default 1. `1` means 100%, `0.05` means 5%, `0.0005` means 0.05%, and `0` never produces true. Decimal entry is not limited to a one-percent spinner step.
- **Roll Behavior**: Keep rolling (default) makes a new roll each interval. Roll once makes exactly one roll after the first interval and holds that first result for this Match, including a failed roll. It does not keep trying until success. Event Once/Every time controls action execution independently.
- A result is held until the next scheduled roll (or until Match ends for Roll once). Reading a condition never rerolls it. Other matching flag/event changes can use that held result, so an Every time event can also run between rolls when those other conditions change.
- Before its first roll, each random condition is false. Groups apply ordinary logic: Any may pass through another true child, and None may pass while the random answer is false. There is no hidden random requirement overriding an enclosing group. Different intervals keep their own schedules and combine their latest results. Each due roll notifies its owning event/flag even if its answer has not changed, allowing repeated successes. Multiple leaves due in the same update are rolled before owners are checked, with one eligible event attempt per notification chain.
- Events still run once by default. Choose **Advanced > Run > Every time** for repeated activations. Flags retain their own Repeatable and accepted-call-count rules. No flag is required to use this condition.
- All/Any/None/Not all and Count Condition combine this answer normally. A failed roll can therefore satisfy None. Invalid chance/interval values never become passing answers through inversion.
- Delay keeps its existing meaning: latest conditions must pass when the delay ends. A later failed roll can prevent the delayed event from running. Pending events do not stack additional delayed runs. Advanced > Random Chance is a separate additional chance gate; leave it at 1 when only this condition should control probability.
- A random condition on a Random Event Group schedules group selection. A random condition inside a child only filters that child; it does not start its parent group. All random-group eligibility rules still apply.
- Leaving Match resets random answers to false and stops their rolls; pending actions are rechecked with normal group logic and existing phase-lifetime rules. Stop/fresh Start clears them too. Late updates roll once and schedule the next interval from that update, without replaying missed rolls in a burst. The existing manager tick supplies scheduling, with its authority/mode guard, and now also supports flag-only configurations.

For a 5% chance every 90 seconds, set Check Interval to `90` and Chance to `0.05`. Set the owning event to Every time if it should continue after a successful execution. This is a fresh independent chance each interval, not a guarantee that one in every twenty intervals succeeds.

This slice adds no weighted selection, cooldown, changing probability or shared random-result catalog. Existing Random Event Groups handle selection; optional flags can remember a result when the mission explicitly needs shared memory.

Verification: production Workbench script compilation and source-level logic review. Deterministic regression scenarios are provided in the optional `GC_RandomConditionTests.c`; they were not executed in-game, in accordance with the requested verification scope.


## Casualties condition

Add **Casualties** directly to an event's Conditions, inside a group/count, or to a Mission Flag's Conditions. All casualty settings stay on this one object; there is no separate faction condition or mandatory flag.

- **Factions**: an array of exact faction keys, such as US and USSR. An empty array includes every faction, including unaffiliated eligible characters. Selected factions are combined into one starting population. Duplicate keys do not double-count; blank entries are invalid.
- **Count**: Players (default), AI, or Players and AI. Spectator bodies, characters already dead at capture, and unoccupied playable slots are excluded. Connected players' living claimed slot characters count as players even when queued possession is still catching up at Match entry. Player-controlled characters outside slots are also included. AI is discovered through the native AI-world agent registry, excluding playable groups and player/slot characters.
- **Casualty Ratio**: a float from 0 to 1, default 1. `0.7` means at least 70% of the selected starting lives have died. `1` requires everyone; `0` requires no deaths but still requires a nonempty starting group. With ten starting lives, 0.7 passes on the seventh death. With three, 0.7 requires all three.

The baseline is captured once at the authoritative manager's Match-entry callback, before its mission events execute. A framework first started during Match captures the living group at that time; it cannot reconstruct an earlier baseline. The starting faction and Players/AI classification stay fixed for each life. Respawns, reinforcements, later joins, faction switches and possession changes do not expand or reclassify that baseline. Death is remembered once; a replacement life does not undo the original death. Disconnecting, unconsciousness, and deletion without a death notification do not count as casualties. An empty matching population returns false. Ordinary All/Any/None/Not all/Count logic still applies to this answer; invalid authored settings fail closed.

The existing authoritative `OnControllableDestroyed` callback records confirmed player and AI deaths, then checks casualty-dependent top-level events and flags. Player slot/death handling finishes before mission actions run. Random-group children remain filters and do not trigger their parent. No per-frame character scan, additional death listeners, client RPC, or shared global override is added. Once/Every time and flag Repeatable settings retain their existing meanings; a repeatable owner can react to later casualty notifications while its full conditions pass. The final snapshot remains readable after Match, with no new deaths recorded; shutdown clears the tracker and engine context.

Installed engine evidence: `game/data/data007.pak/scripts/Game/GameMode/SCR_BaseGameMode.c`, SHA256 `c02bf603991e03e956d880ffaf643cfbde46f87a7d31f2fb1fc0918e95c16652`, lines 1347-1366 dispatches controllable destruction to game-mode components. Existing Lobby2 native player/AI discovery and slot lifecycle paths are reused. Production source was checked by Workbench script compilation and independent source review. Optional deterministic snapshot/interpreter scenarios in `GC_CasualtyConditionTests.c` were authored but not executed; no in-game tests, per the requested scope.


## Total Deaths condition (respawn missions)

**Total Deaths** counts confirmed character deaths during Match, including deaths of respawned characters, later joins and reinforcements. It does not use a starting population or a casualty-ratio field.

- **Factions**: exact faction keys; empty means all. Uses faction at death, unlike Casualties which keeps each starting life's faction.
- **Count**: Players, AI, or Players and AI. Player victims reported by the engine, claimed player-slot characters, and tracked starting player lives count as Players. Spectators and unoccupied playable AI slots are excluded. The same faction/population matching implementation serves both condition types.
- **Comparison**: Greater than, Less than, Equal, or Not equal.
- **Required Deaths**: a whole number. Greater than 4 means five or more deaths; default Greater than 0 means the first death.

Each character life adds at most one death. Respawning creates another life whose later death counts again. A later life does not change Casualties' starting population or counted starting deaths. For example, one starting player can reach 100% Casualties after the original life dies, then accumulate five Total Deaths over five lives. Deleting a living body or disconnecting adds no death. Total Deaths starts at zero even when the initial population was empty. Equal 0 and Less than a positive threshold can therefore pass at Match start. Final totals remain readable after Match; Stop/fresh Start clears the engine context.

Two objects keep the different fields clear: **Casualties** has a ratio; **Total Deaths** has a comparison and number. Both integrate into the same nested condition lists and optional mission flags, using the same authoritative death notification and tracker. There is no separate faction condition and no irrelevant ratio field on Total Deaths.


### Spectator exclusion audit

Casualties and Total Deaths reject every entity carrying `GC_LobbySpectatorBodyComponent` before faction/population selection. This applies both to the starting-life capture and the death-recording path; empty faction filters and Players and AI do not bypass it. Lobby2's dedicated spectator prefab contains that component, and the manager uses that prefab or reuses the same marked body for spectator/reconnect assignment. Following another player moves the spectator camera/body rather than adding another combat life.

The actual soldier's death remains a casualty when its player subsequently becomes a spectator. Checking the controller's current spectator state instead of the victim body could wrongly suppress that legitimate death. Connected claimed slot characters awaiting Match-entry possession remain intended player lives, not spectator bodies. AI is counted only by the AI or Players and AI selection. Disconnect/delete notifications do not increment either death measure.

This conclusion is based on source/prefab inspection, including an independent audit; it is not a new live spectator/respawn test. Custom spectator prefabs must retain `GC_LobbySpectatorBodyComponent`; unrelated mods' unmarked spectator characters cannot be identified reliably by this marker-based contract. The built-in Lobby2 paths inspected retain it. Only tooltip/documentation clarification was needed; death-counting behavior is unchanged.

### Entity Damage

Add **Entity Damage** to any event or flag's Conditions list, including inside Group Condition or Count Condition. Name the target entity in the World Editor, then enter that exact, unique **Entity Name**. This refers to one placed entity, not every instance of a prefab and not a player's account name.

- **Destroyed**: the engine reports that the main damage area is destroyed. For a character this is its main health area, not unconsciousness.
- **Not destroyed**: that damage area is still intact enough not to be destroyed. It can be damaged.
- **Damage amount**: damage has reached **Damage Needed**, from 0 to 1. For example, 0.5 means at least 50% damage. Repairs below the threshold make the condition false again. Zero includes an undamaged supported target; one requires all health to be gone. Use Destroyed for the engine's destruction state rather than assuming every prefab maps destruction to the same health threshold.
- **Advanced / Damage Area**: optional exact hit-zone name. Leave blank for the default area. A named area belongs to the target's own damage manager; child objects such as separately attached wheels need their own entity name. No child hierarchy search is performed.

The native inspector displays Damage Needed alongside Check; its description explicitly limits it to Damage amount. It is ignored for Destroyed and Not destroyed. No additional flag, prefab component, player/vehicle/building selector, health modification, or global class override is required. The underlying entity must already have an `SCR_DamageManagerComponent` and an `SCR_HitZone` for the selected area. Decorative or indestructible scenery without that system is unsupported, rather than presumed destroyed.

Example: name a truck `SupplyTruck`, add Entity Damage with that name and Destroyed, and add the desired actions. For a combined objective, put that condition and a Mission Flag Condition inside All. A damage notification never bypasses the other conditions. A condition on a random-group child filters that child; it does not trigger the parent group's selection. Once/Every time and Delay remain owned by the event. Every time can run on later damage or repair notifications while the complete condition tree passes; it does not mean only the first threshold crossing.

Runtime ownership: GC_LobbyManager passes its world only to the authoritative, active Lobby2 mission engine. Each compiled damage leaf owns a runtime observation using the same manager/hit-zone API for characters, vehicles and destructible buildings. Initial resolution occurs at framework startup; unresolved targets are retried at Match entry. Targets must exist by that point; later dynamically spawned replacements are not automatically followed. Once resolved, the observation tracks that specific entity for the mission. Names must be unique (the native name lookup chooses the named entity; this is not a duplicate-name search).

The observation reads initial health/state and subscribes to the native health and damage-state invokers. Callbacks update the observation and mark it changed; the existing 100 ms framework tick coalesces those callbacks into one damage notification during Match. This avoids running mission actions inside an engine damage callback. Full trees use the latest result, including for delay expiry. There is no repeated health polling or world scan. The tick checks weak-reference availability so deleting an intact target does not masquerade as destruction. A confirmed destroyed result survives subsequent wreck cleanup; if the object is repaired before removal, the updated result applies. Missing/unsupported/lost intact targets fail the whole containing condition tree before inversion, so None cannot turn an unresolved reference into success. A warning identifies an unresolved target/area. Stop removes both subscriptions and clears all observations; no client listeners or new RPCs are added.

Research: TilW's `scripts/Game/Components/TILW_EntityDamageFlag.c` uses a damage manager, optional named hit zone and damage-state subscription to write a flag, with continuous/one-shot listening and optional initial-health mutation. GC reuses the engine pattern rather than that flag component: observation stays in the condition runtime, memory stays in existing events/flags, and this condition never changes health. Installed engine evidence (catalogue revision `95a1918a7fed40bf884083293ae050d6`): `scripts/Game/HitZone/SCR_HitZone.c` lines 18–64 supplies both invokers; `Prefabs/Characters/Core/DamageManager_Character_Base.ct` lines 1–13 selects the default Health area; character manager ancestry passes through `ExtendedDamageManagerComponent: SCR_DamageManagerComponent`; `SCR_VehicleDamageManagerComponent` and `SCR_DestructibleBuildingComponent` inherit the same manager. The CowShed long base prefab declares an `SCR_HitZone Default` on its building component. These are source/declaration/resource findings, not live damage tests.

Validation: production scripts compiled successfully through Workbench Reload, with zero SCRIPT errors. Logic and lifecycle are source-reviewed; in-game damage, repair, deletion and multiplayer tests were deliberately not run at the user's request. Existing unrelated Arland resource/menu diagnostics remain outside this change.

### Area Condition: radius or drawn polygon

Use **Area Condition** for named objects or living Players / AI inside/outside a shape. Use **Living Characters** for a living-population count anywhere in the mission. Area Condition has **Targets**, **Area**, **Inside / Outside**, and optional **Requirement** (Count or living-character Proportion). Both are ordinary framework conditions, with shared population sampling and no condition logic on the shape entity. Area is the exact, unique world name of the area entity. Geometry belongs to the referenced area; there is no shape selector or unused radius field in the condition. Flags remain optional, and actions/repetition/delay remain on the mission event. The future entity-picker UI can replace manual name selection without moving these responsibilities.

Place either prefab from `Prefabs/Logic/Mission`:

- **RadiusArea.et**: name it and set Radius in metres (default 10). It defines a 3D sphere including height. A point exactly at the radius is inside. An editor-only wire sphere previews the range. Scaling the entity does not change the configured metre radius. A radius area can be parented to a moving entity to follow it.
- **PolygonArea.et**: name it and draw a closed outline with 3–128 points using the normal Workbench Vector tool. The existing Map / AO Polygon tool also accepts this area and can generate a regular polygon before you edit it. Its prefab deliberately has no inherited points. Like the AO check, the polygon uses the horizontal XZ outline and ignores height; its point heights are not floor/ceiling limits. Use a simple outline without intersecting edges. Generic closed `PolylineShapeEntity` objects are also accepted. An existing AO polygon contributes its authored shape only, not its restrictions or inversion settings.

Example: place a Radius Area named `PickupZone`, set Radius to 10, and name the truck `SupplyTruck`. Add Area Condition: Targets = Named Entities, Names = [`SupplyTruck`], Area = `PickupZone`, Check = Inside. To use a drawn compound instead, point Area at a named Polygon Area; the rest of the condition and event are unchanged. Named Entities accepts a list. For living characters in an area, choose Targets = Players / AI with optional faction filtering. Count compares matching targets; Proportion compares eligible living characters on one side with all eligible living characters. Automatic arbitrary prefab discovery is not included. It tests the entity's origin, not overlap of its bounding box, collision mesh or vehicle hull. Dead named entities are still positions; Players / AI excludes dead characters. Combine Entity Damage when a named object being intact matters.

**Scheduling and notifications.** Startup and phase changes take a snapshot; during Match the existing server-local interpreter clock schedules one area sample per second. This adds no runtime frame/fixed-frame callback, per-target timer or per-target call queue. The interpreter's existing 100 ms tick only checks the next sample deadline; it does not perform area geometry checks every tick. Late updates take one sample and schedule the next one a second later, without replaying missed samples. The editor-only radius preview does redraw while editing; it does not run gameplay checks. Movement is observed periodically, and result changes produce targeted framework notifications. Initial Match state is also notified. Stable answers produce no new area notifications. Every time therefore reacts to later matching changes, rather than firing every second while a target stays inside. Another condition's notification may still activate an event while the cached area condition passes. Full nested conditions always apply. Child conditions filter random-group children without triggering the parent selection. Delayed actions recheck the full tree using its latest sampled positions. No new RPC, client-side tracking or mandatory flag is added.

**Caching and cost.** Each unique name has one weak entity reference and one position read per sample. Names are resolved only while their reference is missing; missing/deleted targets can be found again on a later scheduled sample, including named replacement entities. Each area shares its geometry cache across targets, and identical target-selection/area combinations share inside and total counts, independently of comparison or side. Each entity/area membership calculation is also cached within the sample across target selections. Radius uses `vector.DistanceSq` against radius squared. Polygons cache authored points and their world-space bounding box; points are transformed again only when the shape's world transform changes or a replacement shape is resolved. The bounding box rejects distant targets before `Math2D.IsPointInPolygonXZ`, the engine function already used by GC's AO. There is no raycast, physics overlap, `QueryEntitiesBySphere`, broad world scan or unconditional all-characters-times-all-triggers traversal. Population selections examine the native character snapshot, sharing geometry membership results across filters. Work is capped at 256 target-selection/area combinations and 128 points per polygon. Excess combinations fail closed and produce a diagnostic. This is a code-level cost bound, not a measured runtime performance claim.

**Failure and lifetime rules.** Missing named target/area, unsupported area type, open/degenerate/oversized polygon, or invalid radius make the containing condition tree unavailable before inversion: Outside/None cannot turn missing data into a successful event. Diagnostics report missing names and invalid shapes once, avoiding repeated log spam. Cached references are weak; deleting a target never means it successfully left the area. The next sample can resolve a replacement with the same name. Names must be unique; native name lookup is not a duplicate-name search. Stop drops the tracker and all caches; only the current authoritative Lobby2 owner advances it. There are no global class overrides or registration hooks to clean up. Runtime editing of polygon vertices/closed state, including separate AO `SetPolygon` overrides, is outside this slice; save geometry and reload/restart to capture it. Moving or rotating the authored shape is sampled normally.

One-second sampling deliberately trades fine movement detail for lower work. Entry/exit can be detected roughly one interval later (longer under a stalled server), and a complete crossing between samples can be missed. This is intended for mission presence checks, not projectiles or precise crossing detection. Boundary behavior for polygons follows the engine polygon function; no competing geometry algorithm is introduced.

**TilW comparison and evidence.** The reviewed TilW `TILW_TriggerSystem` uses a server-owned character list, defaults to 50 characters per processing frame and 1000 ms idle time after completed counting rounds, then evaluates triggers. Each registered character is offered to every trigger. Its Presence Trigger separately resolves named entities on each query and directly checks radius; its faction/extraction paths use the shared radius/polygon helper. The base trigger publishes result changes, optionally sets a flag, and also owns capture/status/repetition settings. GC keeps the low-rate sampling and result-change approach, but checks configured target selections, caches lookups/geometry, shares identical checks and leaves flag/action/repeat behavior in the existing framework. Source: https://github.com/Til-Weimann/tilw-mission-framework/blob/main/scripts/Game/Systems/TILW_TriggerSystem.c and `scripts/game/Entities/Triggers` at reviewed checkout `4a5300f94fb9323ce4942ef296bcffcdda922626`. Installed API evidence: `game/core/data.pak/scripts/Core/generated/Types/vector.c` declares `DistanceSq`; `IEntity.c` declares world transforms and local-to-world coordinate conversion; `ShapeEntity.c` declares `IsClosed`. GC_AOLimitEntity already uses `GetPointsPositions`, `CoordToParent` and `Math2D.IsPointInPolygonXZ`.

Validation: Workbench Reload compiled the production scripts and editor polygon-tool changes with zero SCRIPT errors. Logic, cache lifetime, scheduling, nesting and performance structure received source review. No in-game movement, preview appearance, multiplayer or benchmark test was run, per the user's requested verification scope. Existing unrelated Arland resource/menu errors remain.


### Area target selections prototype

Choose **Targets > Named Entities** for exact world names, or **Targets > Players / AI** for eligible living characters. Both options are available directly on Area Condition. Named Entities accepts 1-256 names; duplicates count once and every listed name must resolve. Players / AI has Who = Players, AI, or Players and AI, plus a Factions array. Empty Factions means all factions; selected factions are combined and duplicate keys are ignored. Uses the character's current faction. Faction settings do not appear on Named Entities.

Players means living characters currently controlled by connected players. Spectator bodies are rejected by `GC_LobbySpectatorBodyComponent` before counting. Dead characters are excluded; unconscious but living characters still count. AI comes from `AIWorld.GetAIAgents`, excludes player-controlled characters, all Lobby2 playable-slot bodies and agents in playable groups, and is deduplicated by entity. A character waiting for initial possession is not counted as a player until possession completes. Disconnected/unoccupied playable bodies are not reclassified as mission AI. This uses current membership, not the casualty condition's frozen starting-life cohort.

The optional **Requirement > Count** compares targets on the selected side using Greater than / Less than / Equal / Not equal. Unset means Greater than 0 (at least one). Examples: Players + US + Inside + Greater than 4 requires five living US players inside; AI + USSR + Inside + Equal 0 requires no eligible USSR AI inside. Outside counts targets outside; it does not mean every target is outside. A valid empty population has count zero, so Equal 0 can pass. Missing named targets or unavailable population sources fail the whole containing condition tree, including under None. Count Condition still counts true child conditions; Area Count counts matching targets.

One native population snapshot is acquired per scheduled sample only when needed. PlayerManager owns player discovery; AIWorld.GetAIAgents is consulted only for AI/Both selections. New controlled characters, registered AI, respawns and faction changes are reflected on the next sample. There is no character-registration override, spawn modification, spatial query or fallback discovery. The snapshot records faction and position once per included character. Above 4096 registered AI agents or included characters the population check becomes unavailable, rather than reporting partial counts; a diagnostic logs once. If AI is requested but its native world is unavailable, population checks fail closed. The explicit owning manager must still be the active authoritative Lobby2 manager. Stop releases the snapshot and caches.

The shared counts update each sample, but each condition notifies the framework only when its final comparison answer or availability changes (plus the initial/phase observation). Changing from one inside target to two does not repeatedly fire an event whose Greater than 0 condition stayed true. Different count comparisons sharing one observation have independent result-change tracking.

TilW's Presence Trigger resolves named objects; its Faction Trigger filters dead characters, spectators and unplayed slots and offers a player-only switch plus faction ratios. This prototype keeps one geometry/scheduling system and uses native player/AI ownership instead of adding a global character override. Capture delays/messages and arbitrary type/prefab discovery are not implemented; extraction proportions are provided by Area Condition below.

**Type/prefab discovery is deferred at the user's request.** Checking a known entity's prefab ancestry is separate from discovering all instances. Searches of installed public sources found group-local vehicle lists but no complete public list of all spawned vehicles; the inspected Vehicle/BaseVehicle declarations do not provide one. No world scans, global prefab searches, vehicle lifecycle hooks or fallback discovery were added. Any future Lobby2-only vehicle registration callback requires separate agreement and initial-load/shutdown verification; it is outside this prototype. The delivered scope is Named Entities and Players / AI with optional faction filtering, using the shared area checks.

**Schema migration:** the first prototype's single `m_sTarget` string is replaced by `m_Targets GC_NamedAreaTargets` with `m_aNames { "old-name" }`. Keep `m_sArea` and `m_iCheck`; leave Count unset to preserve a single named target's Inside/Outside behavior. Inspected project resources contained only an empty Area Condition, so no configured saved target needed conversion. External snapshots using the old field must be converted. User-authored scene changes were not replaced or included in this source update.

Validation: production script reload/compilation and independent source review cover target construction, comparisons, deduplication, population ownership, spectator exclusion, cache lifetime and change notifications. Runtime membership, respawns, faction changes and editor-picker behavior were not tested in-game at the user's request.


### General living counts and area extraction

**Living Characters** is a general population condition. It has **Who** (Players, AI, or both), optional **Factions**, and optional **Count**. It counts eligible living characters anywhere in the mission. There is no Area or Proportion option on this condition. New checks with Count unset use Greater than 0. Count supports Greater than, Less than, Equal and Not equal.

**Area Condition** owns all area-specific settings. Choose **Targets > Players / AI**, set Who and Factions there, reference the named Radius/Polygon Area, select Inside or Outside, then choose **Requirement > Count** or **Proportion**. Leave Requirement unset to require at least one matching target (unless an older saved Count is present; see compatibility below). Named Entities supports Count only; a Proportion with Named Entities is rejected.

For extraction, use Area Condition: Targets = Players / AI, Who = Players, Factions = US, Area = ExtractionZone, Inside, Requirement = Proportion / 0.9. This passes when at least 90% of all currently living US players are inside. The denominator includes eligible living players outside too. Nine of ten passes; eight of nine fails; nine of nine passes. Deaths, respawns, disconnects and faction changes update the population on the next scheduled sample. Use Casualties to compare losses with the starting population instead.

Proportion 1 means everyone. An empty eligible population makes Proportion false, even at threshold 0. A nonempty population at threshold 0 passes regardless of location. A valid empty population has Count 0, so Count Equal 0 can pass. Outside uses the number outside and the same total. Missing/invalid areas or unavailable population data remain unavailable, including under None. Living Characters includes unconscious living characters and excludes spectators, dead characters and unoccupied playable slots, using the same existing filters as area population checks.

| Condition | Settings | Meaning |
| --- | --- | --- |
| Living Characters | Players; US; Count Less than 5 | Fewer than five living US players remain anywhere |
| Area Condition | Players / AI: Players, US; ExtractionZone / Inside; Proportion 0.9 | At least 90% of living US players are inside |
| Area Condition | Players / AI: AI, all factions; BaseArea / Inside; Count Equal 0 | No eligible living AI are inside |
| Area Condition | Named Entities: SupplyTruck; BaseArea / Inside | The named truck's origin is inside |

Neither condition has child conditions. Targets and Requirement contain settings only. Radius and Polygon entities supply reusable shapes only; events, flags and logical groups remain in the framework. Reuse the same named area across different checks. The shared population snapshot, membership cache and one-second schedule serve both general and area counts; no new discovery, polling loop, world scans, spawn hooks or prefab tracking were added. Count and Proportion for the same population/area share their observations, including changes to the denominator.

Compatibility: old Area Condition `m_Count GC_AreaCount` is retained as hidden serialized data and used when the new `m_Requirement` is unset. An explicit new Requirement overrides it. To migrate external resources, rename that field to `m_Requirement`; keep the count values. The existing local scene's old Count remains readable and its other edits were preserved.

The previous prototype's area-based Living Characters configuration must move to Area Condition: copy Who/Factions into `m_Targets GC_CharacterAreaTargets`, copy old Area Name to `m_sArea`, Side to `m_iCheck`, and keep the Count/Proportion object in `m_Requirement`. Living Characters now exposes Count only. Its original Requirement remains a hidden compatible reader: old Counts still apply until an explicit Count is chosen, and old Proportions are rejected, including when their Area was unset. The hidden legacy Area reader also rejects old area-based checks with a migration diagnostic. Neither can silently broaden to count everyone everywhere. The reload saved one previously unsaved Living Characters proportion entry from the local scene. It was migrated to Area Condition, preserving Players and AI and the proportion; its previously unset area remains unset. Other scene edits were preserved and are not included in the source commit. Remove the legacy Area after migrating; do not merely change its name to blank.

Verification: production script compilation/reload and independent source review. Optional GC_LivingCharactersTests.c covers shared filters/observations, proportion boundaries, invalid combinations, missing context, and legacy-count precedence. Optional tests and in-game movement/respawn/multiplayer checks were not executed at the user's request. Existing unrelated Arland resource/menu diagnostics remain.


### Normalized value sliders

All visible Lobby2 editor float settings whose domain is 0..1 use UIWidgets.Slider: event Random Chance, scheduled Random Chance, Casualty Ratio, Damage Amount, and area Required Proportion. Chance sliders use step 0.0001 (0.01 percentage points), preserving selections such as 0.0005 = 0.05%; other ratio sliders use step 0.01. Defaults, serialized field names and runtime validation are unchanged. This does not reinterpret existing saved values or convert legacy 0..100 percentage fields. General configurable float parameters, time and distance values retain their own ranges.


### Condition naming

Every selectable condition uses the visible name **[Name] Condition**: Area Condition, Casualties Condition, Count Condition, Entity Damage Condition, Game Mode State Condition, Group Condition, Interaction Condition, Living Characters Condition, Mission Event Condition, Mission Flag Condition, Random Chance Condition, and Total Deaths Condition. The nonselectable base remains **Do not use**. Serialized class names and field names are unchanged, so saved missions require no migration for this naming update. Targets and Requirement settings are not conditions and retain their short names.


### Mission interactions

Place **Prefabs/Logic/Mission/Interaction.et**. This is an empty, invisible-in-game entity with GC_MissionInteractionComponent, RplComponent, and a native ActionsManager context/action. The supplied context has Radius 2 and the action Duration 1 second. It uses the game's existing interaction menu and progress behavior, not a custom menu. Place it where players should interact; add visible scenery separately if wanted.

The component provides:

- **Action Name:** the player-facing text, such as Activate radio.
- **Factions:** optional allowed faction keys; empty allows every faction.
- **Repeatable:** off allows one accepted use on this object; on allows later uses.
- **Required Flag:** optional existing Mission Flag that must currently be On. Put complex requirements on that flag in GC Lobby Manager. This is a gate, not a second condition list on the object. Use a persistent flag for a lasting availability gate; repeatable flag pulses are transient framework-cycle values.
- **Call Flag:** optional existing Mission Flag to call after use. Its normal conditions, count limit and repeatability apply. Rejection does not undo an otherwise accepted interaction.

Interactions work only during Match with an active authoritative Lobby2 mission framework. Living, conscious, connected player-controlled characters can use them; spectators, AI and dead/unconscious characters cannot. Availability and use count are replicated by the component. Keep RplComponent on the same entity. The server verifies eligibility again at completion, and performs the mutation once; clients do not execute the effect.

To put this on an existing object, add **GC_MissionInteractionComponent** to that entity, keep/add its **RplComponent**, and add **GC_MissionInteractionAction** under its existing **ActionsManagerComponent > additionalActions**. Set the action's **ParentContextList** to an existing action context on that same entity, or create a native UserActionContext. Native context Position/Radius and action Duration own positioning/reach and hold time. Reuse existing components rather than adding a second ActionsManager. All GC interaction action slots on one entity share its component, label, restrictions and use count; use separate child interaction entities for independent buttons. No changes to unrelated prefabs are required.

To observe use without flags, give the object a unique world name such as RadioSwitch. Add **Interaction Condition** to a mission event and select Has been used (or Has not been used), naming RadioSwitch. The condition reads remembered use, not the interaction's gate or conditions. After the first use, Has been used stays true; each accepted repeat use sends another notification to observing events. Event Advanced > Run still controls whether the event runs once or every time. Other conditions in its group must pass normally. Conditions can also live in an existing Mission Flag definition if the result needs to be combined there, but flags are optional.

Place referenced interaction objects before Match starts. Names resolve at framework startup and retry at Match entry, then the resolved component is cached; there is no world scan or continuous name search. A missing component or deleted object is unavailable, including under None or Has not been used. Use an output flag if memory must survive deletion of the object. Duplicate world names are unsupported; the engine's named lookup is not a uniqueness scan.

Use notifications are processed together with the optional output flag call, preserving pulse-flag behavior and existing loop guards. The same interpreter evaluates all conditions; the interaction object contains no nested framework or condition list. State changes update availability through an event subscription. The existing framework tick observes deletion and drains pending changes; no extra timer or frame callback is added. Stop removes condition listeners, and component deletion removes availability subscriptions. No global modded-class override is used.

Reference research: TilW checkout 4a5300f94fb9323ce4942ef296bcffcdda922626, scripts/Game/UserActions/TILW_BaseInteraction.c and Prefabs/Utils/Interactions/TILW_SetFlagInteraction.et, uses a native scripted action, optional faction/flag filters and single-use state. This implementation reuses the native action/context pattern with original Lobby2 code, authoritative component state, and the existing GC flag semantics; destructive parent deletion and teleport are outside this slice.

Installed engine evidence (catalogue revision 95a1918a7fed40bf884083293ae050d6): game/data/data007.pak/scripts/Game/generated/UserAction/ScriptedUserAction.c documents server-first verification and server-only/nonbroadcast effects (lines 40-53); BaseUserAction.c documents native context access and action-manager ownership. SCR_InteractionHandlerComponent.c lines 347-357 uses controlled-entity origin, context visibility range and squared-distance tolerance 1.1 for a range bound. The extra server bound uses that same calculation and only this action's registered contexts; native action handling still owns exact context eligibility/visibility. Component/player identity checks do not rely on UI visibility alone.

Verification: script compilation/reload and independent source review. Optional GC_InteractionTests.c covers authored input validation, unavailable conditions under None, repeated interaction notifications, stop behavior, and interaction plus pulse-flag dispatch. Those tests were compiled but not run. Native menu appearance, interaction input, resource instantiation, client/server transport and replication/JIP have not been tested in Play, per the requested no-in-game-test scope. Treat those as prototype verification limits, not established runtime results. Existing unrelated Arland resource/menu errors remain.

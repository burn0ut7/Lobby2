# Lobby2 mission rules

Select your game mode, then **GC_LobbyManager > Lobby2**. The top settings are **Mission Rules**, **Mission Flags**, **Briefing Catalog**, and **Parameter Catalog**. Mission Rules Enabled and Parameter Lock Seconds remain in Unsorted.

## Start with one rule

1. Add a **Mission Rule** and give it a name.
2. Add its **Event**, then choose **Type** from the dropdown: Match started, Timer elapsed, Named event received, Flag turned On, or Flag turned Off. An empty Event means Match started.
3. Add its **Actions**. They normally run in order.
4. Leave **Conditions**, **Advanced**, and **Random** empty unless needed.

A rule runs once by default. **Advanced > Run > Every time** allows later matching events to run it again. **Wait Seconds** adds a fixed delay. Disabled keeps the rule saved without running it.

**Flags are optional.** Match-start, timer and named-event rules work without flags, flag definitions or conditions. The no-flags example below demonstrates this.

## The terms

| Setting | Meaning |
| --- | --- |
| Mission Rule | An event, optional conditions, and actions to run. |
| Event | Something that happens now and starts a rule. It is not remembered. |
| Condition | A requirement checked when the event happens, and again after any wait. |
| Action | Something the rule does. |
| Flag | A named On/Off value the mission remembers. Use it only when useful. |

**Mission Flags** optionally gives names starting values. Initial values are applied before Match started rules and do not fire Flag changed events. Undeclared flags start Off and can still be set by actions. Names are exact and case-sensitive. Setting a flag to its current value does not send another event. Duplicate or invalid initial flag definitions prevent rules from starting and report an error.

## Conditions

Add a **Condition**, then choose its **Type**. A normal flag condition only needs Flag Name and Flag is On/Off. For combinations, use All conditions, Any condition, or No conditions, and add the inner Conditions. All conditions agree, Exactly, At least and At most cover advanced logic. Number is used only for the three count choices.

Every condition directly under a rule must pass. Empty rule Conditions means no restrictions. Empty All and No combinations pass; empty Any fails. Nesting remains available, but ordinary rules need none. There is no Check Group or Test Condition choice.

## Optional randomness

Add **Random** only to rules that need it:

| Mission-maker intent | Setting |
| --- | --- |
| Sometimes run this event | Chance Percent, for example 25. |
| Wait an unpredictable time after an event | Minimum Seconds and Maximum Seconds, for example 10–30. |
| Pick one outcome without flags | Action Selection > One random action. |
| Sometimes turn a flag On | Random flag action with Chance Percent. Failure sets it Off. |
| Choose a route or several objectives | Select random flags: a list and Selection Count. Selected flags turn On; others in that list turn Off. |

Chance is checked once per matching event after conditions pass. Zero never runs; 100 always runs. A failed chance waits for a new event. A delay is sampled once, added to the fixed wait, and conditions are rechecked when it ends. Events received while waiting are ignored. Repeat timers skip missed ticks instead of executing a burst.

Random action selection chooses one whole action uniformly when execution begins. Random flag selection chooses distinct entries without modifying the configured list. No weighted selection or full gameplay action catalog is included yet.

Use Match started plus 50% chance for an uncertain opening event; Named event received plus a random delay for a response to an earlier event; or a repeating Timer with a chance for occasional activity. Use flags only when later rules must remember or react to a selected outcome.

## Included examples

- `Prefabs/MP/Modes/GameMode_Lobby_NoFlagsExample.et`: no flags or conditions. After a two-second timer and a random extra 1–3 seconds, one of two test log messages runs once.
- `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`: two rules connected by a flag. First sets test_ready; second observes that change and sets test_complete.

Existing actions are Log message, Set flag, Send test event, Random flag, and Select random flags. Named events can be sent by the sample action or the server-side manager SendTestEvent method. No RPC or arbitrary script execution is exposed.

## Runtime and limits

Only the authoritative active Lobby2 game mode starts rules, after briefing at GAME. Shutdown and POSTGAME cancel pending work. Rules/flags do not replicate as a general mission-state API. Flags reset on a fresh engine start; stopped values remain available for diagnostics. Limits are 256 rules, initial flags, actions, or children per group; nesting is limited to 16. A bounded event queue stops runaway chains. Malformed rules fail closed and report their index.

## Updating earlier prototypes

This cleanup changes the saved authoring schema. It is **not** a drop-in deserializer for unchanged older resources. The supplied project example and the user's open example layer were migrated. External resources using GC_SimpleMissionRule, GC_RuleDefinition, old GC_MissionRule fields, specialized event classes, or old condition classes need conversion/recreation before use. Keep a backup. Do not combine removed prototype scripts with this version.

The old interpreter behavior remains covered by regression tests. A historical fixture is loaded through an isolated test-only adapter; this proves its behavior, not unchanged production resource compatibility. The original MissionFramework_v0_1.zip was unavailable, so exact archive compatibility is not claimed.

## Verification and provenance

Workbench 1.9.0.37: 148 interpreter checks, 26 authoring checks, four serialized flag-example checks, five historical-fixture checks 20 random/optional-flag checks, and five serialized no-flags example checks passed in embedded Play (208 total). The Event dropdown was opened and its five entries verified through the native editor. Known unrelated Arland world GUID and missing menu-preset diagnostics remain in this test project.

TilW was studied as inspiration, not copied. Reference commit: 4a5300f94fb9323ce4942ef296bcffcdda922626. Its random-flag and flag-sampling initialization/actions informed the use cases. This framework also supports direct random action selection without flags and event-based random delays.

- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_Instructions.c
- https://github.com/Til-Weimann/tilw-mission-framework/blob/4a5300f94fb9323ce4942ef296bcffcdda922626/scripts/game/TILW_MissionFrameworkEntity.c

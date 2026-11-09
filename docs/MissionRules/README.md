# Mission rules

Open your GameMode's **GC_LobbyManager > Lobby2 > Mission Rules**.
Mission Rules comes first, followed by Briefing Catalog and Parameter Catalog.
Mission Rules Enabled and Parameter Lock Seconds stay in Unsorted.

## Five clear terms

| Term | Meaning |
| --- | --- |
| Mission rule | One event, optional conditions, and a list of actions. |
| Event | What starts the rule: for example, the match starting. |
| Condition | A requirement that must be met before the actions run. |
| Action | Something the rule performs. |
| Flag | A named On/Off value that rules can remember and check. It is not a physical flag. |

## Create your first rule

1. Add a mission rule and give it a name.
2. In **Event**, select **Match started**.
3. In **Actions**, add **Log message** and enter your message.

That is enough. Leave **Conditions** and **Advanced** empty.
Start the match after briefing; the message appears once in the Workbench log.
An empty Event also means Match started.

Each choice shows only its own settings. A Log message action has one Message
field. A Flag condition has Flag Name and Must Be (On or Off).
You do not need to create a group for a single condition.

## Events, conditions, and actions

**Events:** Match started; Flag changed. Advanced choices include Timer elapsed
and Test event received. A timer runs once by default. To repeat it, set
Advanced > Run to Every time.

**Conditions:** optional. Every condition directly in this list must be met.
A Flag condition checks whether a named flag is On or Off. A flag starts Off.
Changing a condition's flag does not itself retry a rule: select Flag changed
as the Event when you want that change to start the rule.

**Actions:** Log message; Set flag; Send test event (advanced). Actions run in
list order. These are development examples, not gameplay objectives or player
messages. No spawning, victory, or other gameplay catalog has been added.

Use exactly the same flag or event name wherever you refer to it. Names are
case-sensitive. Flags are remembered until the world restarts; test events are
one-time notifications and are not remembered. They are separate even if they
have the same name.

## Example

The example prefab is `Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et`.
With the optional test addon loaded, open `worlds/MissionRulesExample.ent`.

- **Start the test:** Event = Match started; Action = Set flag `test_ready` On.
- **Finish the test:** Event = Flag changed, `test_ready` becomes On;
  Actions = Set flag `test_complete` On, then Log message.

Both run once, with no conditions or advanced options needed. Around match
start, the log prints: `Test complete. The flag started this rule.`

## Advanced, only when needed

The optional Advanced object contains Run (Once / Every time), Wait Seconds,
and Disabled. Without it, a rule runs once with no delay.

Advanced condition combinations support All conditions, Any condition,
No conditions, Exactly this many, At least this many, At most this many,
and All conditions agree. Combinations can contain other combinations.
The numeric choices show a Number field; the others do not. Fixed test
conditions provide Yes/No answers for development checks.

An empty All, None, or All agree combination passes; empty Any fails.
Numeric combinations compare zero against Number when empty.

A delayed rule checks its conditions again before actions run. Events received
while it is waiting are ignored. Failed conditions do not consume its one run.
Ending the match cancels delayed and repeating work. Timers skip missed ticks
rather than replaying a burst after a stall. Timing is checked every 100 ms.

Flag changed means an actual change: setting an already-On flag On does not
trigger it again, and an initially-Off flag does not emit an Off event.
Changes produced by actions are queued after the current event's rules finish.
A chain of more than 256 events stops the framework to prevent endless loops.

## Existing missions and compatibility

The normal Lobby2 GameMode has no rules, so no test actions run automatically.
The existing TilW dependency and integration remain intact. These flags are
local to Lobby2's server-side rule engine; they are not TilW flags or replicated
player UI. Only the active Lobby2 manager on the server can send test events,
and only during GAME. Public calls remain SendTestEvent, GetFlag and GetRunCount.

Previously saved `GC_MissionRule` objects remain supported with their original
fields and behavior. They retain their old editing layout; new rules use the
simpler `GC_SimpleMissionRule` layout. The old type is not offered for new rules.
Both formats use the same interpreter. Existing rules are not silently rewritten.

For the earlier separate-component version, move its m_aRules list into the
manager's m_aMissionRules and copy m_bEnabled to m_bMissionRulesEnabled, including
false, before removing the old component/script.

The original v0.1 archive was unavailable. Exact SMF source/API equivalence is
not claimed. TilW was inspected for concepts only; no implementation was copied.
Reference: https://github.com/Til-Weimann/tilw-mission-framework/tree/4a5300f94fb9323ce4942ef296bcffcdda922626

## Developer verification

The optional test addon runs only on explicit calls. On GC_LobbyManager:
GC_TestMissionRules tests the original interpreter; GC_TestSimpleMissionRules
tests simple definitions and flag events; GC_TestMissionRulesExample loads the
new serialized example; GC_TestLegacyMissionRulesExample loads an unchanged
old-format fixture. Intentional loop/oversized-list cases log expected errors.

Limits: 256 rules, 256 actions per rule, 256 children per group, 16 condition
edges, and at most one day for a delay or interval. Invalid definitions fail
closed before any action runs. New editor definitions are converted once at
match start; runtime state is kept separately.

Workbench 1.9.0.37, 2026-11-09: Reload succeeded. All 183 checks passed:
148 original interpreter, 26 simple definitions/flag events, four new prefab,
and five legacy prefab. The example also completed through the actual Lobby2
phase handlers; test_complete was true in GAME. Receipts:
rf1cd8c1762f4448f921e89dc93cc0dd4, r1b46b81df2724b28b6e6b01d5a73a5ca,
r932ab7f42a7a49639a48e7bc27237cda, r4780d16eb195453a8a1e41b6d83a8434,
r03f5125d4092489d8beee33c77733aa5. Existing fixture/resource diagnostics remain.
Remote-client/dedicated-server coverage remains untested.

The live inspector was independently reviewed: Event, Actions, Conditions and Advanced are visible, Flag changed shows only Flag Name and Becomes, and Log message shows only Message. The scene was left open with Play stopped. Interactive creation of every advanced condition and tooltip hover were not exercised.

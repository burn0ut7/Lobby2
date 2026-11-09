# Simple mission rules

Lobby2 has a **Lobby2** section on its GameMode's
`GC_LobbyManager`. Build a rule by answering three questions:

1. **When?** The match begins, a timer ticks, or a named test event arrives.
2. **If?** Check a flag, use a fixed test answer, or combine checks in a group.
3. **Then?** Write a test message, set a yes/no flag, or send another test event.

The base Lobby2 GameMode has an empty rule list on its lobby manager. Existing missions do not
gain automatic test actions. The TilW dependency and adapters remain intact.
These new rules have their own server-local flags; they do not read or write
TilW flags.

## Set up a rule in Workbench

Select your Lobby2 GameMode, select **GC_LobbyManager**, then expand
**Lobby2 > Mission Rules**. Add a `GC_MissionRule`, give it a useful **Name**,
and fill in **When**, **If**, and **Then**. Add `GC_RuleAction` entries under Then.
The editor derives field labels from member names. Descriptions explain each
setting through the property inspector's normal help/tooltip support.

Leave **If** empty to allow the rule whenever its event occurs. To require a
condition, create a `GC_RuleCondition`. Choose **Group of checks**, **Flag is yes**,
or **Test answer**. For a group, add more conditions under **Checks**. A child
condition can be another group. Only fields for the chosen check type apply.

**Reverse answer** changes yes to no and no to yes. A missing named flag is no;
reversing that check tests whether the flag is no. Names are case-sensitive.

| Group needs | Meaning |
| --- | --- |
| All pass | Every child check must pass. |
| Any pass | One or more child checks must pass. |
| None pass | Every child check must fail. |
| Exactly this many pass | Exactly **How many** child checks must pass. |
| At least this many pass | **How many** or more child checks must pass. |
| At most this many pass | No more than **How many** child checks may pass. |
| All give the same answer | All child checks pass, or all child checks fail. |

A nested group counts as one child answer. With no children, All, None and All
the same pass; Any fails. Number-based choices compare zero against How many.
Exactly zero and At least zero therefore pass for an empty group.

## Try the example

`Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et` inherits the normal Lobby2
GameMode. Use it as the **single** GameMode in a disposable Lobby2 test mission,
or copy its three rules into your existing GameMode component. With the optional
MCP test addon loaded, open `worlds/MissionRulesExample.ent` for a ready-made
example: it copies the existing Empty_Lobby fixture and changes only its GameMode
prefab to this example.

- Match start sets `test_ready` to yes.
- A repeatable timer sends `test_pulse` every two seconds.
- A rule checks `test_ready` and a nested Any group, waits one second, then sets
  `test_complete` to yes and prints one success message to the Workbench log.

Advance through the lobby and briefing normally. Around three seconds after
the active match begins, the log should contain:

```text
GC | Mission Framework | 3. Pass the nested test once: Nested test passed. This line should appear only once per match.
```

The timer keeps sending events, but **Run only once** prevents another success.
The example performs no gameplay operations and is not placed in existing worlds.

## Timing and ordering

- Match begins means Lobby2's native **GAME** phase, not loading the world or
  opening the lobby. Nothing executes in preview, slotting or briefing.
- Every few seconds first checks after its interval has elapsed. A late frame
  performs one check; it does not replay all missed intervals.
- Conditions are checked only for a matching event. Changing a flag does not
  automatically emit an event. Use a timer or send a named test event afterward.
- **Wait seconds** delays the whole action list. Conditions are checked again
  when the wait ends. If they fail, the rule may try again on a later event.
  Events arriving while that rule is waiting are ignored, rather than stacked.
- Run only once is consumed when actions begin, not when a wait is scheduled.
  Repeatable rules may run on each subsequent matching event.
- Rules and actions use their authored list order. Changes to flags are visible
  to later rules. Events sent by actions are processed after the current event's
  rules finish, preventing recursive calls.
- Time is checked every 100 ms using world time. Delays are approximate
  game-time waits, not precise wall-clock scheduling.
- Ending the match or closing the world cancels outstanding work. A new world
  starts with no flags and no completed rules.

## Limits and extension points

This is the framework and a small test vocabulary, not a gameplay action catalog.
There are no new player RPCs, menus, gameplay objectives, spawning actions or
replicated flag UI. Only the active Lobby2 GameMode's server component may send
events. Trusted server code can call `SendTestEvent(name)` on that component;
clients and calls outside GAME are rejected. `GetFlag(name)` and `GetRunCount()`
provide local diagnostics. Clients have no authoritative flag copy.

`GC_MissionRule` stores editor settings; `GC_RuleCondition` evaluates condition
trees; `GC_MissionRuleEngine` owns per-match state and scheduling;
`GC_LobbyManager` owns lifecycle and authority. Add later event sources
at the manager boundary and later action/check types in the data classes and
interpreter. Keep gameplay permissions at the server boundary.

Incomplete rules are skipped with a log message, before any action runs.
Conditions reject invalid settings even when Reverse answer is checked. There
may be at most 256 rules, 256 actions per rule and 256 children per group. Tree
depth is limited to 16 edges. Waits and intervals are capped at one day. More
than 256 chained events in one dispatch stops this framework for the match and
logs an error; this protects against rules that repeatedly trigger each other.

## Compatibility and provenance

The earlier chat described v0.1 with nested logic, named flags, disabled rules,
one-time/repeated execution and test events/actions. Those behaviors are covered
here. Its `MissionFramework_v0_1.zip` / `SMF_MissionFramework.c` attachment was
not available in this workspace or through the referenced chat, so source/API
compatibility and exact v0.1 edge-case equivalence cannot be claimed. This is
new Lobby2 `GC_` source, not a replacement retaining the unavailable `SMF_` API.

[TilW](https://github.com/Til-Weimann/tilw-mission-framework) was inspected for
concepts only: named flags, nested logical terms and event instructions. No TilW
implementation was copied into these files. Existing TilW-authored missions
continue using the integration in [MissionFramework.md](../MissionFramework.md).

Installed engine evidence for callbacks and editor attributes: catalogue revision
`95a1918a7fed40bf884083293ae050d6`,
`data007.pak/scripts/Game/GameMode/SCR_BaseGameModeComponent.c` (lines 20-38),
`SCR_BaseGameMode.c` (lines 434-465), and
`core/data.pak/scripts/Core/generated/Attributes/Attribute.c` (lines 65-91).
These are installed sources/declarations, not proof of live behavior.

## Verification

The optional test addon contains `GC_MissionRuleTests.c`. Loading it runs nothing.
During embedded Play, invoke `GC_TestMissionRules()` on `GC_LobbyManager` through
MCP. It calls the production interpreter and checks truth tables, reverse answers,
nested flags, invalid data, event ordering, repeats, delays, cancellation, timer
stalls and event-cycle protection. The intentional cycle and oversized-list tests
each emit a framework error diagnostic; these are expected, not compiler errors.
`GC_TestMissionRulesExample()` separately loads the real example prefab,
deserializes its rules through the engine, and checks its three-rule sequence
without spawning another GameMode.

For lifecycle checks, start with the normal empty rule list in PREGAME, call
`GC_TestMissionRulesPrepare()`, and inspect `GC_TestMissionRulesSnapshot()`.
Advance through real Lobby2 phase handlers. In GAME, send `live_delay` and later
verify its flag; send `live_cancel`, end the match before ten seconds, and verify
the cancel flag remains no afterward. Check that sends in PREGAME, POSTGAME and
a client are rejected.

Verified in Workbench 1.9.0.37 on 2026-11-09 through Reforger MCP:

- Script Reload completed; the interpreter passed 148 checks with zero failures.
- Engine deserialization of the saved example passed five checks with zero failures.
- The editor showed the component, Mission Framework section and three named rules.
- The authored example world reached GAME through production phase requests; its nested delayed rule set `test_complete` and logged success.
- Live lifecycle checks confirmed match-start, timer and delayed actions, PREGAME/POSTGAME event rejection, and cancellation beyond the ten-second deadline. Runs stayed at 30 and the cancelled flag stayed false.
- A fresh normal Empty_Lobby world reached GAME with zero rule runs and no test flags.

Remote-client/dedicated-server behavior and interactive tooltip hover were not
exercised. Authority guards and editor descriptions were inspected in source.
Existing fixture/resource warnings remain outside this change.

Evidence: Workbench session `logs_2026-11-08_23-36-40`, Reload cycle at line 21635.
Interpreter receipt `r5435f11c9c1549f098cfdf2e3e5276ff`; prefab
`r51acce6d7bf147fe8bb712f6146e7c1c`; cancellation
`r885d7a5c97ce437bb2feea06706f4768`; authored-world flag
`r71cdc9fc973c4454bb82176e26ed9cee`; empty-world snapshot
`r77aa4b8912054e499f4c4db81b434e13`.

## Manager placement update

All framework settings now live on `GC_LobbyManager > Lobby2`, alongside briefing and parameters. The separate rules component was removed. The example prefab now overrides the existing manager. If you authored rules with the previous revision, move its `m_aRules` list to the manager's `m_aMissionRules` field, copy `m_bEnabled` to `m_bMissionRulesEnabled` (including a disabled value), and remove the old component. Trusted server calls now target `GC_LobbyManager`.

After the manager move, Reload succeeded and all 153 interpreter/prefab checks passed again. Live manager calls confirmed startup, timer, delay, pre/post-match rejection and cancellation after ten seconds (13 runs unchanged, cancel false). Receipts: rb4292cc34e7d467c8b9985e016bd279f, r37de76793bb840f7992028e98f6f0c00, rdb896710474046f8b88c63b1abcc7372.

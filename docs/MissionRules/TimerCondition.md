# Timer Condition

Add **Timer Condition** to an event or flag's Conditions list, including inside nested condition groups.

- **Start State:** Mission preview, Role selection, Briefing, Match (default), or After action.
- **Seconds:** typed decimal duration, default 60. For example, `2.5` is two and a half seconds.
- **Random Max Seconds:** optional typed decimal value, blank/default zero. Blank or zero uses Seconds exactly. A positive value chooses one duration between the two numbers when the selected state starts. There is no Random Duration checkbox.

Both numeric controls use EditBox rather than SpinBox, and remain float fields. Workbench may display its unset float as 0; zero explicitly means no random range. Seconds must be between 0.1 and 86400. Random Max Seconds must be zero or between 0.1 and 86400. Negative, too-small positive, excessive and non-finite numeric values are rejected. Workbench owns text-to-float input handling. Reversed valid bounds are ordered automatically: Seconds 120 and Random Max Seconds 60 means 60-120 seconds. Equal bounds use a fixed duration without a random draw.

It starts once when the selected state is observed. Its answer is false until the deadline, then stays true until the framework restarts. Counting continues through later states, so a timer started in Briefing can finish during Match. If the framework starts in the selected state, timing starts then; it does not reconstruct past state-entry times. If the selected state already passed before the framework started, the timer waits for that state to be entered. State changes and condition checks never reroll the deadline.

Example: Start State Briefing, Seconds 60, Random Max Seconds 120 chooses one duration from one to two minutes after Briefing starts. Clear Random Max Seconds or set it to 0 for exactly 60 seconds. Combine the timer with Mission Flag `ready` inside All to require both. Neither bypasses the other. Use None around the timer for the opposite answer. Add a Game Mode State Condition if the event should only run in one particular phase.

The timer emits one expiry notification. It is not a repeating interval. Advanced Repeatable allows an event to run on later relevant notifications while the timer remains true; it does not restart the timer. Resetting an event also does not restart it. Use Game Mode State for an immediate state-entry condition, rather than a zero-second timer. Event Delay remains separate: it waits after the event's conditions pass.

Implementation uses timer leaf kind 12 in the existing per-engine scheduler. Fixed durations consume no random draw; a nonzero random range consumes one draw on selected-state entry. Authored containers hold no live deadline or result. The existing manager scheduler runs every 100 ms of world simulation time; expiry is observed on its next update, not on every rendered frame. Pausing simulation pauses this timing. All due leaves update before dependent notifications. Random Chance Condition retains its separate Match-only rolling behavior.

Existing local Timer Conditions contain no explicit old random settings and keep their Seconds and Start State values. For external resources using the previous schema: remove `m_bRandomDuration`; when it was true, rename `m_fMaximumSeconds` to `m_fRandomMaxSeconds` (use 120 if the old maximum was omitted). When it was false/omitted, discard the old maximum and leave the new field absent/zero. Otherwise a previously unused maximum could accidentally enable randomness. Script-created fixtures must explicitly set their start state; Workbench applies the attribute's Match default to authored resources.

Verification: source/logic review and Workbench compilation/reload. Optional `GC_TimerConditionTests.RunTimerChecks` covers selected-state entry, fixed/random deadlines, no rerolls, cross-state timing, after-action timers, persistent results, nested flag gating, restart, late updates and invalid ranges. These fixtures compile but were not executed; no in-game tests were run at the user's request.

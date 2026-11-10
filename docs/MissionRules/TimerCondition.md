# Timer Condition

Add **Timer Condition** to an event or flag's Conditions list, including inside nested condition groups.

- **Start State:** Mission preview, Role selection, Briefing, Match (default), or After action.
- **Seconds:** fixed duration; with Random Duration checked, the minimum duration. Default 60.
- **Random Duration:** unchecked uses Seconds exactly. Checked chooses a duration once between Seconds and Maximum Seconds when the selected state starts.
- **Maximum Seconds:** used only for a random duration, default 120. Must be at least Seconds. Equal limits are a fixed duration and consume no random draw.

Durations must be between 0.1 and 86400 seconds. An invalid or reversed range is rejected, not silently swapped.

It starts once when the selected state is observed. Its answer is false until the deadline, then stays true until the framework restarts. Counting continues through later states, so a timer started in Briefing can finish during Match. If the framework starts in the selected state, timing starts then; it does not reconstruct past state-entry times. If the selected state already passed before the framework started, the timer waits for that state to be entered. State changes and condition checks never reroll the deadline.

Example: Start State Briefing, Seconds 60, Random Duration checked, Maximum Seconds 120 chooses one duration from one to two minutes after Briefing starts. Combine it with Mission Flag `ready` inside All to require both the elapsed timer and the flag. Neither bypasses the other. Use None around the timer for the opposite answer. Add a Game Mode State Condition if the event should only run in one particular phase.

The timer emits one expiry notification. It is not a repeating interval. Advanced Repeatable allows an event to run on later relevant notifications while the timer remains true; it does not restart the timer. Resetting an event also does not restart it. Use Game Mode State for an immediate state-entry condition, rather than a zero-second timer. Event Delay remains separate: it waits after the event's conditions pass.

Implementation uses timer leaf kind 12 in the existing per-engine scheduler. Fixed durations consume no random draw; a nonzero random range consumes one draw on selected-state entry. Authored containers hold no live deadline or result. The existing manager scheduler runs every 100 ms of world simulation time; expiry is observed on its next update, not on every rendered frame. Pausing simulation pauses this timing. All due leaves update before dependent notifications. Random Chance Condition retains its separate Match-only rolling behavior.

Existing serialized Timer Conditions keep their Seconds value and default to Match with Random Duration unchecked. The result now survives later states instead of clearing at Match end. To require Match-only behavior, combine the timer with Game Mode State = Match. Script-created fixtures must explicitly set their start state; Workbench applies the attribute's Match default to authored resources.

Verification: source/logic review and Workbench compilation/reload. Optional `GC_TimerConditionTests.RunTimerChecks` covers selected-state entry, fixed/random deadlines, no rerolls, cross-state timing, after-action timers, persistent results, nested flag gating, restart, late updates and invalid ranges. These fixtures compile but were not executed; no in-game tests were run at the user's request.

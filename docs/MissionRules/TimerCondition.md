# Timer Condition

Add **Timer Condition** to an event or flag's Conditions list, including inside nested condition groups. Its only setting is **Seconds**, default 60, range 0.1 to 86400.

It starts counting at Match start. Its answer is false until the deadline, then true for the rest of that Match. Leaving Match clears the answer. Briefing time does not count. If the framework itself starts during a Match, its timer starts from that framework start; it does not reconstruct an earlier match start.

Example: Timer 120 plus Mission Flag `ready` inside All means both two minutes must have elapsed and `ready` must be true. If the timer expires first, the later flag change can still run the event. Neither condition bypasses the group. Use None around the timer for the opposite answer, remembering that None also sees the timer as false outside Match; add a Game Mode State condition if needed.

The timer emits one expiry notification. It is not a repeating interval. Advanced Repeatable allows an event to run on later relevant notifications while the timer remains true; it does not restart the timer. Resetting an event also does not restart it. Use Game Mode State for an immediate match-start condition, rather than a zero-second timer. Event Delay remains separate: it waits after the event's conditions pass.

Implementation reuses the existing per-engine scheduled-condition path with certainty and one deadline. The condition does not draw a random number, expose chance settings, keep state in authored containers or add its own callbacks. The existing manager scheduler runs every 100 ms of world simulation time; expiry is observed on its next update, not on every rendered frame. Pausing simulation pauses this timing. All due leaves update before their dependent event/flag notifications are processed. Existing depth limits still apply to nested groups.

Verification: source/logic review and Workbench compilation/reload. Optional `GC_TimerConditionTests.RunTimerChecks` covers briefing exclusion, deadlines, persistent results, nested flag gating, restart, match end, late updates and invalid durations. These fixtures compile but were not executed; no in-game tests were run at the user's request.

# Timer Condition

Add **Timer Condition** to an event or flag's Conditions list, including inside nested condition groups.

- **Timer Name:** optional unique name for Timer Action. Leave empty when no action needs to control this timer.

- **Start State:** Mission preview, Role selection, Briefing, Match (default), or After action.
- **Seconds:** typed decimal duration, default 60. For example, `2.5` is two and a half seconds.
- **Random Max Seconds:** optional typed decimal value, blank/default zero. Blank or zero uses Seconds exactly. A positive value chooses one duration between the two numbers when the selected state starts. There is no Random Duration checkbox.

Both numeric controls use EditBox rather than SpinBox, and remain float fields. Workbench may display its unset float as 0; zero explicitly means no random range. Seconds must be between 0.1 and 86400. Random Max Seconds must be zero or between 0.1 and 86400. Negative, too-small positive, excessive and non-finite numeric values are rejected. Workbench owns text-to-float input handling. Reversed valid bounds are ordered automatically: Seconds 120 and Random Max Seconds 60 means 60-120 seconds. Equal bounds use a fixed duration without a random draw.

It starts once when the selected state is observed, unless Timer Action already started it. Its answer is false until the deadline, then stays true unless Timer Action changes it or the framework restarts. Counting continues through later states, so a timer started in Briefing can finish during Match. If the framework starts in the selected state, timing starts then; it does not reconstruct past state-entry times. If the selected state already passed before the framework started, the timer waits for that state to be entered. State changes and condition checks never reroll the deadline.

## Timer Action

Use one **Timer Action** with **Timer Name**, **Operation**, and typed decimal **Seconds**. The name identifies one timer across events, flags, and nested condition groups. Missing or duplicate names reject the action without changing any timer; duplicate targets produce an explicit warning. Unnamed timers keep their existing automatic behavior.

| Operation | Behavior |
| --- | --- |
| Restart | Start now with the configured duration; choose a new duration if random. Clear a finished result and leave Pause. |
| Pause | Freeze a started timer's remaining time. Repeated Pause preserves it. |
| Resume | Continue a paused timer. Repeated Resume does not extend its deadline. |
| Set time | Replace remaining time with Seconds. Also starts a waiting timer immediately. |
| Add time | Add Seconds to a started timer. Can reopen a finished timer. |
| Remove time | Subtract Seconds from a started timer, stopping at zero. |

Seconds is used only for Set/Add/Remove: 0 to 86400, including decimals. Negative, non-finite, excessive values and additions exceeding 86400 remaining seconds are rejected without mutation. Set/Add/Remove preserve Pause while time remains; zero finishes immediately, even while paused. Pause/Resume do not change a completed result. Pause/Resume/Add/Remove on a timer that has not started are rejected; use Restart or Set time to start it. Actions change this run's remaining time, not the authored duration. Restart always uses the authored duration again. A later selected-state transition never overwrites an action-started timer.

Result changes notify the normal condition system. Every enclosing group and other condition still applies. The owning event's completed status and repeatability are unchanged: use Event Action > Reset separately if a once-only event should become available again. Timer Action does not cancel an event's already pending Delay. There are no new polling loops or global hooks; timing remains owned by the active framework engine.

Example: Start State Briefing, Seconds 60, Random Max Seconds 120 chooses one duration from one to two minutes after Briefing starts. Clear Random Max Seconds or set it to 0 for exactly 60 seconds. Combine the timer with Mission Flag `ready` inside All to require both. Neither bypasses the other. Use None around the timer for the opposite answer. Add a Game Mode State Condition if the event should only run in one particular phase.

The timer emits one expiry notification. It is not a repeating interval. Advanced Repeatable allows an event to run on later relevant notifications while the timer remains true; it does not restart the timer. Resetting an event also does not restart it. Use Game Mode State for an immediate state-entry condition, rather than a zero-second timer. Event Delay remains separate: it waits after the event's conditions pass.

Implementation uses timer leaf kind 12 in the existing per-engine scheduler. Fixed durations consume no random draw; a nonzero random range consumes one draw on selected-state entry. Authored containers hold no live deadline or result. The existing manager scheduler runs every 100 ms of world simulation time; expiry is observed on its next update, not on every rendered frame. Pausing simulation pauses this timing. All due leaves update before dependent notifications. Random Chance Condition retains its separate Match-only rolling behavior.

Existing local Timer Conditions contain no explicit old random settings and keep their Seconds and Start State values. For external resources using the previous schema: remove `m_bRandomDuration`; when it was true, rename `m_fMaximumSeconds` to `m_fRandomMaxSeconds` (use 120 if the old maximum was omitted). When it was false/omitted, discard the old maximum and leave the new field absent/zero. Otherwise a previously unused maximum could accidentally enable randomness. Script-created fixtures must explicitly set their start state; Workbench applies the attribute's Match default to authored resources.

Verification: source/logic review and Workbench compilation/reload. Optional `GC_TimerConditionTests.RunTimerChecks` covers selected-state entry, fixed/random deadlines, no rerolls, cross-state timing, after-action timers, persistent results, nested flag gating, restart, late updates, invalid ranges, pause/resume, decimal adjustments, early start, immediate finish, reopening a finished timer, random restart, overflow rejection and missing/duplicate targets. These fixtures compile but were not executed; no in-game tests were run at the user's request.

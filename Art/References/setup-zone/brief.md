# Setup-zone HUD

Active visual reference: [existing staging concept v2](../freeze-hud/concept-v2.png).
This reuses that composition and native layout. It is a wording/state adaptation,
not a new visual design. Runtime owner: GC_SetupPlayerController.

Player journey: spawn inside the applicable setup polygon, read the remaining
release time, remain inside (or stationary for Freeze), then resume normal play
when the countdown disappears. Keyboard/mouse and gamepad share the native
control-processing hooks; this HUD never takes focus.

| Element | Player question | Visibility and owner | Decision |
|---|---|---|---|
| SETUP | Why am I restricted? | Enrolled owner during setup | Retain staging title position |
| Countdown | When may I leave? | Server-supplied remaining zone time | Retain dominant timer |
| Guidance | What can I do now? | Mode/safe-start state; outside warning replaces normal guidance | One line, no duplicate timer panel |
| Dark surface and amber edge | Can I read this against gameplay? | Input-transparent native layout | Reuse existing theme |

Defining traits: compact top-center panel; amber title/edge; off-white dominant
timer; subdued guidance; no controls, icons, extra window or input capture.
Intentional differences from the staging reference: SETUP title, unpadded minute
count, freeze-specific guidance and an outside death countdown in the same final
row. The main release countdown remains visible throughout a soft violation.

Acceptance: correct owner only; stable top-center placement; no clipped guidance;
no extra HUD instances after replacement/release; input remains gameplay-owned;
release and soft timers are distinguishable. Original full-window MCP capture:
freeze-hud.png (3840x2160 window, saved at 1920x1080). Reduced viewport/fullscreen,
separate clients and different UI scales need explicit runtime evidence.

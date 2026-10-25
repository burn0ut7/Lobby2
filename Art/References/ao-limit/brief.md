# AO warning reference

Active reference: concept-v2.png, generated with the built-in image tool. concept-v1.png is retained. The landscape is illustrative, not a game capture. User approved the composition and requested seconds plus milliseconds; v2 changes only 00:20 to 20.000.

Prompt: preserve the first concept's black panel, white skull, RETURN TO AO heading and illustrative game background; replace the timer with 20.000. Show minutes only when nonzero, such as 1:05.250.

| Element | Purpose | Owner |
| --- | --- | --- |
| Black panel | Keep the warning readable over terrain | Layout |
| White skull on left | Signal lethal countdown | Native death texture |
| RETURN TO AO | Tell the player how to avoid death | Layout |
| Seconds.milliseconds | Show time left, minute prefix only when needed | Authority countdown, locally interpolated display |

HUD only: no focus, cursor, buttons, input capture, extra labels, or sound. Hidden inside permitted space, when exempt, dead, unpossessed, outside GAME or during staging. Multiple limits share one warning. Target at 1920x1080: top-center black panel, 420x100 at y=70; white skull left, title 28, timer 40, background black with alpha 0.78. Native skull differs from generated illustration intentionally. Check actual full-screen and smaller viewport, bright/dark backgrounds, countdown transition 60->59.999, hide/reopen, ownership and phase cleanup. Actual MCP captures are runtime-compact-seconds.png and runtime-compact-minutes.png. They supersede earlier runtime images: the user requested about 30% smaller dimensions and a translucent lobby-style background after reviewing the first implementation. The saved layout is 28.8% narrower and 30.1% shorter; alpha matches Lobby_Spectator_Group_Hover.layout. Earlier concept images retain the approved composition, not the final scale/opacity. Host/peer warning delivery and cleanup were exercised; see verification.md for coverage and limits.

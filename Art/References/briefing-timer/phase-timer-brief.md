# Phase timer revision

User now requests a general elapsed timer with no Briefing label. Keep only mm:ss in the existing header position. Reset when the authoritative state changes (Preview/PREGAME, Lobby/SLOTTING, BRIEFING, GAME, POSTGAME), not when browsing another tab. Gameplay HUD explicitly deferred. Reuse runtime-peer-1280.png composition, removing only the label; no new surface or control.

One server phase-entry timestamp and its phase identify the current clock. Existing clients receive transitions and JIP receives the current snapshot. The header shows the current authoritative phase duration wherever it appears; reopening it during GAME reports time since GAME began. Invalid/not-yet-matching replicated timing stays hidden instead of displaying the previous phase time.

Verify initial Preview counting, every phase reset, navigation/reopen continuity, live peer transitions, delayed JIP, and plain header text. No per-second network traffic.

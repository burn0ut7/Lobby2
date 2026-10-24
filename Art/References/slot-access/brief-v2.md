# Single lobby padlock (active reference)

Supersedes the lock-control portion of brief.md / concept-v1.png following the user's correction: exactly one compact icon on Lobby, no Briefing control and no separate locking admin tool.

Active image: concept-v2.png, built-in image generation, 2026-10-24. Prompt: preserve the actual Lobby2 slotting screenshot, add only a 40px white padlock in a 48px target at the right of Slots, keep the existing composition, no text button/toolbar/additional admin window or Briefing control. Source: prior MCP capture `reforger-script-tools/.local/admin-moderation/slotting-voice-after.png`. Historical baseline, not current runtime evidence.

Read-only comparison: ReforgerLobbyCommunity (original directory, not its Copy), `PlayableSelector/UI/Lobby/CoopLobby.layout` ToggleLockAll and `PlayableSelector/scripts/game/UI/Lobby/PS_CoopLobby.c` UpdateButtons. It uses the stock `icons_wrapperUI-64.imageset`, `server-locked` / `server-unlocked` with a 40px image and native input button. Reuse the same stock icons without introducing a dependency on that addon.

Journey: admin opens Lobby, sees closed lock when any slot is locked, clicks to unlock all; open lock means all open and click locks all. Reservations/occupants are preserved. Pending disables the icon; only pending/failure feedback appears in the heading. Non-admins retain the Slots heading with no icon. The existing reservation tool separately provides first-join reservation cleanup; no lock control lives there.

| Element | Need / owner | Decision |
|---|---|---|
| Existing Slots heading | Identifies roster | Keep |
| One native padlock | Availability and bulk action, replicated slot owner | Add in heading only |
| Pending/failure message | Honest request outcome, correlated reply | Show only when needed |
| Existing faction, roster, loadout, voice, navigation | Existing player journey | Preserve |
| Large lock text button, Briefing copy, lock admin panel | User explicitly rejected | Remove |

Theme and critique: native white icon, charcoal control, existing focus affordance. A 48px heading accommodates the hit target. Generated imagery approximates the icon; implementation uses the exact stock images. Compare actual screenshot at 1920x1080 and 1280x720 when MCP returns. Verify keyboard/controller focus as well as mouse entry. Concept is readable and has one control in the intended context; current runtime rendering is unverified.

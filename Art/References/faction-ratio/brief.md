# Lobby ratio toggle

Active reference: concept-v1.png (built-in image generation, 2026-10-24), a minimal edit of ../slot-access/concept-v2.png. Prompt: retain the existing Lobby composition and single padlock, add one 40px scales icon in a 48px native target immediately left of the padlock, 6px gap, no permanent label/panel/extra screen, amber focus on ratio, admin view. The concept is not a current runtime screenshot. Implementation uses the exact stock `lawsOfWar` glyph, which the generated image only approximates.

Journey: authenticated admin opens Lobby, sees scales beside padlock, clicks to enable/disable proportional slotting, waits for server response. Both icons remain hidden and disabled for ordinary players. Ratio starts disabled as in the existing manager and Community's default; admin deliberately enables it. Enabling never forcibly moves current players. Slot locks, claims and faction-switch policy are separate.

| Element | Need / owner | Decision |
|---|---|---|
| Existing padlock and Slots heading | Availability and roster context | Keep |
| Single scales button | Admin ratio toggle, existing authoritative faction rules | Add beside lock |
| Existing heading feedback | Pending or rejection, request-correlated reply | Reuse |
| Other faction, roster, loadout, voice/navigation elements | Existing player flow | Preserve |
| Separate ratio admin tool, duplicate toggles | No need | Omit |

Theme: existing charcoal native button, white scales when enabled/red when disabled (Community pattern), authoritative icon color, amber focus. No optimistic toggle; click intent goes through existing controller/server rules and audit. Both buttons use the same live authenticated-admin check, cleared on role changes.

Source reference: original ReforgerLobbyCommunity/PlayableSelector/scripts/game/PS_GameModeCoop.c `GetTargetRatioPerFaction` and `CanJoinFaction`; UI/Lobby/CoopLobby.layout `ToggleRatio` uses stock `icons_wrapperUI-64.imageset` / `lawsOfWar`. Community divides connected population by total slot proportions; Lobby2 already implements that gate with deterministic largest-remainder rounding. Preserve that single implementation rather than copying Community's independent rounding, which can overshoot population on ties. Existing native group/player data remains membership authority.

Critique before edits: matches the requested compact companion icon, preserves lock placement and avoids another panel. Generated labels are illustrative. Acceptance requires current MCP captures for admin/non-admin, ratio enabled/disabled, pending/rejection, keyboard/controller/mouse focus at 1920x1080 and 1280x720, and actual multi-peer slot restriction/JIP. MCP runtime results and remaining gaps are recorded in validation.md; embedded-host evidence is not multiplayer acceptance.

TILW: inspected local tilw-mission-framework/addon.gproj and scripts. No TILW-specific compile-time macro found. Its map-shape component is TILW_MapShapeComponent. Lobby2 currently references no TILW classes or dependency, so leave integration out as requested. Do not add a made-up macro, reflection workaround, or hard dependency. Other ongoing native map-marker edits are outside this change.

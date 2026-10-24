# Slot availability and reservation clearing

The lock-control design below is superseded by brief-v2.md and concept-v2.png. Retained as design history; the reservation cleanup scope remains applicable.

Active reference: concept-v1.png, generated with the built-in image-generation tool on 2026-10-24. References: prior MCP captures ../faction-readiness/runtime-icon-toggle.png and ../slot-reservations/implemented-transfer.png. These are historical baselines, not current runtime evidence.

Journey: authenticated admin opens Lobby or Briefing, selects Unlock all slots (if any locked) or Lock all slots, waits for server response, sees replicated availability. Reservations and occupants remain. Admin Tools > Slot reservations offers the rare Clear all reservations action, followed by Confirm clear all / Cancel. Only first-join reservations are removed. Mouse and native keyboard/controller focus are intended; runtime coverage is pending at nominal 1920x1080 and constrained 1280x720.

| Element | Purpose / placement | Owner | Decision |
|---|---|---|---|
| Existing navigation, map/roster, voice list | Preserve normal player journey | Existing menus | Keep |
| All-slot button below header, above voice list | One immediate availability action across factions | Server slot locks; admin visibility | Add |
| Result text, only when needed | Pending or rejected request feedback | Correlated server reply | Add |
| Existing reservation transfer controls | Keep bulk import/export accessible | Existing reservation service | Keep |
| Clear all reservations with first-join explanation | Rare cleanup in reservation tools | Existing reservation service | Add separately |
| Confirm clear all / Cancel | Review destructive scope without a new modal | Local confirmation, server reauthorizes | Add only when confirming |
| Duplicate state badges, roster selectors, counters | No additional decision served | None | Omit |

Theme: existing charcoal rectangular native buttons, off-white condensed labels, amber focus. Reuse the existing button resource; do not introduce a new input mechanism. Concept prompt requested two context frames, exact button labels, the existing composition and no additional dashboard elements. The generated map frame is cropped and its footer does not label itself as a concept; this brief supplies that provenance. It is a visual target, never game evidence. Implementation keeps a 375-pixel-wide control above the voice list and uses the full existing viewport; retain native readable font sizes rather than matching the mockup's scaling. Reuse dark button fills; the generated amber export fill is not a requested change.

Acceptance: admin-only control on both views; correct mixed/all-open labels; default locked/configured unlocked startup; one authoritative bulk lock update per peer; occupied and reserved slots unchanged; JIP agrees; logged-out requests rejected and centrally logged; pending reservation clear never removes occupied/reconnect/KIA claims; confirm/cancel; reopen cleanup; screenshots, focus and constrained viewport checks.

Reforger MCP tools disappeared from this session before implementation. Live source-index API checks, compilation, multiplayer, input and screenshot comparisons are unavailable until reconnection. Do not infer a pass from source inspection.

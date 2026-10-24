# Kick / Ban

Journey: Admin Tools → Kick / Ban → search or browse a player → inspect identity → enter reason → choose Kick or Ban → review → confirm → observe authoritative result.

| Element | Purpose and visibility | Owner | Decision |
|---|---|---|---|
| Existing heading, tool list, Close | Navigation; keep established full-screen layout | Tools component | Keep |
| Kick / Ban tool and heading | One home for moderation | Tools component | Combine |
| Searchable player picker | Select one connected player; browse without knowing name | Native roster, existing picker | Keep |
| Current name, Arma UID, platform profile availability | Distinguish renamed accounts; no guessed Steam mapping | Verified engine identity | Keep |
| Reason / note | Record rationale, shared by both actions | Admin input, server validates | Keep |
| Permanent / Duration / Until date | Choose ban lifetime | Server validates native ban seconds | Keep, relevant to Ban |
| Duration amount/unit or UTC date/time | Specify expiry without competing fields | Admin input | Show selected mode only |
| Kick and Ban buttons | Explicit outcomes in one workspace | Server authority | Keep |
| Inline review and Confirm / Back | Verify consequential action without another modal | Identity-bound snapshot | Show during review |
| Status | Pending, rejected, uncertain, confirmed outcomes | Server response | Keep |

Theme: existing near-black full-screen surface, white primary text, muted secondary text, existing amber selection. No badges, cards, decorative outlines or duplicated explanations. Native button outlines/focus retained. Clear type sizes and ample spacing.

Baseline: MCP capture `.local/admin-faction-tools/heal.png` in MCP repository (earlier task, before ADMIN badge removal). Intended mouse/keyboard workflow; gamepad and constrained viewport require runtime evidence.

Concept: concept-v1.png, built-in image generation; illustrative full-screen 1920×1080 composition. Representative identities are fictional. Final source must label unavailable platform identity honestly.

Acceptance: one moderation tab; selected identity readable; searchable roster; reason required; exclusive expiry modes; invalid/past/overflow dates cannot submit; final review binds same target and reason; server rechecks login and identity; pending is not success; no duplicate action after confirm; no custom ban store.

Concept critique: the generated image incorrectly shows two simultaneous review cards. Implementation will show the two simple action buttons in edit state, then ONE inline review for the chosen action with Confirm and Back; no boxed review cards. Native profile availability permits an Open platform profile button; do not display a fabricated Steam name. Existing native button outlines and full-screen geometry take precedence over generated decoration.

Active concept: concept-v2.png supersedes v1 following user feedback. Compact searchable session table includes disconnected players; player picker replaced. Ban expiry is one dropdown with conditional amount/date input. Manual Arma UID allows offline bans. Steam ID only resolves via verified known session mapping. Table names/identity can truncate visually with full selection details below. Existing bans may use native paged view; must label page-local filtering. Keep neutral status text rather than generated green/red. No separate badge.

User refinement during runtime review: remove Refresh, selected-player detail line and Open platform profile button. Keep table row selection; move automatically filled/editable Arma UID to the bottom left beside Kick/Ban. Use relevant server events for refresh; no polling. Native dropdown root requires a Lobby2-owned inherited layout above full-screen menu Z order and upward opening. All these simplify concept-v2 rather than introduce a new visual vocabulary. Session-only rows preserve observed identity history, not a replacement ban database. Table Steam column contains native verified Steam ID where available. Other active work added Slot reservations; moderation occupies the next navigation row.

Clipboard addition: right-click a session row → existing context popup → Copy player details → inline copy result → paste externally. Reuses the active table concept and existing context-menu presentation; no permanent controls added. The context stores the clicked identity, independent of the selected row or editable UID. Server rechecks authenticated admin access and exports its observed name, Steam ID (or Unavailable), and Arma UID to the requesting owner's PC clipboard. The existing status line reports actual clipboard readback. Cancel leaves clipboard and form selection unchanged.

Latest user-directed interaction: Kick and Ban execute on the first click after field validation. Remove the inline review, Confirm and Back controls entirely; disable actions during pending requests. Right-click a session row also offers Remove ban, using the clicked stable Arma UID and the existing native server ban service. It remains available for session identities regardless of the table's historical ban status, because the native ban store is authoritative. This supersedes the confirmation journey above without changing concept-v2's table/form composition. User verified external clipboard pasting on 2026-10-23; scripted embedded clipboard readback was empty and is not passing clipboard evidence.

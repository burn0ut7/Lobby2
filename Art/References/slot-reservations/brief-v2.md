# Slot-selected reservations (active reference)

This supersedes concept-v1 and its duplicate slot list, rejected by the user. The existing slotting screen is the sole place to choose a slot. Journey: secondary-click a role in Lobby > Reserve slot > review its fixed team/group/role > enter Arma UID and optional display name, or choose a connected player > Reserve slot > server result > Close to return. A pending reservation can be cleared here. Assignment never pins the player. Admin Tools contains only the import/export utility.

| Element | User need | Owner and visibility |
|---|---|---|
| Existing slotting screen behind dimmed dialog | Keep familiar selection context | Existing menu |
| Reserve slot title and Close/Back | Identify task and return | Native modal menu |
| Selected faction/team, group, role, slot number and current claim | Confirm exact target without another selector | Authoritative replicated slot |
| Arma UID and optional player name | Identify offline player, display useful label until first join | Local draft, validated server-side |
| Connected-player picker | Fill identity and name from current roster | Existing searchable picker |
| Reserve slot and Clear reservation | Commit assignment or remove initial hold | Authenticated server; clear only pending reservations |
| Result message | Pending, confirmed result, recoverable failure | Request-correlated reply |
| Separate import/export text area and explicit transfer controls | Reuse roster with descriptive metadata | Existing Admin Tools, no slot selector |

Theme: existing charcoal surfaces, off-white condensed text, gray borders, amber primary action. Dialog centered within the game viewport, existing lobby remains visible but inactive beneath it. Use native menu lifetime and back handling. No second faction/group/slot list, no duplicate selection controls, no invented website controls.

Baseline: original MCP screenshot `.local/admin-moderation/slotting-voice-after.png`, inspected on resume; depicts current Lobby2 slotting UI inside Workbench. Concept-v2 will use this as reference. Reuse existing WLib buttons and player picker. Minimum initial checks: normal and constrained viewport, long names, pending/current result, stale target, invalid UID, permission loss, close/reopen, and real context-menu entry. Record missing gamepad coverage explicitly.

Export describes scenario, stable slot key and current display number, faction key/name, group key/name and authored prefab/position, role key/name, player name and Arma UID. Descriptive names are snapshots, not import authority. Import matches stable keys and current eligibility and validates the whole request before claims change.

## Concept critique before implementation

concept-v2.png preserves the actual slotting screen behind a focused dialog. Native implementation omits the generated flag/icon decoration and redundant ready label; the team/group text and existing role label already establish the target. The dialog is sized proportionally to the game viewport so the UID stays on one line, with wrapped target/status text and explicit Close. Admin Tools contains only the transfer text and its four actions.

Rendered comparison: the concept's amber primary button produced insufficient contrast with the inherited white label. Reserve and Import therefore deliberately use the existing native dark button surface and light label. The fixed target, field order, parent-screen backdrop and separate transfer-only utility remain as designed. Constrained viewport and controller-navigation coverage remain unverified.

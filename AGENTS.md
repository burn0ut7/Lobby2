# Lobby2

Game-mode phases: lobby/role selection → briefing → match → after-action report. Keep transitions explicit.

Use `$reforger` for game research, coding, UI and MCP testing workflows. Consult the [architecture reference](docs/Lobby2_Architecture_and_Best_Practices.md) for affected lifecycle, multiplayer, controller, group or menu responsibilities; this file takes precedence if guidance conflicts.

## Ownership

Base-game APIs/events own factions, groups, membership and character affiliation. Lobby2 owns slot selection, reservations, locks and presentation metadata. Reference authoritative IDs and re-query ownership rather than maintaining shadow relationships. Discuss a new ownership source before introducing one.

Keep one canonical implementation per responsibility. Prefer the smallest coherent change; discuss changes to the phase model or multiplayer ownership.

## Mode isolation

Lobby2 may remain loaded in unrelated modes. Scope effects to the active Lobby2 mode, including asynchronous callbacks and RPCs. Shared overrides preserve inherited behavior when inactive. Release Lobby2-owned state/listeners on exit without overwriting others' changes.

For shared override changes, verify unrelated-mode behavior and transitions against an addon-free baseline; voice changes include transmission, reception and routing. Record missing coverage.

## Administration

Require an active authenticated admin login at execution time; enforce it on the server wherever supported. Reject unresolved or logged-out requesters without mutation. UI visibility is not authorization.

Route accepted and rejected actions through `GC_AdminActionLogger.Log`: requester, action, target/context and outcome, excluding credentials. Keep engine print logging behind this single extension point. Test authenticated execution, logged-out rejection without mutation and audit output.

## Tests and delivery

Keep project helpers in the optional [test addon](tests/README.md), using production handlers and real permissions. Production Lobby2 remains independent of it; tests run explicitly through MCP.

Commit and push completed task changes. Commit subject: at most five words; no body.

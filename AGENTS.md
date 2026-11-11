# Lobby2

Game-mode phases: lobby/role selection → briefing → match → after-action report. Keep transitions explicit.

Use `$reforger` for game research, coding, UI and MCP testing workflows. See [release structure](README.md). Keep development tools, test addons and evidence outside this addon.

## Ownership

Base-game APIs/events own factions, groups, membership and character affiliation. Lobby2 owns slot selection, reservations, locks and presentation metadata. Reference authoritative IDs and re-query ownership rather than maintaining shadow relationships. Discuss a new ownership source before introducing one.

Prefer supported engine and base-game capabilities over custom helpers. Retain custom complexity only for a concrete feature, policy or compatibility gap; verify behavior and preserve features before replacing it. Line-count reduction alone is not a reason to change a contract.

Keep one canonical implementation per responsibility. Prefer the smallest coherent change; discuss changes to the phase model or multiplayer ownership.

## Framework independence

TilW Mission Framework is research material only. Do not add its dependency, call its APIs, extend its classes, use fields injected by it, or place its framework entities in Lobby2 resources. Retain source attribution when documenting research.

Choose behavior from Lobby2 requirements and supported base-game APIs. Evaluate reference patterns for ownership, authority, lifecycle, performance and failure behavior before adopting the idea; matching TilW behavior is not an acceptance criterion. Implement and validate Lobby2's own behavior, including operation with TilW absent.

Optional integrations with separately requested addons belong in separate companion addons; they must not make production Lobby2 require those addons. The optional Map Drawing integration is distinct from TilW Mission Framework.

## Mode isolation

Lobby2 may remain loaded in unrelated modes. Scope effects to the active Lobby2 mode, including asynchronous callbacks and RPCs. Shared overrides preserve inherited behavior when inactive. Release Lobby2-owned state/listeners on exit without overwriting others' changes.

For shared override changes, verify unrelated-mode behavior and transitions against an addon-free baseline; voice changes include transmission, reception and routing. Record missing coverage.

## Administration

Require an active authenticated admin login at execution time; enforce it on the server wherever supported. Reject unresolved or logged-out requesters without mutation. UI visibility is not authorization.

Route accepted and rejected actions through `GC_AdminActionLogger.Log`: requester, action, target/context and outcome, excluding credentials. Keep engine print logging behind this single extension point. Test authenticated execution, logged-out rejection without mutation and audit output.

## Tests and delivery

Keep any test helpers in a separate addon outside this release tree, using production handlers and real permissions. Production Lobby2 remains independent of test addons; checks run explicitly through MCP.

Commit and push completed task changes. Commit subject: at most five words; no body.

## Spatial checks

Do not use `QueryEntitiesBySphere`. Area/capture conditions use direct named
references or native player/AI registries and point-in-area checks. Avoid world
entity scans for recurring area sampling.

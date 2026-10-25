# Scoped slot locks — 2026-10-25

Admins can right-click a faction or group and choose **Lock faction/group slots**
or **Unlock faction/group slots**. Mixed scopes offer both actions; an action
with nothing to change is omitted by the existing context-menu presenter.
Individual slots retain their existing Lock/Unlock slot action.

Only currently empty, unclaimed slots qualify. Occupied/reserved slots are not
freed, and scoped actions do not change the default for newly created slots.
The whole-lobby padlock retains its existing default-setting behavior.
All bulk requests share one owning-controller RPC and authority implementation;
the server resolves current group/faction membership and logs every result.
An invalid group/faction cannot broaden a request to the entire lobby.

## Embedded Workbench evidence

Optional `tests/McpTestAddon`, Arland example, Reforger 1.8.0.13. Native right-click
handlers open the menus; actual popup button handlers dispatch mutations.
Local captures and raw receipt readbacks: `Art/References/scoped-slot-locks/`.
Audit log: `logs_2026-10-25_19-06-59/console.log`, lines 638–708.

- USSR unlock changes its eight slots, leaving all sixteen US slots locked.
- Group lock/unlock changes its two slots, leaving other groups unchanged.
- Mixed faction menu presents both actions; faction lock normalizes all eight.
- Individual slot lock/unlock works through the existing context action.
- With slot 1 occupied, faction lock changes seven eligible slots and preserves
  slot 1's player and unlocked state.
- Unknown requester and missing scoped targets reject without state changes.
- After removing the local host's admin roles, a controller bulk request rejects
  on authority, and faction context has no admin actions. This is role removal,
  not a literal native logout command.
- Existing whole-lobby request unlocks then locks all 24 slots in a fresh Play
  session (`r6f5f05d42a3b4c8181354714a09d964c`,
  `r27be908424de4039bbf85c3cca40c7d6`).

The first open-popup teardown exposed an existing null access in
`GC_ContextMenuComponent.HandlerDeattached` at 19:08:53.345. Cleanup now uses the
input manager captured on attach instead of looking it up during game destruction.
Final reload `99a04240c8e24fa981f47a6075698df3` succeeded. Opening the faction
popup (`r106342fba45f4575b7e3839c1071e34c`) and stopping Play
(`1b6e871f07e64dfd9a3499d4787731e7`) completed without that exception;
the fresh log cycle, lines 981–1349, has no VM/null-pointer exceptions.

Separate-client replication/JIP, physical mouse hit areas, and literal admin
logout were not rerun. Slot replication/serialization and permissions use their
existing owners; this is not evidence of those untested journeys.

# Admin Gear

Admin Tools → Gear closes Admin Tools and opens the shared catalogue inside the
native inventory. The top-right selector chooses Inventory, Nearby crate (closest
eligible standalone storage within 5 m), or Create crate. Creation uses the shipped
empty USSR ammo box, traces nearby ground/floors, and selects the new crate.
The selected destination applies to catalogue spawning. Existing inventory items
continue through native movement/drop handling. Prefab Save / Load stays Workbench-only.

The owning controller derives requester identity on authority. Open, creation and
item insertion require `AuthorizeAdminAction`; item requests additionally validate
the current character's replication identity, carryable resource metadata, owned
target storage, slot compatibility, and crate proximity. Native inventory callbacks
report completion; centralized admin logs record successes and rejections. Pending
destination selection is disabled and responses are bound to their originating
Gear session. Unavailable replication identities are rejected before insertion.

## Evidence — 2026-11-01

Workbench 1.8.0.13, PID 55132, optional McpTestAddon, Empty_Lobby test world.
Production handlers were exercised through MCP and optional test fixtures; this
was an authenticated embedded host, not a connected remote-client test.

- Reload `1f90b26f028b441b98c46b6452ec665d`: no script compilation errors. Existing
  ArlandLobby GUID, SCR_DedicatedServerPlugin and m_aFormations diagnostics remain.
- Production Gear button: `rf07b127a89e64187abeca1f4c1be6215`; admin menu closes,
  catalogue opens, no Save / Load footer in admin mode.
- Create crate: `r1dabc748fdc54908b379d70f8988bda0`; selected, empty, no pending operation.
- Crate insertion: `ra9334c92d29c4f9bb14d19e18122be98`; crate items 0 → 1,
  player items remain 46.
- Switch to Inventory, insert: `raa2979ebf5fc493890880b90219f1215`; player items
  46 → 47, crate remains 1.
- Invalid target: `r1d27961a4e0a4a7db1fd26890c05d754`; counts unchanged, clear failure.
- Unresolved unauthenticated controller: `r77a4f198a90e47ea9ced9f8be1cc0281`;
  production open/create/spawn all rejected without inventory/crate changes.
  Workbench log `logs_2026-11-01_13-21-18`, lines 17916–17918, contains all three
  `rejected-not-logged-in` audit records. This is not a connected-player logout test.
- Nearby selection: `r396df7d4891142fa9e641027fbf01a9c`; existing crate selected.
- Move 20 m away, insert: `ra0605187691b493f8de9b3f20df884c3`; rejected, counts unchanged.
- Catalogue drag to clothing: `rc1b95d70d4c64d3787d4ba2c0ffff9dc`; player 47 → 48.
- Mode transition to native Isolation world: `re1e6650b6541467b85e7d1cb1bb30fdc`
  has customPanel=0 and visible native inventory. `rcd52f7b31e784421b47e5a09f6d96a94`
  reports selected=1, dragging=1, released=1, matching the recorded addon-free
  behavior in `evidence/inventory-catalogue-20261031/isolation.json` (viewport scales differ).
- Separate diagnostic client PID 20656 compiled Game scripts without WORKBENCH;
  `PeerPlugin1/logs/logs_2026-11-01_13-32-13/console.log`, lines 727–749, zero SCRIPT
  errors. No server connection was established. MCP stopped it with zero survivors;
  existing peer settings were unchanged.
- Returned to Empty_Lobby: Workbench browser `r2a06494726a54e65b9bc9201e1102358`
  retains files=1; switching through Admin Tools → Gear
  `rf0a48d51d3e34e11bd71951b18b01123` returns admin active=1, files=0.
  Left this test world and admin gear screen running, with the optional test addon loaded.

Raw receipts and captures reside in the tools workspace at
`.temp/local/evidence/loadout-foundation/`. Captures:
`1793557424337148200-capture_screenshot.png` (Admin Tools),
`1793557428623689400-capture_screenshot.png` (Inventory destination),
`1793557891198482700-capture_screenshot.png` (crate destination).
Independent source and visual review approved after replication-readiness,
pending-selection and stale-response fixes.

Remaining coverage: connected remote admin grant/revoke, dedicated gameplay,
and delayed network responses were not exercised live. Session-revision and
pending-response protections were source-reviewed. No claim of packaged-platform
or all-modded-container coverage.

## Reproduce

Use the optional `GC_TestNativeInventoryStart`, then `GC_TestAdminGear(action, prefab)`:
`menu`, `open`, `create`, `spawn`, `inventory`, `nearby`, `invalid`, `denied`,
`far`, `return`, and `state`. The fixture positions its test character on the
Empty_Lobby test terrain. `spawn` accepts an inventory prefab resource name.
`denied` creates and removes an unassigned test controller, without granting roles
or replacing production authorization. All helpers are Workbench-only and live
only in the optional test addon.

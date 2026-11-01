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

## Gear audit expansion — 2026-11-01

All records use `GC_AdminActionLogger`, hence the normal server console and
session-history persistence. Privileged requests record the authenticated requester;
spawn completion retains the requester's identity if the controller disconnects,
and never attributes that completion to a recycled player ID.

- `gear-open` means server authorization and a display request, not proof of what
  an untrusted client rendered. `gear-close` distinguishes client-reported close
  from controller shutdown.
- Spawn records include request sequence, prefab, item name, quantity (one entity),
  requested character/storage/slot/crate, automatic-placement flag, resolved
  destination and slot, entity replication IDs, positions, and any replaced item.
  Accepted/pending and completed/failed are distinct; rejection logs retain the
  requested context. Count completed `gear-spawn` records when counting spawns,
  not the pending or inventory-observation records.
- Crate selection/creation logs identify the crate prefab, replication ID, position,
  character and search radius, with an explicit result.
- Authority observes native inventory additions/removals for connected, logged-in
  admin inventory owners throughout the active Lobby2 mode, even after Gear closes.
  Records identify the item and affected storage. These observations **do not
  establish who initiated a transfer**: the log explicitly says
  `input-instigator-not-reported`. Native failed drag attempts are not successful
  inventory-change events. Inventory behavior/permissions are unchanged.

Native reload `a05c3c7d4d444a51b8e219694b6e92f0` compiled with no SCRIPT errors.
Live host checks in `logs_2026-11-01_13-21-18`:

- Lines 19085, 19170–19222: menu authorization, inventory/crate spawns, explicit
  rejection details, close, then native drop/take of the same item after close
  (`r2418b2b7326e43ce942592627fcd94e6`, `r72e2f7112fbb4a27bc362cb06498635f`).
- Replacement `rb3fc92ee872c4eb9937d187044ce0bfb`, lines 20592/20595: beret into
  character slot 0 records the old PASGT helmet and new beret; readable storage
  class name, matching pending/completed contexts.
- Persisted JSON independently inspected under
  `$profile:Lobby2SessionLogs/01a19868-1fb1-874f-8400-000000073df7-events-*.json`:
  batches 0 and 2–7 contain these open/spawn/crate/reject/close/native-change events.
- Transition to unrelated Isolation mode: `r93b4989b976242afa79c06b6094c9d29`
  has native inventory/customPanel=0; `rd08e0c64ac724861b99116326ee47217` reports
  selected/dragging/released=1, matching the recorded addon-free baseline above.
  Native drop `rd96469ff9d284b3f9080e0ed6d52a074` leaves the entity alive and outside
  inventory. No Gear audit entries occurred in that mode (after the Lobby shutdown
  record at line 20601 through line 20832). Returned to the original Lobby test world.

Fresh independent review approved. Connected logged-out remote clients and pending
callback completion after disconnect were not exercised live; identity safeguards
were source-reviewed. Native events are observations, not an automatic cheating verdict.

## Crate and quantity controls — 2026-11-01

Crate selection reuses the nearest eligible storage within 5 m (inclusive), creating
one only when absent. Its native vicinity grid replaces the player inventory and
preview on the right. Selected root crates bypass capacity limits while Lobby2 owns
the mode; insertion targets that root directly, never clothing stored inside it.
The top-right X closes Gear. Double-click opens the native quantity dialog: slider
1–100, positive integer text through 2147483647, Add and Cancel. Requests execute
one item at a time with existing server permission checks and completion reporting.
English runtime strings accompany the string table. Catalogue bottom spacing is
32 layout pixels shorter, retaining ten rows and both pager controls.

Embedded-host checks used installed 1.8.0.13 sources, catalogue revision
`721cd7b05eb54d3ca3591d9097af381d`, and production handlers through the optional
test addon. Receipts are in `evidence/admin-gear-quantity-20261101/runtime.json`.

- Final Reload `53097673e0a045da915fddf49182fa2b` had no script compilation errors;
  existing Arland map GUID and dedicated-server plugin errors remain unrelated.
- Invalid zero, decimal and overflowing quantities stayed in the dialog. Typed
  150 retained that amount with slider at 100; moving the slider to six updated text.
- Six bandages went to the crate; six to Inventory increased player count 46→52
  without changing the crate. Selecting Crate again reused its replication ID.
- A subsequent 150-bandage batch completed. Adding a jacket then six AN-M8 HC
  smoke grenades left all items at the root (169 root items, 31400/10000 volume,
  nested count zero; receipt `ra879d86bcd664651bdc5fd19280cd73a`).
- Cancel preserved counts. Exit closed Gear (receipt
  `rf675ad384e314f9d8c97c9925a8779a0`). Final paging advanced offset 0→10→0.
- Final 150 batch reported 150/150 and could accept more at 15000/10000 volume.
  After logout, raw open/select/single/quantity requests logged
  `rejected-not-logged-in` at 18:25:41 in `logs_2026-11-01_17-45-51/console.log`.
  Rejection and follow-up snapshots retained 46 player/179 crate items. Concurrent
  user interaction added items between earlier snapshots; those deltas are not
  attributed to the rejection test.
- Shutdown released the manager and restored native capacity: the same overfull
  crate changed unlimited=true/canFitMore=true to false/false. Unrelated Isolation
  mode then had customPanel=0, native player/grid visible, and
  selected/dragging/released=1, matching the addon-free inventory baseline in
  `evidence/inventory-catalogue-20261031/isolation.json`. Restored the Arland world.

Local MCP screenshots in `Art/References/admin-gear-quantity/` include
`quantity-six.png`, `inventory-six.png`, and `final-spacing.png`. The final image
shows translated labels, ten rows, tighter footer spacing and the native crate grid.
The earlier `crate-filled.png` predates removal of the native capacity indicator.
Remote replication and inventory-full partial batches were not verified live.
Double-click coverage invokes the production handler with native hit testing;
it does not simulate physical OS mouse double-click timing.
Independent source, evidence and final visual review approved these changes.

# Scoped slot purge

Authenticated admins can right-click a faction or group and choose **Purge faction slots** or **Purge group slots**. Purge releases connected occupants through the existing spectator transfer, clears reservations, and locks every affected slot. KIA remains KIA. **Unlock faction slots** and **Unlock group slots** remove those locks without reviving KIA slots. Overlapping lotteries are cancelled and announced so pending draws cannot refill purged slots. Failed transfers report partial completion; successful slot changes are not rolled back.

## Validation — 2026-10-28

Workbench 1.8.0.13, Lobby2McpTests, Empty_Lobby.ent, embedded host. Fresh Reload `30da3cf76fa74729bcb9da24c0af2e31` succeeded with no script compilation errors. Existing world/plugin diagnostics remained: Arland resource GUID mismatch, unknown SCR_DedicatedServerPlugin and m_aFormations. Empty_Lobby also reports missing map/loadout managers.

Actual right-click handlers and popup buttons passed occupied group purge, occupied faction purge, both unlock actions, reservation clearing, and preservation of other-faction slot states. A live entered lottery was cancelled by purge. The optional KIA fixture retained dead state while purge cleared its claim and applied its lock; unlocking left it dead. Invalid targets and requester -1 were rejected without roster mutation. The owner received a visible rejection hint. Central audit records and receipt brackets corroborate the actions.

Optional helpers: GC_TestScopeLockUI, GC_TestScopeLockSnapshot, GC_TestScopeLockReject(slotId, purge), GC_TestScopePurgeKIA and GC_TestScopePurgeInvalid. Local MCP screenshots, full returned states and audit capture are retained under tests/evidence/slot-purge-20261028 (ignored local evidence). Independent source and visual review approved this embedded-host scope.

Not tested: connected logged-out requester, remote clients, dedicated server, JIP, or induced partial spectator-transfer failure. KIA used a simulated claim/state fixture, not an actual death event.

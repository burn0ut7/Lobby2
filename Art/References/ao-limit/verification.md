# AO verification — 2026-10-24

Workbench/game 1.8.0.13, optional Lobby2McpTests addon, Empty_Lobby world. Production handlers were exercised through the generic Reforger MCP runtime interface. No permissions were granted by fixtures. Saved world geometry was not changed.

## Observed results

- Native script reload passed, including the final safe-history change and corrected server snapshot helper. Final helper receipt r013f8d524e3c4521a15e35bf23a7ec99 reports player, health, position and AO state correctly.
- Normal hard boundary returns the character. Inverted soft boundary expires and the server reports dead=1. Re-entry, exemption, disable/re-enable and removal clear/restart the appropriate clocks and warning.
- Higher-priority soft overrides hard while breached; disabling it exposes the hard restriction and returns using preserved history. Equal priority chooses hard. Exemption outside no longer overwrites a genuinely safe return position.
- Inventory presence/absence, faction inclusion/exclusion, on-foot/driver/passenger, and matching/excluded vehicle predicates were exercised. Native inventory traversal and prefab inheritance are used.
- Real M1025 compartments were used for occupied-driver and passenger correction. Native return moved the vehicle without ejecting its occupants.
- Distinct client player 2 connected to Workbench listen server (RplMode=2, port 2001). Owner report ao-soft-out shows HUD=1:26.843 while the host HUD is absent. Screenshot runtime-peer-warning.png confirms rendered delivery.
- Remote hard return is confirmed by ao-hard-out at x132.636/z125.2 and matching server dispatch/observation logs. Shared vehicle reports ao-vehicle-before/after confirm remote driver and host passenger stay seated after correction. Passenger-only restriction also returns the remote-driven shared vehicle (ao-passenger-rule).
- Remote soft expiry produced server log `expired player=2 ... dead=1`, followed by ao-expired-peer reporting the spectator character and HUD absent.
- Time formatting cases 20, 30, 65.25, 59.999, 60, 0, -1 and 0.001 passed. Live snapshots crossed from 1:00.725 to 59.467 without a leading zero-minute field.
- Latest compact/translucent HUD is rendered in runtime-compact-seconds.png and runtime-compact-minutes.png. White text/skull remain readable over bright sky; 420x100 and alpha .78 implement the user's size/opacity correction. Timer includes milliseconds. Removing the live limit returns limits=0, clocks=0 and HUD=absent.

## Evidence and limits

runtime-evidence-final.json contains cumulative receipt-correlated calls and results. ao-logs.json preserves production enforcement logs, including earlier failing checks and later corrections. Earlier snapshots/concepts are retained as iteration history. The malformed initial server snapshot helper output is not used as position evidence; independent owner reports and production logs corroborate multiplayer results.

Boundary crossings were arranged with the native editable transform, not keyboard walking/driving. Dedicated-server topology, loaded large missions, aircraft, terrain collision at authored return anchors, pathological polygons, and all multi-item combinations were not exercised. The latest compact layout was captured in the smaller Workbench viewport; the prior larger version was also captured on a separate full-screen-sized client. No dedicated dark-terrain capture was made. Enforcement is sampled at 100 ms, and client interpolation is presentation only.

Existing unrelated engine/resource warnings remain, including obsolete APIs, unknown legacy world/plugin resources and radio-manager warnings. No clean global-log claim is made. Earlier peer launches failed due to missing base-game/test-addon search paths or standalone hosting; these were diagnosed and the successful multiplayer run used explicit mounted paths and native local-server mode. Peer paths were restored and owned peers stopped after the test.

Saved-source readability review covered tabs, Allman blocks, separators, creator tags, names, authority checks and lifecycle. Related enums and small state records intentionally share their owning AO module; the warning extends the addon-owned controller only. Fresh independent reviewer ao_review approved the final implementation, host/peer behavior, compact seconds/minutes captures and documentation on 2026-10-24. No outstanding fixes were identified. Workbench Play and all owned peers were stopped, original peer search paths restored, and native Play settings read back as Viewport On / both local-server options Off. The test Workbench exited normally (code 0). Production Lobby2 was reopened in process 34708; native project context confirms only core, ArmaReforger and Lobby2 loaded, with both optional test addons absent.

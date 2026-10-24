# Lobby ratio toggle

## Behavior

The scales icon beside the padlock is admin-only. White/selected means ratio enforcement on; red/unselected means off, matching Reforger Lobby Community. Both controls are hidden and disabled without an authenticated admin login. No separate Admin Tools page or Briefing control was added. Ratio starts off, preserving the existing default; admins enable it per mission.

The existing `GC_FactionBalancePolicy` and manager join gate remain the sole policy implementation. Each faction's total slot capacity determines its share of connected players. Largest-remainder rounding gives exact integer totals and deterministic ties. Example: 10 versus 30 slots with 20 connected players yields quotas of 5 and 15. Locked/reserved slots still contribute to total authored capacity, as in Community. One-side scenarios are unrestricted by ratio. Ordinary joins to a side at its quota are rejected on authority. Moving within one's current side remains allowed. Enabling never moves existing players; existing explicit admin assignments retain their authorized override. Slot locks, claims and faction-switch rules are preserved.

Toggle uses the existing revision-checked faction-rules handler and reliable broadcast/JIP fields. A stale revision rejects rather than replacing another admin's changes. Request-correlated replies include rejection after logout. Visible slot eligibility refreshes after population/affiliation changes and topology changes on either side, without rebuilding unchanged rows.

## Required MCP execution

No runtime pass claimed: the Reforger MCP namespace is currently unavailable. New source/test helpers have not been compiled. Use tests/README.md to load the optional test addon when the connection returns.

1. Reload scripts and retain fresh compile output. Run `GC_TestFactionQuotas()` for exact ratio, zero population, asymmetric rounding, tied remainder and zero capacity cases.
2. In Lobby, run `GC_TestRatioUI()` as a non-admin: both visible and enabled flags false. Authenticate normally: controls appear. Capture admin/non-admin at 1920x1080 and 1280x720. Verify native mouse/keyboard/controller focus, enabled/disabled scales states and a single padlock. Briefing contains neither control.
3. Use existing `GC_TestClick("FactionRatioButton")` to exercise the actual handler. Inspect `GC_TestRatioUI()` after the correlated result and central `faction-rules` audit. Require authoritative state/revision to agree on all peers; join another client to check JIP. No optimistic state or success on dispatch. Toggle repeatedly and reopen: one request per click, no duplicate subscriptions.
4. Set up 10 versus 30 slots and 20 connected players, unlocked slots and unlocked faction switching. At 5 players on the small side, a sixth ordinary player cannot join; at 4, one can. The large side cap is 15. Use ordinary owning-player slot RPCs, not admin assignment, and verify native membership and selected slot after each request. Repeat with enforcement off: ratio alone no longer rejects. Re-enable while a side exceeds quota: nobody is moved, new cross-side entries are blocked. Same-side role switches still work. Repeat simultaneous peer requests at the boundary, odd populations, a single faction, locked/reserved slots, joins/disconnects and capacity changes on the nonselected side.
5. Run `GC_TestRatioReject(realLoggedOutPlayerId)` on authority and again with unresolved ID. Require unchanged slots/rules/revision plus central rejection logs. Dispatch actual logged-out owner requests too. Log out while the UI is open: both controls disappear and server rejection resolves pending state. Existing `GC_TestFactionRuleReject()` checks stale and invalid rule requests; verify another admin's policy survives stale toggles.
6. Before an accepted ratio toggle call `GC_TestSlotAccessRemember()` on each inspected peer; afterward `GC_TestSlotAccessCheck("unchanged", false)` must pass. Compare faction-lock override too. Inspect pending/rejection text and actual render against Art/References/faction-ratio/concept-v1.png.

## Optional map drawing

Inspected local `tilw-mission-framework/addon.gproj` and scripts: no TILW-specific compile-time macro declaration found. `TILW_MapShapeComponent` supplies that addon's shape rendering. Lobby2 has no TILW script reference or addon dependency, so this integration remains omitted as requested. Native map-marker changes from other work remain separate. A future explicit optional adapter needs a supported build boundary; do not invent a macro or require TILW for Lobby2 to compile.

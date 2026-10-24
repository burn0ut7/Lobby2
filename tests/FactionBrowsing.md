# Faction browsing and admin slot eligibility

Everyone may browse faction cards and hover roles to inspect their loadouts.
Browsing does not change the player's faction or slot. Regular players retain
the configured faction lock and balance policy. Authenticated admins bypass
those policies and empty-slot admin locks; occupied claims, reservations, KIA,
active lotteries and phase boundaries remain enforced.

## Runtime acceptance checks

Use the optional MCP test addon and production controller requests described in
README.md. Use a real authenticated admin and a regular connected peer; do not
grant roles in fixtures.

1. With faction locking enabled, assign the regular peer a faction. Browse the
   opposing faction and hover a role: its loadout must display, membership must
   remain unchanged, and clicking an enemy slot must not assign it. Submit the
   same request through the owning controller and verify server rejection.
2. As admin, select a free opposing slot while faction locking and faction
   balancing would otherwise reject it. Verify the resulting slot and native
   membership, plus the central `slot-self-select` audit outcome.
3. Repeat for an empty admin-locked slot. Verify regular-player rejection and
   admin acceptance. Occupied/reserved, KIA and lottery slots must still reject
   ordinary clicks; POSTGAME must reject all self-service slot changes.
4. Log out of admin with the menu open. Verify slot visuals refresh immediately,
   browsing remains available and the server rejects a prohibited enemy-slot
   request. Log back in and verify eligibility refreshes without reopening.
5. Join a peer after the phase advances and repeat browsing and slot rejection.
   Capture the actual menu and inspect correlated server/client state and logs.

## Evidence

2026-10-24: source paths and saved diff reviewed; whitespace check passed.
Reforger MCP tools were unavailable in the editing session. Script compilation,
runtime interaction, screenshots and multiplayer checks above are **not run**.

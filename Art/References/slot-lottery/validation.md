# Slot lottery validation — 2026-10-24

Implemented in Lobby2; tests are isolated in the optional MCP test addon.
Workbench 1.8.0.13, AdminMultiplayer.ent. Multiplayer coverage used an embedded
Workbench host and a separate remote peer, not a dedicated server. Final scripts
compiled on startup (06:49:39, completion line 222) and on the peer. All engine interaction used the configured
Reforger MCP server over its normal stdio contract. Source edits used local tools.

## Observed behavior

- Five independent single-slot draws ran together. Native chat `/join` entered
  only draw 5; the other four entrant counts stayed zero (response 35).
- The real 30-second callbacks assigned the entered roles through the production
  handler. A player entered in two independent draws moved on the later win
  (43/46). Unawarded locked roles stayed locked.
- Group and faction context actions created one pool each. The faction pool
  covered ten slots (94); the final result assigned one of them (101).
- Overlap, normal direct claiming, duplicate entries and stale IDs were rejected
  (37). A connected logged-out admin was rejected without mutation (145/147).
- Entry followed by withdrawal left zero entrants (139); advancing the phase
  cancelled the pending draw (143). Central audit logs recorded rejection,
  starts, awards and cancellation.
- Remote player 2 saw all five draws, entered draw 5 through native chat, and
  received slot 5 after 30 seconds. Client and authority snapshots agreed.
  Its unauthenticated start request was rejected by the server audit handler.
- A peer rejoining an active faction draw received draw 6, its nine eligible
  slots and 12 seconds remaining through the JIP snapshot.
- Two players entered one two-slot group pool. At the real deadline each
  received a different role; server and remote snapshots matched. See
  `peer-group-open.png` and `peer-group-result.png`.
- Outside Lobby2, after switching to Isolation.ent, the inherited `/join`
  callback ran exactly once (receipt rd0be8e76e963426ab29216fd9bd7675d).
  The addon-free baseline loaded only core, base game and isolation probe and
  returned the same result (rfa7596287fc14512badea9a27345def2).

## Visual inspection

[faction-lottery.png](faction-lottery.png) shows all ten participating rows with
clock icons and `Lottery open`, and the existing admin popup explains `/join`.
[group-announcement.png](group-announcement.png) records the shared group wording.
The first capture exposed stale lock icons; subscribing group widgets to lottery
updates fixed this, and the faction capture verifies the corrected state.
The feature reuses the existing context menu, notification and slot status design.

## Evidence and limits

`results.json` contains selected actual MCP returns and receipt identifiers.
Full numbered MCP responses, screenshots and server logs are retained locally in
`reforger-script-tools/.temp/local/evidence/lottery/`. Source API checks used the
installed PAK mirror revision 9b2fea665c0943b9bcf87ec2891bc011. Cached wiki research
was unavailable; this is not presented as live wiki validation.

Earlier peer attempts failed because a saved duplicate `-addonsDir` option hid
the nested test addon. After correcting that temporarily, a missing WORKBENCH
guard in the new context fixture caused compilation failure and cascading base
script diagnostics. Guarding the embedded-only method fixed compilation and the
peer connected successfully. Original peer parameters are restored after tests.

No physical mouse/gamepad navigation, small viewport, high-load stress, or
statistical fairness distribution test is claimed. Disconnect/reconnect eligibility races remain uncovered.
Saved source readability and whitespace checks passed. Independent review approved
the production logic and incremental fixture/isolation changes.

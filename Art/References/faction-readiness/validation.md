# Faction readiness validation — 2026-10-23

Arma Reforger 1.8.0.13, Arland Lobby, optional Lobby2McpTests addon. All editor,
runtime, peer and capture operations used the C# Reforger MCP server. Source
editing used local development tools. Selected receipts/results are retained in
`evidence.json`; complete local ledgers are in the MCP repository's
`.local/commander-readiness/` directory.

| Check | Observed result |
|---|---|
| Compilation | Final scripts reloaded successfully (`1792798711119511400`). |
| Command designation | First displayed group's first sorted slot: USSR 1, US 9. Lower occupied slot 2 does not inherit command. |
| Native button handler | USSR commander marked ready/unready/ready through the visible stock button component; server revisions advanced. |
| Authority | Wrong phase, non-commander, wrong faction and stale revision rejected without altering acknowledged state. |
| Slot departure | Moving commander from slot 1 to slot 2 revoked readiness and left commander vacant. |
| Join in progress | Standalone peer received USSR ready at revision 1, commander 1, before selecting its own slot (`1792798564183589100`). |
| Remote requests | Peer 2 selected US command slot 9; ready → unready → ready produced server revisions 2, 3, 4. Wrong-faction request retained revision 2 and returned an error to peer 2. |
| Actual disconnect | Existing authenticated kick handler disconnected test peer 2. Native connection/controller/slot readback became zero; US readiness revoked at revision 5, USSR remained ready (`1792798615884242100`). |
| Phase exit | Both factions ready at revision 2; admin Game action moved to GAME, revision 4, both unready (`1792798478333816000`). |
| Presentation | Standalone 1280×720 capture shows both commander names, distinct ready states, action only on own faction, and native focus outline. Two-line commander fixture fits without covering the action. |

The remote test caught and fixed a client-only vacancy label caused by requiring
other players' controllers locally. Native connection and faction queries remain
authoritative. Long text originally clipped at both ends; the final caption wraps
onto two lines. Native focus needed a Border widget, now present and captured.

`runtime-peer-1280x720.png` is the final layout with native focus applied through
the workspace API. `runtime-long-name-1280x720.png` verifies the same two-line
caption before the focus border was added; its long name is explicitly a visual
fixture, not an actual renamed player. The generated concept is not test evidence.

Limitations: button-handler and programmatic native-focus checks do not establish
physical pointer hit testing or full keyboard/gamepad traversal. Dedicated-server,
packet-loss and arbitrary many-faction layouts were not tested. No global engine
callback overrides were added by this slice. Workbench repeatedly displayed the
previously documented resource-leak assertion during script reload; it was
recorded and dismissed to continue, so this is not clean-teardown evidence.

Peer process termination with the existing timeout-disabled test setting did not
immediately produce a server disconnect; actual departure coverage therefore uses
the production admin kick and verified connection readback. An initial peer launch
failed because its saved extra addon-directory argument hid the nested test addon.
The test temporarily removed that argument and used 1280×720; original settings
were restored after testing. MCP verified stopped peers, original 1920×1080 and
addon-directory arguments, Server localhost Off, and only core/ArmaReforger/Lobby2
loaded after reopening the production project (`1792799039428648100`,
`1792799013583487800`). Independent review approved the result with the stated
limits. Saved source readability and scoped `git diff --check` passed.

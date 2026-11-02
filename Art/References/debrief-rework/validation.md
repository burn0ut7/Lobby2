# Debrief validation — 2026-11-02

Final live views: [Overview](overview-final.png), [Players](players-final.png),
[Activity](activity-final.png). The player-list capture uses an isolated 128-row
fixture; overview and activity use native match events. The original concept is
retained as design history; final captures supersede its geometry.

## Implementation and load bounds

Counters project the sealed backend mission batch. Activity reads the existing
session log; no additional damage/death collector or backend storage was added.
Personal history retains at most 256 events and reports truncation. Directory
pages contain at most 16 players; activity pages at most six events. Public
postgame lookup uses report ordinals, never persistent account identities.

A shared server queue sends at most eight responses per 50 ms tick with an 8000
estimated-size budget, reserving 3800 units before each response. These are
conservative text/field estimates, not measured wire bandwidth. Each controller
has one pending server request. Clients serialize requests and cache up to 128
immutable pages across menu reopen. Final directory metadata is computed once.

## Live evidence

Workbench PID 78252; Lobby2 MCP Tests; Arland_Lobby; console session
logs_2026-11-01_22-52-25. Final Reload operation
0021eca3c4474b289a5e27e961309a0d completed (lines 18505–18642).
Final-cycle log inspection through line 18961 found no debrief compile/runtime
errors. Existing Arland resource GUID/world-property errors, mouse-mask error
and the test world's missing loadout manager remain unrelated limitations.

- Invalid/pre-final requests: no rows, explicit Unavailable
  (receipt r1d8bde40877a400b941f152c18f50e79).
- Production postgame finalization: r74f49f0964aa4c288824c0edcb5c96ca.
- 128-player production report fixture: valid, eight pages, maximum 2112 payload
  characters, build 3 ms / total 4 ms (r4d834b9044b0439d981330ace4f57370).
- Isolated 260-event fixture: bounded to 256; all-events 43 pages; each of four
  filters 64 events / 11 pages; decreasing cursors and retention status verified
  (rf1c0e6b46a4d49d5aff9eae4db3f2aac).
- Twelve warm-cache requests remained at one server request and delivery
  (rdbb072aa65934949994276b39fb88a3e).
- Native damage/kill handlers produced nine activity records across two pages
  (r909b32eea5c44ee199d9f2f7fe3d220c); incoming-damage filter selected one
  (r3f952a1777bc4510a0ad855fd0721cce).
- Search found Jim Bob among 128 fixture rows; sorting and direction reversed
  the first row correctly (r3b6dc293db51422eb824367cb6ded4b2,
  rd783bbf695ab4bea85512116dbf50efd, r5e978187493c4f26b46af408a699e772).
- Player-list failure remains visible after rendering as 1 player · Unavailable
  (r39fb1ca8b89e42158d8e2d0c483b3294). Fixed-stat sorting avoids duplicate columns.
- Fixture state restored; selecting the real player opened Overview
  (r33dec219927f4d9a97051f0b058ce5ee), then Private was restored.

Independent source and visual review approved the final changes after resolving
error-status visibility and duplicate-stat-column findings.

## Remaining coverage

Host/local fixtures do not establish actual 128-client transport performance.
No remote-client soak, packet-loss, gamepad, multiple viewport/DPI or addon-free
mode-transition baseline was run in this pass. Estimated payload bounds and
local timings are not a network throughput guarantee. Shared overrides retain
mode guards and inherited behavior; broader isolation remains unverified here.

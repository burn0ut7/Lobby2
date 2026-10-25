# Setup-zone validation — 2026-10-25

Environment: Workbench 1.8.0.13, embedded authority/owner, optional Lobby2McpTests
and IsolationProbe. Empty_Lobby / GameMode_Lobby3; final isolation used stock
IsolationMode after leaving Lobby2 in the same process. Production and optional
test scripts reloaded successfully after final changes. Existing world/resource,
missing-loadout and map errors remain in the session; no new setup compilation
error was observed. Wiki index was unavailable; research used installed source
catalogue revision `4942ab6194e34c74bd5972e7761785e2` and existing project evidence.

## Observed results

- Freeze: native movement disabled=1, exact zero displacement under two seconds
  of continuous native forward input; remaining timer 0:04 then 0:02. At expiry,
  active=0, frozen=0, movement disabled=0 and HUD text absent. A separate released
  walk moved 15.16 m. The first pass also held four seconds of forward input.
- Hard: five seconds of continuous forward input against a 2 m half-extent held
  the player at 1.99 m. This tests current input, not just a dispatched teleport.
- Soft: crossing 8 m outside showed a separate death countdown while the setup
  release timer remained visible. Re-entry removed the violation guidance.
  A later 20-second continuous violation killed the original character at
  02:38:36.234; native death/slot/spectator processing followed. The later controlled
  entity is a living spectator body, not evidence the original character survived.
- Eligibility: US character excluded by USSR filter stayed inactive. A character
  first observed outside remained inactive after moving inside. Matching US was
  enrolled. Each fixture used the shipped prefab and trusted Configure operation.
- Combat: rifle ammo stayed 29 under fire input during setup. Occupied M2HB ammo
  stayed 99 over 501 injected fire frames; after zone removal it fell to 90.
  Melee attack-tag frames stayed 0 under setup and reached 356 after removal.
  Grenade action input/throw frames stayed 0 with setup. The released grenade
  action selected/raised the grenade but did not produce a completed throw in the
  observation window. Further grenade-in-hand fire/raise controls also produced no
  completed throw (`grenade-control-inconclusive.json`). Grenade suppression is
  therefore inconclusive, not a passing test.
- Admin scope: selected self identity applied, stale identity rejected. AffectsPlayer
  returned true for selected ID 1 and false for unconnected ID 2; this is a predicate
  check, not a second connected-client observation. Rifle ammo stayed 29 during
  the selected freeze. An invalid unauthenticated requester was rejected with
  revision unchanged (3→3). This does not simulate a connected host logging out.
- Mode exit: stock SCR_BaseGameMode/SCR_PlayerController, Lobby2 manager and freeze
  component absent; infantry fire consumed 200→190, matching the preserved
  addon-free baseline in MCP repo `.local/admin-implementation/freeze-input-isolation-progress.json`.
  Stock occupied turret consumed 99→89 after leaving Lobby2. No fresh addon-free
  turret comparison was run in this task.

## Evidence and review

`receipts-first-pass.json` preserves original calls. Its inline-expression
`active` snapshot fields are unreliable (contradictory formatting); the fixture
was corrected to capture independent locals. Use `receipts-second-pass.json`
and `receipts-final-pass.json` for corrected state, eligibility, expiry, scope and
isolation assertions. `soft-death-admin-log.json` and `admin-final-log.json` retain
receipt-correlated native logs and authorization results.

Original MCP captures `freeze-hud.png` and `soft-warning.png` preserve the reused
staging concept's top-center composition, amber accent, readable hierarchy and
single-line guidance. The setup and death clocks are distinguishable. These are
one 3840×2160 Workbench window with a reduced editor viewport, saved at 1920×1080;
no new fullscreen, UI-scale, dark-background or gamepad navigation evidence.

Independent review found an ID-reuse inconsistency in admin scope: clients initially
checked only the selected numeric ID. The final code replicates selected identities
and checks the existing authoritative identity snapshot on both sides. The final
compile and selected-self checks use this correction. Setup owns boundary/HUD
while an overlapping admin freeze retains combat restrictions, preventing two
different staging areas from issuing competing returns.

## Remaining coverage

No new separate-client/dedicated run, real disconnect/ID reuse, JIP, packet-loss,
latency, moving-vehicle freeze, multiple simultaneously connected faction zones,
or complete grenade-throw parity. The implementation uses the existing owner RPC
and native movement paths, but embedded results do not establish those cases.
No microphone/network-voice behavior changed or tested here. Admin selected scope
is the existing scripting request contract, not a new admin-menu picker.

Independent review approved the saved implementation and evidence with these
limits on 2026-10-25, after the identity and combined-guidance corrections.

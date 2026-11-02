# AI debrief validation — 2026-11-02

[Final live AI overview](ai-final.png) retains the existing layout/blur, with a
Players / AI switch on each faction. This capture contains native gameplay data.

## Evidence

Workbench PID 78252, Lobby2 MCP Tests, Arland_Lobby. Native AI rifle pulses used
CharacterControllerComponent.SetFireWeaponWanted after preparing a loaded rifle;
the fixture does not call the stats collector or inject shot counters.

- Two one-second bursts consumed 20 rounds (magazine 29 to 9), recording 20 shots
  and 20 rounds of 545x39 Ball 7N6. Player shots stayed zero and persistent player
  records stayed one. Receipts r6e9ea1c263414533b2fb39da2450a93c,
  r4faeca3e2d5c40078d9f00d494c1a468 and re8da1a1dc4f34ef297b7a631c538aeae.
- Native damage callbacks: AI killed an AI, player killed an AI, AI killed the
  player. Final AI totals: one player kill, one AI kill, two deaths (one by player,
  one by AI), two friendly kills. Receipt r3273d23a9fdf41af913d01b816ac23e1.
- Production match-end sealed the report and returned two fields with the same
  totals/ammunition: rc98e9d997d5e4020ad5843e65ad364e8.
- Twelve native Players/AI switches stayed at three server requests and three
  deliveries: r1d2e065c1ee747baacf56ffab474eaca and
  r3f601a472da248f1b5fe865b419e82f0.
- Postgame native AI firing still consumed ammunition but left the sealed AI
  totals unchanged: ra48c4df4bd6340539a09f6621dd6621e.
- Empty US AI report returned zeros and No ammunition fired:
  ra9d51b0e312b405382e7e1b54c96bada / rb36c069a1c8a4ccea5db62e8c32f1dde.
- Direct player-exclusion guard check passed: r34b6d35194b44c468b2f40a8b241ab38.

Source references (installed catalogue 721cd7b05eb54d3ca3591d9097af381d):
SCR_MuzzleEffectComponent.c 21–24; SCR_OptimizedMuzzleEffectComponent.c 57–60;
BaseProjectileComponent.c 18; BaseMuzzleComponent.c 52; AK74_base.et 115–120;
CharacterControllerComponent.c 92. These are declarations/source; the rifle
ammunition delta above establishes observed callback behavior on the listen host.

## Scope and remaining coverage

No extra AI counters/ammunition are serialized. Existing session death records
are unchanged. Counts are per native faction at the event, not individual AI.
Hand-thrown grenades and movement/medical/radio are not collected or displayed.
Weapons without either supported muzzle-effect callback are not covered. Other
weapon families, headless dedicated-server callbacks, remote/128-client load,
addon-free mode-transition baseline, gamepad and other DPI scales were not tested.
The callbacks preserve super behavior and route through active-mode authority
checks; this does not substitute for an addon-free runtime baseline.

The independent reviewer approved the UI and identified shared death-limit
truncation reporting; the final patch propagates that condition to AI partial
status separately from persistence failures.

Final verification: Reload 2979af6745f94f988a8cf23238cf3529 compiled successfully
(console logs_2026-11-01_22-52-25, cycle 22154–23122). The isolated partial-state
fixture returned partialVisible=1 and two report fields, then restored the real
store (rea4159a202ca41cdac968a72b2f49b47). Final-source native firing/death/end
retest returned 10 shots, 10 rounds, one AI death and zero player shots
(rf1912ba0d7694742ba180aaf8c2e0ff8). No new script errors through line 23424;
the test world's existing missing-loadout-manager error remains. The independent
reviewer approved the final patch after this correction.

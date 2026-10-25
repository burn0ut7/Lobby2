# AO first-entry enrollment

2026-10-25, Workbench1.8.0.13, Lobby2McpTests Empty_Lobby embedded host. Latest user clarification: a unit is exempt until it enters the AO's effective area. Implemented using existing Permits(position), which respects inversion. Setup retains existing start-inside enrollment. No new UI layout or engine APIs; mode-owned entity/system only.

Per-AO per-character enrollment is recorded on authority before freeze/filter evaluation, only while enabled and permitted. AppliesTo gates both gameplay and existing wall audience. Enrollment persists through settings/disable/freeze and is not inherited by replacement characters. Per-unit records live with the zone, no added tick/listener. Existing filters remain live additional conditions. Normal/inverted area membership uses existing installed Math2D.IsPointInPolygonXZ and Permits implementation; no replacement geometry system. Saved-code style and diff whitespace checks passed.

Runtime evidence in receipts.json:
- Normal AO centered40m, half10; initial player0m remains alive after3sec with2sec grace, clocks0/no HUD. Setup centered30m similarly active0. AO wall enabled: audience0/mesh0, including player25m only5m from edge.
- Move35m first enters permitted area: AO wall audience1/mesh visible. Exit25m creates19.560 countdown. Setup remains inactive despite traversal into its area. Setting changes preserve AO enrollment.
- Inverted AOFixture2 centered35m, half10: replacement soldier starts34.47m inside forbidden polygon; survives2.6sec with2sec grace, no HUD/clocks (r2160afe408054aa98124fd78f0b818d7). Move55m first enters permitted exterior; move35m then gives119.598sec warning (r32682f8926f74126b481532736221302).
- Normal hard AOFixture3 starts same soldier inside35m; moving60m returns35m (rd2599252191e4be482284ede82013663).
- Setup starting inside remains active; leaving produces RETURN TO SETUP (r3e928f9d48d5411795a1df7058f63703).
- Old soldier enrolled in AOFixture1 and died on expiry. Admin-authorized production heal recreated a different soldier. Enabling same inverted AO while new soldier inside forbidden polygon leaves it exempt past2sec grace (r2021f8eab0434facb4f137a22ab6ffd7): enrollment is not inherited across replacement.
- Enrolled normal AO disabled/re-enabled while outside still warns (rc49500869623409bbdaf1cf493ceed2b).
- Final exempt.png: fresh distant AO and outside setup, both2sec grace, alive past expiry with all HUD roots absent.

Preserved failed test arrangements: first inverted fixture was placed with character exactly on polygon edge, so that initial baseline is not used for inversion acceptance. While between calls its20sec warning expired; subsequent calls through r075ef233d1a844e08871a0348f1657a2 acted on a spectator and are excluded from gameplay acceptance. Existing production admin heal restored the real slot, and all relevant checks were rerun on the new soldier. Switching soft AO to hard while already outside had no saved hard-safe pose and correctly logged unavailable; later hard test first established safe history. Raw logs retain fixture/plugin/resource/map/loadout errors. No compile/movement/enrollment script exception observed.

Limits: embedded host only; no dedicated/JIP network run, no explicit admin-freeze regression in this slice. Shared vehicle can still be returned for an enrolled occupant, carrying exempt passengers; their own soft timers/enrollment remain independent. AI-only units are unchanged by these player-control systems. Per-zone first-entry observation occurs on the existing100ms GAME evaluation.
## Review correction: occupied vehicle enrollment

The independent review identified a seat/body mismatch. Enrollment now resolves the same effective mover as enforcement: vehicle origin for occupants, character origin on foot; membership remains keyed by character.

Fresh embedded session PID 20028 compiled and exercised the correction. At the 20 m edge, vehicle offset x=20.300 was outside while driver x=19.646 was inside. After more than the 2-second grace the driver remained alive, with no HUD, clocks or wall audience (r8f9b4d55965940df8813353fa302b128, r0358f028c59a4d26b3d2b51e9e6b4e8d). Moving the vehicle to x=19 enrolled it: wall audience=1 (r89d4890f8862440e880c6a4e83ea3590). Moving back to x=20.300 then produced the AO warning and one clock, remaining=119.325 (rc3b3eee8e9c2464d8e21d13cbbc9bf7e). Arrangement readbacks reported damaged=0, fades=0, alive=1. Returned vehicle inside after testing. Raw receipts include both sessions; vehicle-errors.json captures the final session error log.

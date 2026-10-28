# Slide-out edge regression

The shared `GC_SlideOutComponent` hit test used an exclusive fractional right/bottom bound against integer pointer coordinates. At the expanded screen edge this excluded the edge pixel; beginning to retract could put that same stationary pointer back inside. Hit testing now includes a one-physical-pixel tolerance on the bounds. Layout dimensions and animation timing are unchanged.

2026-10-28, visible Workbench 8324 / Lobby2 MCP Tests / Arland_Lobby:

- Fresh reload `0851dd90ec4c45509c725bd283a4c2f2` completed. Read the fresh 12:47:04–11 cycle in `logs_2026-10-28_01-58-33/console.log` directly because automatic console discovery was ambiguous: Game loaded without script errors.
- Optional `GC_TestSlideOutEdge(name, action)` exercises both `SpectatorPlayerSlideOut` and `SpectatorOptionsSlideOut`. Open, allow animation to finish, probe, then leave. Rendered right edge was 2409; the original predicate excluded it and the test closed both panels.
- With the fix, 120 synchronous production hit-test/hover/animation ticks stayed at progress 1 with zero target reversals. Player receipt `ra6ea1f82426b438b87ab287c701ded92`; options `r5499f33fba074dd0bc9220e5b942f2be`. Three pixels beyond the edge remained outside; moving the test coordinate away closed both to progress 0. No receipt-correlated script exceptions.
- MCP screenshots in tooling repo `.temp/local/evidence/admin-scope-20261024/`: `1793206065094518700.png` collapsed, `1793206072400025900.png` expanded.

This is deterministic production hit-testing on actual widget geometry, not physical OS mouse movement or 120 rendered frames. Other viewport/UI scales have not been exercised. The one-pixel allowance is applied after layout scaling, in screen coordinates.

Follow-up: a frame-separated 16 ms probe now starts collapsed, opens at the fixed edge coordinate, holds through tick 120, and leaves through tick 150. Both drawers reported minimum hold progress 1, exactly one target change (initial opening), and final progress 0. Receipts: r5ec901d186ec4fcda74129c8e87fb53f and rb1d3837a5e984d849242b30b831615ff. Reload e63e6109fd7845cca5c37ed033cfe043 completed with a fresh successful compile. The user also confirmed the fix works; independent reviewer approved.

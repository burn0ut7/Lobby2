# Slot selection during a match

Successful character assignment closes the owning player's slotting menu during
GAME. Lobby and briefing selection remain browsable. The existing authority
checks and character-assignment flow are unchanged; rejected requests do not
close the menu, and clicking an already selected role still releases it.

## Verification — 2026-10-29

Embedded Arland, Lobby2 MCP Tests, Workbench PID 17588. Reload
`ef0a7f960a7c4dc383bc540360d7d209` completed without script compilation errors.
Existing world GUID and dedicated-plugin diagnostics remain; runtime also logs
the scenario's missing loadout manager. This is not an error-free world claim.

Using the visible slot button's production handler (`GC_TestSelectSlot`):

- SLOTTING: select slot 1; spectator remains and slotting stays on top
  (`rf4d332b55d3c4713b7d8f3d6c542aaca`).
- BRIEFING: select slot 2; spectator remains and slotting stays on top
  (`r91823af0a6a44f8d8564e8e47b2ccaed`).
- GAME: reopen slotting and switch 2 to 1; slotting disappears, PLAYING is
  observed, and the controlled entity matches slot 1
  (`r67ed2c82b26c405685c2eac0e0790ca8`, `r2bf20e679c2c41f59040af714c26e85a`).
- Release slot 1; spectator mode returns. Reopen slotting and select slot 2;
  the menu disappears and control matches slot 2
  (`r25d0e7a0d9b94947b87056d8889c8d28`, `r31dc486dc1e542f49bdf22457977de34`).
- A remote player claims slot 1 through the production owner RPC. A host
  request for that occupied slot is rejected by authority; slot 2 and the open
  slotting menu remain (`r5810b84dd1ba498383a6d57fce00695b`). Console session
  `logs_2026-10-29_16-14-34`, line 25729 records the rejection.

Actual MCP captures are retained locally in the MCP workspace at
`.local/slot-auto-entry/briefing.png` and `game.png`. Remote menu closure and
dedicated-server presentation were not independently exercised. No shared
base-game override or layout changed.

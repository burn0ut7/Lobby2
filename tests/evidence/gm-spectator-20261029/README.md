# Game Master death camera regression

Verified in Workbench 1.8.0.13, PID 56348, Lobby2 MCP Tests, `$Lobby2:worlds/examples/arland/Arland_Lobby.ent`, embedded host player 1. Session `logs_2026-10-29_19-03-12`.

## Reproduction and cause

Advance via `GC_LobbyFreezeComponent.GC_TestFreezeAdvance` to SLOTTING, assign self to slot 1 with `GC_LobbyManager.GC_TestAdminAssignSelf`, then advance through BRIEFING to GAME. Open native Game Master with `GC_TestOpenGameMaster`, kill the controlled character with `GC_TestSpectatorPlayerJump("die")`, and close with `GC_TestCloseGameMaster`.

The original close queued `ReconcileSpectatorPresentation` without its three vectors. The native call queue does not apply the method's default parameters: log line 9421 reports `Not enough parameters, expected '3' got '0'`. Both before and after closing, the owner reported spectator intent and a spectator body but no spectator camera. `before.json` and `before.png` record the failure.

## Change and verification

Pass the three zero vectors explicitly to the existing deferred call. No new polling, ownership state, shared overrides, or UI layout changes.

Reload operation `8b0bbe733872411483a351f78766577d` completed; no script compile errors. The test world still reports resource GUID warnings/errors, an unknown dedicated-server plugin class, and a missing loadout manager; these did not prevent the exercised flow. No callback parameter error recurred.

After Reload, repeat the production transitions above using the existing optional test helpers. Owner observations use `GC_TestPeerCommand(1, token, "snapshot", "")`, then `GC_TestPeerReport(token)` after the RPC completes:

| Token | Situation | Result |
| --- | --- | --- |
| fixed-alive | Open and close GM while alive | Playing; spectator requested=0, body=0, camera=0 |
| fixed-dead-in-gm | Die while GM remains open | Spectator requested=1, body=1, camera=0 |
| fixed-after-gm | Close GM after death | Spectator requested=1, body=1, camera exists=1 and current=1 |
| fixed-reopen | Reopen GM while spectating | Spectator camera suspended, exists=0 |
| fixed-reclose | Close GM again | Spectator camera exists=1 and current=1 |

`after.json` contains the live owner reports; `after.png` shows the restored spectator viewport, unit markers, and spectator menu handle. Screenshots are native MCP captures at 1920x1084 of the same 3841x2168 Workbench window. No separate remote client was exercised for this owner-local callback correction. `git diff --check` passed.

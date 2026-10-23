# Slot reservations

## Admin workflow

1. Sign in as an administrator and open the normal slotting screen.
2. Right-click the actual slot and choose **Reserve slot...**. The dialog identifies its team, group, slot number and role; it has no second slot list.
3. Enter the player's Arma UID (UUID, not a Steam ID), optionally a display name, or choose a connected player. Select **Reserve slot**.
4. An offline player is assigned automatically when they join, including preview, slotting and briefing. A connected player is assigned immediately. Players remain free to change slots under the normal phase rules.
5. Use **Clear reservation** for an initial reservation. Existing player claims use the ordinary admin slot actions.

Reservations may be edited before the match. Initial reservations last until joined, cleared or the session ends. After joining, disconnects use the game mode's normal reconnect timeout (120 seconds by default). Joining a KIA reservation never resurrects the character.

## Import and export

Open **Admin Tools → Slot import / export**. Export the roster, copy the text, fill in `armaUid` and optional `playerName`, then paste and import it. This panel transfers assignments; individual slots are selected in the slotting screen.

The version 1 document contains `version`, `world`, and `slots`. Each slot is a string-valued record containing:

| Fields | Meaning |
| --- | --- |
| `slotKey`, `groupKey` | Stable authored identities used for matching |
| `slotNumber`, `unitIndex` | Session display number and authored unit index |
| `teamKey`, `teamName` | Base-game faction key and translated name |
| `groupName`, `groupPrefab`, `groupPosition` | Group presentation and authored provenance |
| `rolePrefab`, `roleNameKey`, `roleName` | Exact character prefab and role label |
| `armaUid`, `playerName` | Persistent player UUID and display name |

Keep the exported world and keys unchanged. Names and session slot numbers are descriptive, not matching keys. Authored group placement, faction, prefab or unit-list changes can invalidate old exports. Runtime-created slots without an authored identity are excluded and counted in the result. Both limits apply: at most 128 rows and 32 KiB per document. Metadata usually reaches the byte limit first (the 42-slot fixture uses about 21 KiB), so this is not a guarantee of full 128-slot roster support. Export includes the whole authored roster and fails explicitly if it exceeds either limit; this UI does not export subsets.

Imports validate every row before applying any changes. Wrong scenarios/versions, unknown or repeated slots, invalid or duplicate UIDs, unavailable slots and conflicting claims reject the import. Blank UIDs leave slots unchanged; matching claims are no-ops. Clear conflicting initial reservations or use ordinary admin move/remove actions before importing. No automatic disk persistence or website connection is included.

## Verification — 2026-10-23

Executed through Reforger MCP in the optional `McpTestAddon`, using production context actions, buttons and owner-controller RPCs. Workbench process 13228 hosted `AdminMultiplayer.ent`; separate native Peer Tool clients exercised actual replication and server authorization. No test permissions were granted.

- Script compilation succeeded: receipt `1792787789149575700-a7dbd8cbc7194cf0bd4f20d3f0701be8`.
- Actual slot context menu, manual UID/name, connected-player picker, reserve/clear and ordinary slot changes were exercised. Final owner reply supplied the current server claim without a false stale-claim warning.
- Export/import through the actual transfer panel round-tripped all 42 authored slots (about 21 KiB). All 42 slot/group keys matched after a world restart; importing the earlier export assigned the connected host.
- Invalid final rows and duplicate UIDs rejected the entire import without partial changes. Wrong world/version, invalid/zero UID, multiline name, missing slot key, stale claim, idempotency and blank-UID preservation were checked.
- A separate client joined an offline reservation automatically in `PREGAME`, with native group membership and no assignment pin, then switched slots normally. A later join in `BRIEFING` also assigned automatically. JIP replicated another offline reservation's initial flag/name.
- An unauthenticated remote client attempted the production reservation RPC. The server rejected it without changing the slot and emitted `rejected-not-logged-in` through the central admin logger (receipt `1792788290060051100-3e2aaf37f4b3422286b9bdc79efa62c5`).
- Removing the host's actual admin role closed the open reservation dialog. A subsequent production request was denied and left the target slot free (receipt `1792788521425679500-2607dcac6642472b8c4199edea803316`).
- A reserved character killed in `GAME` remained KIA after its player joined; the peer possessed a spectator body with its spectator camera active. Reserve/import were rejected during `GAME`; export remained available.
- An untouched initial reservation remained held for more than the default reconnect timeout and had no reconnect timer.

Detailed request/response receipts, assertions and screenshots are in the tools workspace `.local/slot-reservations`. Source review and runtime evidence are separate: the test world emits existing resource/map/connection diagnostics, and Workbench produced a thermal-resource leak assertion on earlier reloads. These are preserved in the receipts rather than described as a clean engine run.

Final rendered evidence after restarting Workbench to reload the saved layouts: [reservation dialog](../Art/References/slot-reservations/implemented-dialog.png) and [transfer panel](../Art/References/slot-reservations/implemented-transfer.png). An independent reviewer approved the code, evidence and final screens. The saved scripts passed the Reforger readability review and the task's tracked diffs passed `git diff --check`; existing externally referenced RPC and serialization names were preserved.

Remaining coverage: remote authenticated bulk transfer (the test server has no admin password configured), controller navigation and constrained viewport sizes. No shared global gameplay override was added by this slice.

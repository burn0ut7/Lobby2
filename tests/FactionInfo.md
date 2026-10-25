# Faction Get info

Right-click any faction card and choose **Get info**. Every player can open it,
including for a locked or unselected faction, without changing their selection.
The native read-only dialog shows that faction's total slots, the server's
connected-player count, and each faction's **current / needed players**.

Needed means the total target allocation, not additional recruits. Targets use
all connected players (including unassigned spectators), total slot capacities
(including locks), and the existing deterministic proportional balance policy.
For example, 20 and 40 slots with 30 connected players gives targets 10 and 20.
Targets are informative even when balance enforcement is off. Current players
come from connected native faction membership, not slot reservations. Values
are a snapshot at opening; reopen to refresh. Close or Back returns to the lobby.

## Verification — 2026-10-25

Workbench 1.8.0.13, optional McpTestAddon, embedded Empty_Lobby host:

- Script reload passed after moving the new test methods into the existing
  test class that owns their helpers. The initial standalone modded test class
  could not resolve those helpers; that failure is not a passing compile.
- The production right-click handler exposed Get info for admins and, after
  native role removal, ordinary players. The real context action button opened
  the native dialog. Both factions were inspected without selecting them.
- With 10/32 total slots, all locked, and one connected spectator, the dialog
  reported Soviet `0 / 0`, US `0 / 1`. Unlocking preserved those targets.
- Selecting slot 6 then reopening the US dialog reported Soviet `1 / 0`,
  US `0 / 1`, with US total slots 32. Leaving the slot and enabling balance
  produced `0 / 0` and `0 / 1` for the ordinary player.
- Invalid faction execution was ineligible and left the current dialog alone.
- Native Close button and native MenuBack action delivery both dismissed it;
  subsequent snapshots showed no dialog and the original slotting screen.
- Existing balance-policy regression cases passed. Additional direct calls to
  that same production policy passed 20/40 slots with 30 players => 10/20,
  zero players, zero capacity, and a single faction. This is not a 30-client test.

Local original MCP receipts, correlated logs, and screenshots are saved under
`Art/References/faction-info/`. Active visual reference: `concept-v1.png`.
Actual `dialog.png` and `ordinary-player.png` preserve the compact dark dialog,
selected-faction title, multiline data and single dismissal action. Native
typography, panel size and bottom-left ESC/Close footer are intentional native
adaptations; no new layout/input implementation was introduced.

Coverage is one 3840x2160 desktop viewport, with captures saved at 1920x1080.
Remote/JIP, physical mouse/gamepad input, alternate resolution, long faction
names and many-faction scrolling were not exercised. The test world retained
resource/property, missing map/loadout-manager, and icon-mask errors; runtime
dialog receipts reported no script exception. The feature adds no shared
override, mutation RPC, polling loop or retained roster state.

Final production reload passed with zero errors (34 warnings), with only core,
ArmaReforger and Lobby2 loaded. Normal Arland was reopened with Play stopped.
Final evidence: `Art/References/faction-info/final-reload.json`.

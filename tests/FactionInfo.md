# Faction Get info

Right-click any faction card and choose **Get info**. Every player can use it,
including for a locked or unselected faction. A standard game hint shows the
selected faction's total slots, connected-player count, and each faction's
**current / needed players**. It expires after 15 seconds; use the stock Hide
control to dismiss it sooner. Reopen to refresh the snapshot.

Needed means the total target allocation, not additional recruits. Targets use
all connected players (including unassigned spectators), total slot capacities
(including locks), and the existing deterministic proportional balance policy.
For example, 20 and 40 slots with 30 connected players gives targets 10 and 20.
Current players come from connected native faction membership, not reservations.
Targets remain informative when balance enforcement is off.

## Verification — 2026-10-25

Workbench 1.8.0.13, optional McpTestAddon, embedded Empty_Lobby host:

- Production right-click and Get info handlers displayed the stock hint for an
  admin and an ordinary player after native role removal. No dialog opened.
- With 10/32 total slots, all locked, and one connected spectator, it reported
  Soviet `0 / 0`, US `0 / 1`. Opening US info replaced the title and total (32).
- Switching the displayed faction remained possible while the hint was shown.
- Invalid faction execution was ineligible and left the existing hint unchanged.
- Script reload passed. Initial calls without a menu hint component delivered
  data but no visible hint; the slotting layout now embeds stock Hint.layout.
  The updated layout became visible after reopening the world and reloading.
- Earlier data-policy verification passed 20/40 slots and 30 players => 10/20,
  zero players, zero capacity, single faction, and connected faction membership
  after slot selection. This was not a 30-client test.

The requested stock hint supersedes the earlier dialog concept and captures.
Current appearance evidence is `Art/References/faction-info/hint.png`; receipts
are in `hint-runtime.json`. The standard hint owns rendering, Hide input,
settings, timeout, and subscription cleanup. No custom layout resource, dialog
configuration, shared override, polling loop or retained roster was added.
The slotting screen references the existing stock hint resource so it can render
above the menu; it respects the player's normal hint visibility setting.

Coverage is embedded host at 3840x2160 (capture 1920x1080). Remote/JIP,
physical mouse/gamepad, other resolutions, long names and many factions were
not exercised. The test world retains unrelated map/loadout/resource errors.

Automatic expiry returned hint=absent after the 15-second interval. The test
button-dispatch helper did not establish dismissal through the stock Hide
control; its immediate snapshot still showed the hint. Manual Hide input remains
unverified, and is owned by the unchanged base-game component.

Final production script reload passed with zero errors (34 warnings), with
only core, ArmaReforger and Lobby2 loaded. Normal Arland was reopened with Play
stopped. Evidence: Art/References/faction-info/hint-final-reload.json.

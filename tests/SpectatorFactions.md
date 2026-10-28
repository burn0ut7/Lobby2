# Spectator faction roster

The spectator drawer adds Factions alongside Voice and Players. Other uses of the
shared drawer keep the new tab hidden. All/Alive/Dead applies to connected players
in registered factions; dead means the authoritative slot is KIA. Unassigned
spectators are excluded. The colored strip uses each faction's filtered member
count as its native layout weight. Count buttons remain equal-width and toggle a
single-faction list filter; clicking the selected faction clears that filter.
The strip/counts retain the whole comparison while a faction is selected.

Uses existing player rows, group callsigns, faction colors and role icons.
Player/slot/affiliation, voice-assignment and role events coalesce refreshes.
Inactive tabs do not rebuild; no per-frame or network polling was added.

## Verification — 2026-10-28

Embedded Workbench 1.8.0.13, optional MCP test addon, one real player assigned to
USSR slot 1. `GC_TestFactionRoster("open")` opens the production spectator menu
for presentation testing. `dead`/`alive` deliberately change that disposable
host slot's presentation state; restore it after testing. `ratio` installs only
two temporary visual segments with weights 5/10; `refresh` restores real data.

Verified native tab/filter clicks, empty Dead list while alive, slot KIA event
adding the player to Dead and removing them from Alive, faction filtering/reset,
Voice/Players/Factions switches, and 1:2 strip geometry (215.806/431.612 pixels).
Dead label survived native admin row selection. Real deafen toggle and existing
voice/role presentation events refreshed the open roster. Restored alive state,
hearing and production ratio data. Existing missing-LoadoutManager world error
occurs during slot selection; no new UI errors in the filter checks.

Final MCP reload `37b593bd8c644c92b35833556bd34e9a` completed. Automatic log ownership
association was unavailable; the existing process console's 11:41:06–11:41:13
reload cycle was read directly and had no script compilation errors. Current
captures and receipts reside in the MCP workspace local evidence directory
`admin-scope-20261024`, including `1793202142215216700.png` (selected dead/deafened)
and `1793202190431408900.png` (restored alive). Independent visual/code review
approved after two live-update fixes. Dedicated peers, fifteen real connected
players and long-list scrolling were not exercised.

UI reference and design brief: `Art/References/spectator-factions/`.
Installed source leads: catalogue c032908fa5cc4428b257394a5dd4644d,
Core generated UI/LayoutSlot.c (SetSizeMode/SetFillWeight), Visual/Color.c (Copy),
and Game UI/Components/WidgetLibrary/Button/SCR_ButtonBaseComponent.c:279
(SetBackgroundColors). Existing player-row resource remains authoritative for
role icons and context interactions.

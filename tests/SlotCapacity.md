# Lock-aware lobby capacity

Faction and group counters show connected occupants / slots excluding only
admin-locked slots. With no unlocked slots, the counter shows `Locked` without
numbers. Reserved, occupied and KIA slots remain capacity; balance quotas do not
cap the displayed denominator. Selection permissions and balance calculations
are unchanged. Counts refresh through the existing slot/faction events.

Reference: installed ReforgerLobbyCommunity `PS_FactionSelector.UpdateCounter`
and `PS_RolesGroup.UpdateCustomName` subtract locked slots and replace zero
capacity with their localized Locked label. Lobby2 uses its existing English UI
style and separate group-count widget; the reference's optional ratio counter
is outside this lock-only change.

## Verification — 2026-10-25

Workbench 1.8.0.13, optional McpTestAddon, embedded host, Empty_Lobby world:

- Fresh script reload succeeded. Initial ten-slot Soviet faction and 32-slot
  US faction displayed `Locked`; every visible group also displayed `Locked`.
- Production unlock-all restored `0 / 10` and `0 / 32`.
- Production individual locks on slots 3 and 4 changed the Soviet faction to
  `0 / 8` and their four-slot group to `0 / 2`, without reopening the menu.
- Reserving slot 5 left both denominators unchanged; the slot became reserved
  and unselectable. Enabling balance likewise left the counters unchanged.
- Selecting slot 6 changed the counters to `1 / 8` and `1 / 2`.
- Unlocking slots 3 and 4 restored `1 / 10` and `1 / 4`.
- Lock-all retained occupied/reserved capacity (`1 / 2`) while the empty US
  faction and empty Soviet groups returned to `Locked`.

Existing optional fixtures exercised production controller/action paths;
receipt-correlated central logs reported applied actions. An initial balance
request with invalid mode -1 was rejected; the subsequent valid mode 0 applied.
MCP captures at 3840x2160 (saved at 1920x1080) showed readable Locked and 0 / 8
labels in the existing layout. Local evidence is in the MCP repository's
`.local/slot-capacity/`: runtime.json, audit.json, locked.png, partial-locks.png.

Coverage is embedded-host only, not remote replication/JIP or physical input.
KIA inclusion follows the same lock-only predicate but was not exercised live.
The test world logged existing resource/property errors, missing map/loadout
managers, and an icon-mask error; this is not a clean-world health claim.
No shared global override, authority handler, replication payload or layout was
changed. Test state is discarded on stopping Play.

Final saved scripts reloaded in normal Lobby2 with zero errors in that reload
cycle (34 warnings). Only core, ArmaReforger and Lobby2 remained loaded; the
original Arland world was restored with Play stopped. Final compile and addon
inventory evidence is `final-reload.json` alongside the runtime evidence.

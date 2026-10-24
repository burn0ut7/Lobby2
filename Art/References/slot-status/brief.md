# Slot status icons

Small revision of the existing slot row; retain the current role/name columns,
40-unit row height, faction accent and native 32-unit status icon area. Reuse the
existing lobby composition shown in ../faction-ratio/runtime/icons-visible-ratio-off.png.
No new screen, panel, control or generated asset.

| Element | Player purpose | Owner | Decision |
| --- | --- | --- | --- |
| Role icon/name | Identify the role | Authored role info | Keep |
| Status icon | Identify locked, KIA, held or disconnected state | Replicated slot | Replace status words |
| Player name | Identify occupant or intended recipient | Slot cached name/native admin presentation | Keep without status prefix/suffix |
| Empty available row | See an open role | Slot | No status icon |

Stock icons in {2EFEA2AF1F38E7F0}icons_wrapperUI-64.imageset: server-locked,
death (skull), time (reservation/held), disconnection, player. White silhouettes,
existing player-name colors, no new color-only distinction. Existing role hover,
context menu and click rules retain their existing handlers. Each row has at most
one status icon. Priority: KIA skull, connected player, initial reservation clock,
disconnected symbol, unoccupied reserved clock, empty-slot admin padlock.
Only FREE slots without a player, identity claim or pending reservation are
lockable. Both single-slot and bulk actions enforce this on the server. Claim and
lifecycle transitions discard the empty slot lock before publishing changes.
Names remain separate from the status. The header lock control considers only
eligible empty slots and is disabled when none remain.

Validation uses the optional test addon's detached display copies. This gallery
exercises the production widget but is not evidence of authoritative reservation,
death or disconnection transitions. Restore canonical bindings after capture.

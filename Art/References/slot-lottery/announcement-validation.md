# Lottery announcement verification

2026-10-24, optional Lobby2McpTests AdminMultiplayer world, embedded host admin,
3840x2160 Workbench window (MCP captures scaled to 1920x1080).

Native script reload completed successfully. Production context actions started
single slot 1, group containing slot 3, and faction containing slot 11.
Snapshots reported respectively one, four, and 32 slots, with labels
`Soviet Army - Kompas-21 - Senior Rifleman`, `Soviet Army - Buran-32`, and `US Army`.
single-announcement.png, group-announcement-v2.png, and faction-announcement.png
show explicit scope headings, faction/group/role detail,
and a blank line before the /join and context-menu instructions, without clipping.
The faction wording comes from the scenario's native display name.

MCP invocation receipts: r51750d5e252f4cf69173a3fa80733683 (single),
rdf7537997efd4757a6bbb44aeb090802 (group),
r2837fc61e091452e8b8fed55c92318f6 (faction),
r0da716a247ff41b49d4baf4fa7a4a90c (concurrent snapshot).
Workbench log session logs_2026-10-24_07-33-32 confirms each start and receipt.
The fixture still emits its existing missing SCR_MapEntity diagnostic and resource
warnings; these are not a zero-error scenario claim. Remote clients, alternate
languages, constrained viewports, and physical input were not retested for this
wording change. Earlier lottery behavior evidence remains in validation.md.

Independent review corrected label construction to use ResolveSlotGroup, matching
slot collection when a native group pointer is temporarily stale. Final production
script reload passed after that correction; the captured normal-group presentation
is unchanged. The transient stale-pointer case was source-reviewed, not reproduced.

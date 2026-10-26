# Gameplay map briefing

## Right-anchored index refinement

The gameplay drawer now fills its columns from the right: the index stays at the
right edge and the selected document expands leftward. Document padding mirrors
the column order. Only this map layout hides the handle background's right cap,
leaving a flat attachment edge and rounded outer left corners.

Verified through MCP in the same Arland test project on 2026-10-25:
reload `ac061631b4024e70bdd7cac39d16f65f` succeeded; compact and expanded index
rectangles both begin at x=2246.66px with width=358.519px. Expanded document starts
at x=1448.53px with width=790.305px, to the index's left. Selection, deselection,
pinning and scrolling were exercised through existing handlers. Geometry receipts:
`r512bb62f9d534462b20901ab457659f1`, `r44ad60ba6ac64358977e3d9ac7764cfa`,
`rafe2b46f8b01432cbd4bb31ff5a82aaf`. Pin receipt
`rfa667d2fac9347f7817d6adcd1f9aacc` has correlated log lines 4076–4077.
Local captures: `Art/References/map-briefing/reversed-flat-handle.png` and
`reversed-scrolled.png`. Physical input/alternate-scale limits below still apply.

## Original integration

The native fullscreen gameplay map creates one Lobby2-owned right-edge drawer.
It reuses the briefing view and slide-out component: proximity reveals the arrow,
hover/focus opens the drawer, and the arrow pins/unpins it. Page selection expands
the compact index to the existing document width. Closing the map destroys the
drawer; reopening starts collapsed and unpinned. The drawer does not activate the
briefing menu's D-toggle context.

Use `GC_TestMapBriefing(action)` on the optional test addon's `GC_LobbyManager`:
`snapshot`, `proximity`, `focus`, `leave`, `pin`. These inspect or invoke production
handlers; they do not emulate physical mouse movement. Existing helpers open the
native inventory map (`GC_TestAOMapSurface("gameplay", true/false)`), select a page
(`GC_TestOpenBriefing`), inspect widget hit targets (`GC_TestMapHit`), and scroll
the existing widgets (`GC_TestScroll`).

Validated in embedded Workbench 1.8.0.13, Arland, 2026-10-25:

- Successful native script reload after final changes.
- Drawer absent from Preview and Briefing menus.
- Initial gameplay map: collapsed, unpinned, content disabled, compact width.
- Proximity calculation: inside 1, near 0.73283, far 0.
- Focus opens; Situation selection displays the existing document.
- Pin remains open after focus leaves and renders the amber arrow.
- Unpin preserves expansion while the actual pointer remains over content.
- Earlier leave test settled collapsed with zero handle opacity.
- Existing index scroll reaches the final Scenario entry.
- Native map close/reopen returns a fresh collapsed drawer.
- Handle hit target is enabled; D toggle and its context are inactive.

Independent review caught and corrected a disabled-width-override regression.
`IsWidthOverride()` now gates initial child sizing. The existing spectator voice
drawer was exercised in BRIEFING and retained its authored 584-unit width
(receipt `re53e024869ee4b01a383c69099b92b8b`). The gameplay briefing was then
rechecked at 343 units collapsed and 949.2 units expanded/pinned.

Final reload operation: `95888f6366b04da38186599daa675787`.
Pin receipt: `r907380562bf44c118d469656f064d465`, correlated at console lines
2786–2787 in `logs_2026-10-25_20-20-49/console.log`.
Reopen snapshot: `r3a5f4a80d7234f22829554877e42ac4d`.
Local concept, captures, detailed receipts and preserved failures are under
`Art/References/map-briefing/`, excluded from Git by the repository's Art rule.

Coverage limits: physical mouse click/travel and motion timing, gamepad input,
alternate viewport/UI scales, separate peers/JIP and addon-free isolation were
not tested. Runtime screenshots use a 2613x1408 game viewport inside Workbench.
Existing world/resource/loadout/icon errors remain in the session; a GUI `name`
parse message at map opening is unlocalized. No blanket clean-log claim is made.

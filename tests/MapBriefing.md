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
- Original unpin behavior preserved hover expansion; superseded by the explicit-close fix below.
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

## Wheel ownership regression

The map cursor's wheel action listeners run independently of widget consumption.
`GC_MapCursorModule_Briefing` guards only those two listeners using the local
controller's live drawer bounds and active Lobby2 ownership. Digital/gamepad
zoom and inherited wheel behavior outside the drawer remain unchanged. No
shared input state or subscriptions are retained.

`GC_TestMapBriefingWheel(targetName, wheel)` in the optional test addon supplies
synthetic pointer coordinates to the production map handlers and calls the
existing scroll handler. Targets include `BriefingTitlesScroll`,
`BriefingSectionScroll`, `SlideOutHandle`, and `map`; directions are -1 and 1.
This checks handler behavior, not physical event routing.

Validated 2026-10-25, Workbench 1.8.0.13, reload
`18143574133b4cf9a90a69b5528729ce`:

- Titles move 0 -> 83.437 -> 0 pixels with zoom fixed at 18.0659
  (`r6c853ae012544ec4bc51e467e83af46f`, `rae1538502e164c63b414eadf236d8612`).
- Visible Situation body consumes both directions with no overflow and no zoom
  (`r79f1191c91494e45b86c015149d34ef1`, `rdd5b57354bad408eb5df52f5cbd331e6`).
- At list bottom, scroll remains 534.844 and zoom stays fixed
  (`rc1b77c70a0584b0688fb850b03c88cde`). Handle blocks both directions.
- Outside the drawer both zoom directions change the target zoom; collapsed
  drawer also allows map zoom (`r05fcae782bb34ed282f356cd4eb752b2`).
- Native map close/reopen allows zoom (`r514ae5d4d6874e1f9b50779b3f5f4a9b`).

The initial reload found a reserved test variable name; the first runtime attempt
found missing BaseContainerProps registration on the modded cursor class. Both
were corrected before the passing cycle. Latest-cycle errors were inspected;
existing world/resource/loadout/icon/GUI errors remain, with no cursor exception.
Physical wheel delivery, overflowing document-body content, and unrelated-mode
comparison against an addon-free baseline remain unverified. The shared override
has an explicit active-mode guard but that alone does not establish coexistence.

## Explicit close regression

Unpinning now requests collapse regardless of hover or retained button focus.
Hover opening is suppressed until collapse finishes and the pointer is outside
the drawer. Focus events generated during that close are ignored; a later fresh
hover/focus or explicit pin can reopen it. This is shared slide-out behavior,
including the voice drawer, rather than a briefing-only exception.

Validated through MCP in embedded Workbench 1.8.0.13 on 2026-10-25, reload
`5201523775514cb897faf4e0a04e8bda`. The `closeFocused` test action invokes the
production focus/pin/unpin handlers and another focus event in one call:

- Close starts with target=0 despite focus (`r4ba48bdeada14db0b12d1b949fe4273a`).
- Settles at progress=0, target=0, opacity=0, content disabled
  (`r8dcbf75cb8d34470bce69b251c63d658`).
- Fresh focus reopens fully (`r657b991867b64f5db0d08d7ee997ff31`).
- Pin persists after focus leaves (`rdcdb7e2d9f1545d3959f948763bfec2a`).
- Repeated explicit close settles fully hidden (`rb911ddc846424567bd91fafaa9adf8b9`).

Physical mouse movement/click delivery and the voice drawer were not exercised
in this cycle; the checks above invoke the shared production handlers.

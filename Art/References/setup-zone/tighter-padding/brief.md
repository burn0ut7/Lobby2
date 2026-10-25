# Tighter boundary HUD padding

2026-10-25. Minor spacing revision to ../compact-hud/concept-v1.png, guided by user-reference.png with highlighted wasted horizontal space. Preserve current words, font sizes, icon dimensions, heights, vertical stacking and passive input behavior. No new UI elements or redesigned concept.

Cards:420 to324 design units wide (-23%); AO skull left24 to4, text left100 to64, right20 to8. Setup/staging bar matches324 width, side padding18 to12. AO title remains28, timer40; setup title24/timer28; skull60x64; heights100/44 unchanged. RETURN TO SETUP is the longest title and needs explicit rendered clipping check. Existing native center anchors and fixed warning rows preserve non-overlap.

Checks: same-sized content, reduced marked gaps, all titles/timers fit, same-width aligned stack. No gameplay, script or input changes.

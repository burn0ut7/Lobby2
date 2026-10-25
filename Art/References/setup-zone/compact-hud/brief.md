# Compact setup and boundary HUD
Active concept: concept-v1.png, built-in image generation using existing MCP screenshot.
Normal journey: read setup time; if outside a boundary read its return deadline; return;
warning disappears; setup expiry removes setup bar while any active AO warning remains.
Passive HUD: ignores cursor, no focus/actions. Same behavior for keyboard or controller.

| Element | Player question | Owner | Decision |
| --- | --- | --- | --- |
| SETUP and m:ss in one row | When am I released? | Setup owner deadline | Combine, compact 420x44 |
| Return-to-setup skull card and numeric deadline | Where must I return, how soon? | Setup outside deadline | Reuse AO warning layout under bar |
| Return-to-AO skull card and numeric deadline | Which other boundary am I violating? | AO authority | Keep, stack below setup warning |
| Weapons disabled / stay in area text | Redundant routine information | Setup | Remove |
| STAGING timer | When does admin staging end? | Admin freeze | Same compact layout |

Theme: existing AO black translucent surface, white Roboto bold type, vanilla skull
only for death countdowns. Bar at y28, bottom72; warnings y80 and188, height100,
same420 width,8px gap. No colored decoration, new icons or extra HUD fact.
AO alone keeps original y70. Setup and AO warning clocks independent; setup warning
always first, AO second when both present. No Z-order dependence for separation.
AO enforcement active during setup by explicit user choice; admin freeze keeps its
existing authority behavior. AO wall eligibility must match the updated enforcement.

Pass checks: compact single-line bar; no routine weapons text; proper named warning
and live countdown on exit; both visible without overlap; reentry/reset, expiry,
AO death while setup active, release while AO remains; no duplicate widgets.
Compile, MCP screenshots and rectangle readbacks; document viewport/input limits.

Concept critique: clear one-line setup bar and distinct warning labels; common width
and whitespace separate all three. Generated editor pixels are illustrative; actual
layout uses native scaling, with precise 44/100 heights and 8px design gaps.

Concept generation brief: edit the existing AO Workbench screenshot, preserving gameplay/editor surroundings. Show a compact single-line SETUP 1:24 bar, then RETURN TO SETUP 18.450 and RETURN TO AO 9.320 cards. Reuse white bold type, translucent black surfaces and vanilla skull style. Equal widths, small visible gaps; no routine guidance or colored accents. AI image is illustrative, native layout metrics remain authoritative.

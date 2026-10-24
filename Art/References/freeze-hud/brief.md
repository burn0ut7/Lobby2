# Staging countdown HUD

Active visual reference: **concept-v2.png** (2026-10-22). Version 1 is preserved as concept-v1.png.

Scope: small non-interactive HUD shown to ordinary players and spectators during active GAME staging. Not an admin-menu redesign. Production ownership is GC_LobbyFreezeComponent; the overlay closes when countdown/staging ends or the game mode exits. Build with native panels and text; these concept images must remain outside runtime import folders.

## Defining traits and acceptance

- Top-center placement, with approximately 28 px top margin at 1920×1080; central gameplay view remains unobstructed.
- Compact straight-corner dark blue-green translucent rectangle with a thin amber left edge. No map pattern or ornate frame.
- Three centered rows: small amber **STAGING**, prominent off-white **02:15**, and subdued off-white **Stay in your staging area • Weapons disabled** on one line.
- Narrow readable sans serif consistent with the existing Lobby2 typography. Timer dominates; guidance remains secondary.
- No interaction, controls, focus capture, icons, or animation implied by this static reference.
- Canonical implementation target is approximately **360×90 at 1920×1080**. The generated v2 canvas is 1672×941 and its panel is approximately 282×84 (about 324×96 normalized to 1920×1080). The modest width/height correction to 360×90 is intentional so native layout can preserve readable spacing. Retain the small on-screen footprint and all defining traits.
- Compare the actual rendered native overlay with concept-v2.png in gameplay context, including a reduced Workbench viewport and 1920×1080 where available. Verify text fits and the countdown never blocks the view center.

## Sources and tool

Creative direction and visual inspection: gpt-6-astra, xhigh.
Media: built-in image_gen.imagegen, two calls. This is generated concept media, not runtime evidence.
Style source: ../admin-tools/source-lobby-v1.png.
Gameplay context source: C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792697015305468700.png.
The concept reconstructs the test-grid viewport as a full-screen context. It is not a pixel-preserving screenshot edit.

Saved outputs:
- Art/References/freeze-hud/concept-v1.png — initial, oversized panel with faint map texture; retained for provenance.
- Art/References/freeze-hud/concept-v2.png — active; smaller plain panel, exact requested text.
- Art/References/freeze-hud/brief.md — this brief.

No runtime asset or source was created by this concept task. Independent review of the completed implementation remains required.

## Initial prompt

Use case: ui-mockup.
Asset type: Concept reference for an all-player staging countdown HUD in Arma Reforger Lobby2.
Input images: Image 1 is the existing lobby art-direction reference only (dark desaturated military panels, off-white text, restrained amber accent). Image 2 is gameplay-context reference: empty light concrete test grid, sea horizon and pale blue clear sky in the central game viewport. Do not copy editor chrome.
Primary request: Produce one 1920x1080 landscape gameplay-context mockup of this compact countdown HUD. Use the game viewport of Image 2 as the full background, preserving its simple flat concrete test area, sea horizon, clear sky, viewpoint and muted realistic rendering. Remove the surrounding editor panels, toolbars and console entirely.
HUD composition: A single compact panel centered horizontally near the very top, with top edge at about y=28 px. Panel dimensions about 360 px wide by 90 px high at this 1920x1080 presentation (under one fifth of screen width). This must be genuinely small and non-obstructive, leaving the center of the view unobstructed. Rectangular, nearly black desaturated blue-green translucent surface, straight corners, subtle edge. Thin 2 px amber line on the left edge only. Avoid oversized typography and decorative frames.
Text exactly, centered, in three lines:
STAGING
02:15
Stay in your staging area • Weapons disabled
Typography: clean narrow military sans serif consistent with Image 1. STAGING small, about 12 px, uppercase with modest tracking and muted amber. Countdown 02:15 is the focal point in off-white at about 30 px. Bottom guidance one single line, off-white slightly muted at about 12 px; keep it readable within the 360 px panel. Balanced close spacing and about 10 px padding.
Constraints: This is a quiet gameplay HUD, not an admin menu, title screen, dialog or redesign of lobby navigation. No controls, no tabs, no captions, no callout arrows, no additional symbols, no branding, no sample device, no watermarks. Do not add extra gameplay characters, weapons, vehicles or scenery. Output only the game view with the one small top-center HUD panel.

## Refinement prompt

Use case: ui-mockup edit. Edit this staging HUD concept with one targeted size/style correction. Preserve the entire gameplay background, viewpoint, aspect ratio, and exact three lines of text. Keep the HUD horizontally centered with its top edge at the same height. The current panel is too large: make the whole HUD about 20 percent narrower and 28 percent shorter. On the 1672x941 input image, target the outer HUD panel bounds at approximately x=679, y=24, width=314, height=79 (equivalent to 360x90 at 1920x1080). Use a flat plain nearly-black blue-green translucent panel, with absolutely NO map texture or other pattern. Preserve the very thin amber accent down the left edge, straight corners, amber small STAGING label, off-white countdown 02:15 and subdued single-line guidance "Stay in your staging area • Weapons disabled". Reduce type sizes and spacing to fit; countdown about 26 px on this input canvas, guidance about 10.5 px. Keep text crisp and legible. Maintain the three-line hierarchy: STAGING, 02:15, then guidance. Do not add anything else. The result must be a quiet small top-center HUD over otherwise unchanged gameplay, implementable using native rectangles and text.


## Native implementation iteration (2026-10-22)

Creative-direction comparison by the concept author (gpt-6-astra, xhigh), not
independent final review: concept closeness passed this iteration against actual
MCP captures `1792698651136237100.png` (full) and `1792698629834540700.png` (reduced),
in the MCP repository `.local/admin-implementation/`. Top-center placement,
small footprint, amber edge/title, timer hierarchy and single-line guidance were
preserved. Native panel appears lighter/more transparent and timer strokes are
thinner; these were judged non-blocking, with full-size text readable and no
clipping. Reduced-window guidance is small. Final independent visual acceptance
and remote rendering remain pending.

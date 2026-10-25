# Smaller boundary HUD with consistent padding

2026-10-25; revision base ae62203. The user's second refinement supersedes the first near-zero spacing pass: keep smaller content, restore modest standard whitespace. Font sizes and skull dimensions are 75% of the previous values. Content-sized backing removes the unused fixed-width area. Final warning layout is 68 units high with 6-unit horizontal margins, a close visible skull/text gap, and centered title/timer column. Countdown is 32 units high with 6-unit side margins and an 8-unit text/time gap. Stacked cards have 6-unit gaps.

Existing compact HUD concept and marked user-reference.png remain the design references for this minor revision. First-tight captures preserve the rejected spacing; final-stack, final-seconds and final-long-labels show the revised spacing, seconds and minute counters, and longer custom headings. Final-freeze-stack verifies the existing admin staging display sharing this layout. Passive cursor behavior and gameplay rules remain unchanged.

## Live checks

Reforger MCP, embedded host in Arland_Lobby, optional McpTestAddon. Existing 3840×2160 Workbench window, embedded viewport approximately2613×1408, native DPI scale1.3037. Captures are original MCP results at1600 or3840 maximum dimension. No desktop input backend was used.

- Normal setup with both return warnings: countdown screen rect154.212×41.7185; setup warning294.311×88.6518; AO warning246.4×88.6518. Gaps7.8222 screen pixels correspond to6 design units. All titles, skulls and counters fit.
- Custom setup `BLUEFOR PREPARATION`, warning `RETURN TO THE OPERATIONS AREA`, and announcement `WAIT FOR THE NEXT MISSION PHASE` fit concurrently without overlap.
- Seconds-only counters around17 seconds and minute-formatted counters around1:55 remain readable.
- Returning inside cleared return warnings; ending setup removed its countdown. Admin staging and announcement occupied distinct rows.
- Review found duplicated countdown sizing across three owners. Extracted the exact shared layout operation into GC_CountdownHud.Fit; gameplay services remain separate. Recompiled through native reload operation2b22fbf034044a5582c0e9a9f625f0ce and re-exercised all three countdown paths. shared-sizing-stack.png and shared-sizing-freeze.png show this final implementation. Geometry matches the preceding captures.
- Raw invocation evidence is in receipts.json. Post-extraction setup/warning geometry: r8e1db8127156498eae278ca86657c22b. Admin staging: r58cdf86cb45143f5b24ab726e3f3761e. Announcement: r4bfcb0261da64579bd84228e369ee0a8.

final-log.json preserves the fresh error query from logs_2026-10-25_13-50-48 after final reload. No changed-HUD layout load failures or script exceptions appeared. Existing plugin/Steam/world/loadout/mouse-image diagnostics remain; the optional AO fixture logged its initially unconfigured state before successful Configure. These diagnostics are not claimed fixed.

Saved code checked for tabs, Allman braces, creator tags, method separators and readable flow. Resource widget names, RPCs, public contracts and gameplay semantics preserved. No new frame callback or widget recreation loop; native text measurement runs in the existing presentation updates. No performance benchmark was run.

Coverage limits: embedded host and existing viewport only; no constrained viewport, dedicated/remote client, dark-background comparison, or physical input/focus traversal test. This is passive HUD presentation. Runtime fixtures were discarded, not saved into Arland. Existing resourceDatabase.rdb and Arland layer changes remain outside this commit.

Independent reviewer hud_spacing_review approved the final source, screenshots and post-extraction state evidence with no remaining blocking findings.

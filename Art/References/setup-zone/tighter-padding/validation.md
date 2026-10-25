# Padding validation

2026-10-25; Workbench1.8.0.13, embedded Lobby2McpTests Empty_Lobby. Loaded revised native layouts through real setup/AO warning flow. Screenshots both-warnings.png and long-countdowns.png are original MCP captures.

All three card widths read422.4 physical pixels at1.3037 scale (324 design units), versus547.556 previously (420). Heights unchanged:57.363/130.37 physical (44/100 design). Warning y104.296/245.096 leaves10.43 physical /8 design gap. Fonts and icon dimensions unchanged. RETURN TO SETUP fully fits, minute-plus and59:59.xxx countdowns fit without clipping. Passive IgnoreCursor properties preserved. Narrow padding matches marked user-reference.png; original composition from ../compact-hud/concept-v1.png retained.

Layout-only change; no new gameplay logic or scripts. Tested nominal embedded viewport, bright background; alternate UI scales/constrained viewport and dark background not rerun. Arland unsaved edits saved with user authorization before switching; those world files remain outside task commit.

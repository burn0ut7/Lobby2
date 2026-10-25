# Validation, 2026-10-25

Tested in embedded Workbench Play with optional McpTestAddon, Arland, authenticated local admin. Normal lobby/faction/slot/briefing/game handlers reached GAME. Initial calls with obsolete widget names returned rejected without mutation; inspected the UI and used the actual names. Receipts retain those failures.

Actual captures ao.png and setups.png show the compact red stop button aligned to the AO tab, with equal 24%-width / 7.5%-height geometry; map button uses the same dimensions. Global description explains scope. Each row labels affected factions, status and mode. Three-faction Joint staging remains one row. Stable row-action heights avoid growth with wrapped text.

Production +5 minutes handler r0247b35130234936b71cc1e018ce7987 extended Joint staging to about14:51, leaving Setup1 at1:45 (catalog r8391a873dbda46f4897983a8fe068930). Stop-all button handler ra2fe6e8cb7a540f9bc0b5ded104a0ae8 resulted in AO disabled and both setups ended (r4386997f315c472c8fdf165217e0f67b).

Compiled during fresh project startup (log cycle14:07:59); modified UI ran in Play. Source readability and git diff whitespace checks passed. Existing world/plugin/resource/Steam/loadout diagnostics remain in logs; no changed-UI exception observed. No gameplay, authority or replication logic changed.

Coverage: current embedded viewport, captures at1920px, native widget handlers. No new dedicated/remote, hardware-input, constrained-resolution, or extreme faction-list coverage. Three ordinary factions were shown without wrapping; existing dynamic wrapping remains unchanged. Parent Arland was saved before testing; test fixtures are runtime-only.
Independent fresh review approved with no actionable defects. Stopping an active announcement was not retested in this refinement; its existing handler is unchanged.

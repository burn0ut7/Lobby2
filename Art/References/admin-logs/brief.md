# Admin mission logs

Journey: authenticated Admin Tools → Logs → filter current mission by player name/identity and event → pause/live or browse older records → Close. In-game lookup covers this session; permanent server JSON batches support future historical lookup. This is structured gameplay/admin history, not the engine debug console.

| Element | Question served | Owner | Decision |
|---|---|---|---|
| Existing tool sidebar/title/Close | Where am I; how do I leave? | Admin Tools | Reuse; add Logs |
| Player search | What happened involving this player, including disconnected players? | Server event filter | Keep; names or stable IDs, actor or victim |
| Event dropdown | Which kind of activity matters? | Server event filter | Keep; team kills separate |
| Live/Pause and Refresh | Follow incoming activity or hold a page still? | Local view | Keep; refresh latest, no per-shot RPC |
| Time/type/people/details rows | Who did what, when, and with which weapon? | Authority event records | Keep; plain text, team-kill accent |
| Newer/Older | Inspect earlier matching events without loading everything | Server sequence cursor | Keep |
| Status | Loading, no matches, access lost, retained-window limit or dropped records | Server response and UI | Show only relevant state |

Theme: existing nearly black blue-green Admin Tools surface, off-white text, amber active tool/focus, restrained red team-kill text. Existing tool typography and spacing. No decorative charts, totals cards, moderation actions, technical queue counters or extra explanatory panels.

Reference input: actual MCP capture `../admin-boundaries/simple-headings/other-tool.png`. Concept-v1 is a generated target, not runtime evidence. Nominal 1920x1080 and embedded Workbench viewport; existing native button/edit/combobox input mechanisms. Active concept: concept-v1.png. Implementation must retain bounded paging and legibility under long names/resource labels; logs must clear on admin access loss.

User clarification: each damage event remains an individual record and row; batching applies only to persistence/delivery. The concept's illustrative '2 impacts' damage row must become a single damage event with its own reported value. No damage coalescing.
Concept generated with image_gen from the actual Admin Tools screenshot. Critique: clear existing composition and accessible exit; retain native typography, plain-text rows and bounded paging. Use individual damage rows instead of the illustrated grouped damage entry. Source: exec-f899183b-382a-4e42-80a2-33436d9094e2.png.


Initial runtime: runtime-initial.png (MCP capture 2026-10-27 06:26:34 UTC, embedded Workbench viewport). Critique: existing composition retained, but row text too small; raised to 22 and row height to 80. Replaced duplicate footer with relevant page state. Same-timestamp records show sequence IDs. Added explicit native combobox OpenList binding, unchanged-page caching, and guarded teardown after the live close test exposed destroyed child handlers.

Final captures: runtime-damage.png shows two distinct native damage records; runtime-dropdown.png shows the native filter open; runtime-debrief.png verifies the separate debrief resource and team-kill row. Composition and populated rows were reviewed; keyboard/gamepad and long-text stress coverage remain unverified.

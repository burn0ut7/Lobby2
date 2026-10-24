# Validation — 2026-10-24

MCP restored. Workbench 1.8.0.13, process 25400, session
logs_2026-10-24_05-22-12. Loaded Lobby2 + optional McpTests/IsolationProbe.
Native reload passed after moving dependent test helpers into the later
GC_SlotAccessTests modded declaration. Initial compilation failure is retained
in the session log. Subsequent reloads report the existing BlockListApi leaked
instance; no claim of clean engine shutdown or zero diagnostics.

Live rendering exposed header images behind inherited backgrounds. Explicit
image Z Order 1 fixes both. Original screenshots in runtime/ show before, ratio
off/locked, and ratio on/unlocked. Concept remains concept-v1.png, using the
actual stock glyphs. Runtime colors, not a selected background, communicate state.

Embedded-host evidence in runtime/runtime-results.json:
- All 42 slots unlock and relock through the actual button handler; claims remain
  unchanged. Final lock pass r38bda798db4c4d9c971cdea4b3326b04 includes an occupied slot.
- Ratio counters 0/10 and 0/32 become 0/0 and 0/1 for one connected player.
- Ordinary slot1 request rejected with ratio on (rce79b5be15434be6b093b52e19af05f9),
  accepted with ratio off (r1a728fad66aa45ffa2c026cf1f46bba9), confirmed by native
  slot membership and correlated logs in runtime/runtime-log.json.
- Quota cases pass including 10:30 with 20 players, zero population and ties.
- Invalid actor/stale revision/invalid rules rejected without mutation.
- One interim lock assertion failed during external clicks (revision advanced
  independently). Retained as an interrupted observation; later correlated checks pass.

Known test-world warnings include absent map entity, mouse-mask resource,
legacy resource properties, and missing loadout manager in Empty_Lobby. Later
checks used AdminMultiplayer with its loadout manager. These are not a clean
whole-project diagnostic pass. No multiplayer, JIP, real login/logout, regular
client screenshot, controller or keyboard navigation pass is claimed. User moved
to slot-status work before the connected-peer stage. The fixture was restored.

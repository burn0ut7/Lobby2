# Boundary admin controls
User requests emergency disable-all, individual AO enable/disable, setup extension/end, admin map reveal, faction-targeted display-only countdown. Explicitly chose setup End permanently. User rejected concept1 as convoluted, and asked that many-to-many faction membership be clear. Active reference concept-v3.png: separate AO limits / Setup timers / Announcement timer tabs; one row per named native zone, complete wrapping Applies to line; no faction grouping or zone duplication. Same factions can recur across rows. Admin name optional on prefab; native entity identity targets actions; locations are identified on the map.

Player/admin journey: open existing Admin tools -> Boundaries -> choose type -> find named zone and factions -> enable/disable AO or add5min/end setup -> server outcome and refreshed row. Panic remains visible across the three Boundaries tabs and bypasses normal UI pending request. Announcement is one separate current timer, multiple native faction choices or all, replace/stop. No restriction state touched. Map toggle personal admin only; named AO/setup outlines through existing map overlay.

Element rationale: tabs identify rule type; names identify zone; faction line shows audience; mode/state/time describe behavior; row actions act only on that zone; emergency button immediately releases all; announcement form creates display-only timer. No modal/details sidebar. Existing dark admin UI, amber selection, native tool buttons,20-26pt text. The red emergency button is inside Boundaries, directly below its tabs.

Source authority: existing admin controller owner RPC pattern, SCR_Global.IsAdmin checks on server, GC_AdminActionLogger; native faction manager for current audiences. Native GetCurrentCamera etc unrelated previous feature. MCP-only runtime verification required. Reuse native layouts/controls and owned-mode services; no base-game shared overrides.

Concept generated using image_gen, prompt emphasized flat functional row list, full wrapping faction membership and explicit tabs, no cards/detail pane. Initial concept saved rejected. Runtime evidence to follow.

Latest user direction: panic belongs inside Boundaries only, visibly red and immediate. Removed position coordinates from every row; locations are shown on the map. These changes supersede the sidebar placement described above.

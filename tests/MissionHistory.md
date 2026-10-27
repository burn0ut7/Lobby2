# Mission history and performance

## Contract and runtime checks

Use Lobby2 MCP Tests with Arland. The optional `GC_MissionHistoryTests.c` helpers read the production collector and request production mission completion; no automatic fixture runs. Reload scripts through MCP before testing. Do not ingest the live profile: copy all three outboxes to a temporary directory.

1. Start Play, inspect `GC_TestHistorySnapshot` on `GC_SessionLogComponent`. Check a new mission ID, stable profile server ID, initial phase/factions and `hostKind=workbench`.
2. Advance through the normal phase request, select a faction and command slot, then enter GAME. Wait at least ten simulation seconds. Check session and GAME samples, player/active-AI peaks, and the commander's persistent identity and interval.
3. Request `GC_TestHistoryEnd("USSR", 1002)` on `GC_LobbyManager`. Check native POSTGAME, completed/victory/USSR, phase timestamp, closed CO interval and unchanged performance samples during postgame. Stop Play and check the final archive has `closed=true`.
4. Start a fresh session and stop it without a native result. Check interrupted/unknown, no invented winner, stable server ID, new mission ID. A lobby-only session must still have history and attendance even without a player-stats batch.
5. `GC_TestPerformanceSummary` must yield 3 samples, FPS mean30/min10/max60, one below15 sample, player peak5, AI peak20 and maxgap15.
6. Build importer Release; apply schema to a temporary PostgreSQL database, then apply it again. Validate engine archives. Import latest history first (incomplete), remaining histories (complete), then player/session batches. Verify mission/player/CO/faction joins. Replay identical files, verify no extra rows; conflicting content with the same identity must be retained/rejected. Reject malformed negative FPS, inconsistent winner, missing faction list and reversed CO interval.

## Evidence, 2026-10-27

Workbench 1.8.0.13, process48584, Lobby2 MCP Tests, Arland. Final faction-snapshot script reload `cfef5707277f405f8b1d45f1bdc69fdc` succeeded. Importer Release build succeeded without warnings/errors; schema creation and repeat application succeeded after correcting an ambiguous SQL faction reference during development.

Mission `01a17de5-2c8f-8862-8400-000000460207`: initial snapshot `r708531b369a34b1ebc04ff003d2c4ca0` included US/USSR, PREGAME, hostKind workbench and the server ID from the preceding session. Phase requests and normal slot clicks used `r8fb950ac7622428ab1851e8980ab3559`, `r316c8a5039a14de7bf3083123cf7f57b`, `r8e3c7fd0e18d4b3d953dca3a085283f1`, `rab819447c04a4e4b9c081cf448c4b620`, `r1c2d30818bfe44169a10e5eff043c65c`. Running snapshot `r707339c838284fa0985f3ec8b7702286` retained its commander and four phases. The running readback preceded its first GAME sample; the final archive has seven session samples and one GAME sample. Native victory request `r70ff79a7313147da8efc61fa6fcc4de0` returned true; stop operation `5ae803c354974a9a9c1b8a6745d9831f` completed.

Deterministic accumulator receipt `r9f5f9eabd63348db969ee3e8a4e5b585` matched all expected values above. Earlier live GAME snapshot `r023093073de94cce82f62a7941404afa` measured one GAME sample and 31 session samples, with one player/14 active AI; victory snapshot `rae14562303d843af8ccd4250695fd3a1` recorded USSR and closed command interval.

Interrupted GAME mission `01a17de6-afa5-8527-8400-00000048b843`: live snapshot `r3f603e60b38145faaee005485fbeebac` recorded seven session/five GAME samples and retained the USSR CO. Stop `fa3e4bd2342046f996ec71fae3cb148d` completed; final revision3 recorded interrupted/unknown, closed=true, no winner, eight session/six GAME samples, and the closed CO interval. Pregame-only mission `01a17de8-3267-8588-8400-0000004a7b6a` snapshot `r1a9c6277960d460aabea9193c255f118` had the same server ID, initial factions, one phase and zero GAME samples. Stop `5ed057b915ad470bb89c5ee09a89824a` completed. Workbench remains open on the same world with test addons loaded and Play stopped.

Database checks are recorded separately in `evidence/mission-history-ingestion.txt`. Workbench's native mission header and server config are absent in these runs, so those strings remain empty; the world resource is present. Dedicated-server metadata/FPS behavior and load at production population have not been exercised. These are functional tests, not evidence of performance overhead or accurate client FPS. No production database deployment occurred.

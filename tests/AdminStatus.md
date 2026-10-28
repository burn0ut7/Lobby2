# Admin Status

Status uses server-owned player/admin counts, native `System.GetFPS`, `System.GetTickCount` and `AIWorld.GetCurrentNumOfActiveAIs`. The owner-only controller response checks current admin authority. Collection is limited to once per 250 ms per requesting controller; faster requests receive the already collected snapshot. There is no background server timer, entity scan, broadcast or disk history.

Visible metrics update every second and after coalesced player connection, disconnection and role notifications. Only changed text is written. History stores at most 60 ordered samples, one per five seconds while visible. It pauses while hidden and resumes as a continuous filled graph; it is not a wall-clock recording of hidden periods. Switching tabs retains values and history, while closing the admin menu releases them. One outstanding read is allowed, with a three-second retry deadline. Cached values remain on return; missing replies eventually show waiting feedback.

## Verification — 2026-10-28

- MCP Reload `3b915777fba148828943e367e462ee31`: script compile passed. Four existing Arland resource/plugin diagnostics remain unrelated.
- Native button activation opened Status, switched to Logs and returned: samples and values remained, waiting feedback stayed empty. A simulated 20-second hidden interval also returned populated immediately.
- One-second receipts showed updated FPS and uptime. Population notification handler triggered a coalesced read. This was notification-path testing, not an actual remote join.
- Optional `GC_TestStatus("bounds")`: 100 samples leave exactly 60, first40/last99. Ordered removal prevents the overlapping-fill defect found during review.
- Stale request IDs ignored. Logged-out server requests did not advance collection state; two requests emitted one central denial log.
- Original MCP captures: `1793198218797765800.png` (40-sample visual fixture), `1793198336794797100.png` (restored live metrics/history). Fixtures were cleared after capture.
- Independent source/visual review approved the final continuous-history and immediate-return behavior.

Local evidence lives under `reforger-script-tools/.temp/local/evidence/admin-scope-20261024/`, including `status-current.json`. A 100-collection embedded benchmark was below the millisecond timer resolution; this is not a loaded-server performance result. Dedicated peers, real remote joins/admin changes, network latency and console input remain unverified.

Use `GC_TestStatus("state"|"event"|"long-hide"|"bounds"|"history"|"stale"|"cost"|"denied")` through the generic MCP runtime interface. `bounds` and `history` are synthetic visual fixtures; close/reopen the admin menu afterward to restore only real data. `denied` requires an already logged-out requester and never grants permissions.

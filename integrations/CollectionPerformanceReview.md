# Collection review — 2026-11-10

Target: 128 connected players, normally one hour, up to two hours. AI and vehicle counts must be specified separately: recorder admission counts all tracked actors.

**Verdict:** source ownership and contracts are substantially unified, but an existing local importer package is incompatible with current output. Short native measurements show modest average recorder cost, but this is not clearance for 128-player performance. Synchronous publication creates game-thread spikes, and cumulative statistics checkpoints can exceed a 30-FPS frame budget at their configured history limit. No 128-client dedicated-server soak or sustained-fire benchmark was performed.

## Ownership and consistency

| Stream | Owner | Cadence and meaning |
|---|---|---|
| Player statistics | `GC_MissionStatsComponent`, character trackers, shared `GC_StatsAmmunition` | Match-only counters; one-second movement/assignment sampling; cumulative checkpoints every 30 seconds. Use the latest revision; never sum cumulative snapshots. |
| Session events | `GC_SessionLogComponent` | Shared event sequence and meaningful damage-credit owner; callbacks enqueue; up to four 256-event batches publish each second. |
| Mission history | `GC_MissionHistory` | Session lifecycle and command intervals; host FPS sampled every ten seconds; cumulative checkpoints every 30 seconds. Sampled FPS is not a frame-time trace. |
| Playback | `GC_TelemetryRecorder` | Same mission/session identity and session-world clock; nominal 5-Hz actor observations, change-only output and periodic refreshes. Combat references join session events. |
| Prior player history | `GC_PlayerHistory` and external bridge | Prior missions come from the database; current-match time remains owned by statistics. |

The shared .NET `LobbyBatch`/`TelemetryBatch`/contract library routes all streams. StatsIngest references it; the documented website dependency is a versioned package. This review did not inspect the deployed website's installed package. Separate streams are useful projections with different retention and precision. Network/database work is outside the engine; file publication is not.

Player and AI damage observers share the session credit owner and exclude duplicate player classification. AI counters are faction-level summaries, not invented player identities. Per-shot assignment/role lookup and AI retained-life scans remain event-time work; movement tests do not measure a firefight.

## Findings and changes

**Deployment mismatch requiring attention:** the existing package at `C:/Users/Gray/Documents/Lobby2Data/importer` has no `Lobby2.Contracts.dll` dependency in its manifest. Running its read-only `--validate` on a copy of current packet `01a1c5d0-d7ed-81af-8400-00000010e90b-telemetry-103.l2t` returned exit 1 / `InvalidDataException`; the current source build accepted the same recording. This proves incompatibility of that local package, not which package a remote production server currently runs. No deployed files, processes or database were changed. Update/verify collector and importer versions together; verify the website's package too. Source-level reuse alone does not prevent deployment drift.

1. **Remaining server risk: synchronous publication.** `GC_StatsOutbox.WriteVerified` and `GC_TelemetryBinaryBatch.SaveVerified` write and read back on the game thread. The two-millisecond telemetry budget is checked between actor observations: it cannot preempt map capture, one actor's inventory serialization, combat callbacks or publication. Four session batches may also publish in one tick. A replacement must preserve immutable retry payloads, event order and ready-marker publication. Removing verification is not the proposed fix.
2. **Fixed: incomplete recorder timing.** `Report()` adds `tickMs`, `maxTickMs` and `flushes`. Tick timing includes envelope construction and publication. It excludes out-of-tick combat callbacks, other collectors, fixture motion and shutdown flushes. Timers have millisecond resolution; they are not whole-server CPU or FPS measurements.
3. **Fixed: unchanged turret serialization.** Compare native snapshot fields before creating JSON. Presence, slot identity, weapon availability and direction still trigger output; five-second refreshes remain. Squared direction tolerance is `1e-12`. Native compartment discovery remains current rather than relying on a stale controller cache.
4. **Fixed: repeated queue shifting.** Successful session publication compacts the unpublished tail once. Failed publication retains the identical prefix. The old path repeatedly shifted the tail for each removed entry. Native timings were only 1 ms versus 0–1 ms, so this is an algorithmic cleanup, not a claimed large FPS improvement.
5. **Fixed: damage links stopped after 8,192 hits.** Replace the recording-wide first-8,192 lookup with two integers per recorded life. Only the hit retained by the existing session credit owner updates the cache. Death returns a link only when the exact session event matches. Bleed/self-damage cannot replace that retained credit; old lives cannot inherit it; memory no longer grows with mission hit count.

## Measurements

Workbench 1.9.0.37, local embedded authority, existing Arland scenario. Reload passed script compilation; existing Arland resource-reference and menu-preset diagnostics remain. These numbers reflect this workstation's filesystem, not production storage.

Synthetic one-hour cumulative statistics: 128 identities, two ammo rows per membership interval, two measured calls per size:

| Intervals/player | Bytes | Encode | Verified write/read | Combined stages |
|---:|---:|---:|---:|---:|
| 1 | 154,308 | 0–1 ms | 6–7 ms | 6–8 ms |
| 4 | 475,386 | 2 ms | 8–9 ms | 10–11 ms |
| 8 | 903,746 | 4–5 ms | 9–10 ms | 14 ms |
| 32 | 3,473,906 | 16–20 ms | 22–23 ms | 39–42 ms |

The last case reaches 4,096 intervals / 8,192 ammo rows. It is a stress bound, not a prediction for an ordinary one-hour match. Fixture construction, player sampling, final debrief construction and database ingestion are outside these stages. Test files go only to `$profile:CollectionPerformanceBench`, without production ready markers.

Moving recording `01a1c5d0-d7ed-81af-8400-00000010e90b` reached 128 concurrent tracks: existing scenario actors, four mounted-gun/vehicle fixtures and 107 extra characters moved by an explicit test callback. Three native deaths later reduced active tracks to 125. These were not 128 connected clients or thinking/fighting AI.

During a measured 45.180-second interval after fixture admission, recorder `tickMs` rose from 134 to 1,225: 1,091 ms, approximately 24.1 ms/second or 2.4% **recorder Tick elapsed fraction**. Capture max was 2 ms; total tick max was 9 ms during that interval and 11 ms later. This excludes combat callbacks and all other collector/game work. It establishes neither total overhead nor tail percentiles, and is not a before/after FPS comparison.

Queue tests preserved order and size for 8,192 queued events draining 0, 256 and 8,192 entries, restoring live references before return. The damage-link regression passed 10,000 retained-hit updates and life-isolation checks. Fresh native recording `01a1c5d3-db42-8b82-8400-0000001423e8` linked death 306 to original damage 165 / session event 7 after later self-damage, following the session owner's retained credit.

The moving recording sealed with 143 batches, 5,843,006 binary bytes and 109.416 seconds of captured time. All 143 copied batches passed the real importer validator and native capture audit: 50,817 character-state records, zero drops/gap records, contiguous sequences/revisions, monotonic timestamps and no states after death. Median emitted-state interval was 202.172 ms; maximum was 5,146.965 ms. These are emitted-state intervals, including stationary refreshes, not direct measurements of every sample call. A native packet also passed the decoder regression: one valid packet accepted and 20 malformed variants rejected. The 39 existing telemetry and 13 turret contract cases passed, as did the Release build. No production outbox files were consumed by these validation runs.

## Capacity and client impact

- Telemetry admits 256 active tracks and retains at most 256 retired lives. With 128 players, only 128 additional AI/vehicle tracks fit. Exceeding this stops capture explicitly; increasing it alone is not a performance fix.
- Telemetry stops at 512 queued records / 128 KiB. Normal flushing starts at 384 records / 64 KiB or five seconds. Projectile identity history is bounded to 4,096. The default 1,024-MiB quota is per recording, not global disk retention.
- Constantly changing player state alone costs `128 × 5 × 96` bytes/second: about 211 MiB/hour, or 422 MiB/two hours, before events, gear, maps, turrets and headers. This is a calculation, not observed full-mission storage. Fire and voice can materially increase it.
- Session persistence attempts at most `4 × 256 = 1,024` events/second. An illustrative sustained 1,280-event/second input fills an initially empty 8,192-event queue in about 32 seconds, ignoring other events. This is a stress calculation, not an observed match rate. Overflow is reported and completeness is lost.
- Statistics cap identities at 1,024, intervals at 4,096 total / 256 per player, ammo rows at 8,192 total / 128 per interval, pending deaths at 256 and remembered deaths at 16,384. Allocation bounds do not guarantee latency; cumulative history is serialized again every 30 seconds.
- Personal combat histories retain 256 events per identity, but that dictionary has no separate identity cap. Stable 128-player participation limits this workload; high identity churn needs a retention policy. AI damage observers follow native entity deletion rather than all historical spawns.
- Full replay data is not broadcast to clients. Local voice-state reports refresh every 500 ms while active, with change throttling; radio totals report every ten seconds. Spectator feeds are owner-targeted, change-driven and limited to 20-event pages at most once per second; focused caches are bounded. Many spectators/voice senders still require network measurement.
- The external importer processes files transactionally, with checksums and indexed continuity checks. Telemetry uses serializable transactions. Sharing the same physical server means importer CPU/disk contention still matters. Earlier native/codec measurements in `tests/evidence/telemetry-binary-20261028/report.txt` show binary reduces engine serialization/storage but can increase external JSON projection cost.

## Acceptance gate

### Startup comparison switches

On the scenario's `GC_LobbyGameMode` entity, use the **Lobby2 Collection** properties before starting the world. Both default to enabled; existing scenarios retain collection without edits. Restart the scenario between comparisons and distribute the same authored scenario to server and clients.

| Collection Enabled | Playback Telemetry Enabled | Result |
|---|---|---|
| On | On | Normal statistics, session/mission history and playback, including turrets. |
| On | Off | Statistics and session/mission history remain; no playback recorder or playback voice reports. |
| Off | Either | No collection session, statistics sampling, damage trackers, playback recorder, history polling or collection voice/radio reports. |

The full-off run has no new debrief statistics, combat event history or database-derived new-player highlighting. Gameplay, voice communication, administrative authorization and console audit logging remain active. Existing files and the separately running importer are untouched; drain its backlog or control its workload separately for comparable measurements. Empty component containers and disabled-path guards still exist: this is collection off, not an addon-free baseline.

These are startup settings, not live switches. The existing authenticated `SetTelemetry` entry point cannot override a disabled startup policy. For fair comparison, use equivalent missions and exclude startup/shutdown work consistently.

Workbench 1.9 native startup checks passed for unchanged defaults, statistics-only and all-off through the production phase transitions. Defaults started both collectors; statistics-only retained session/history and statistics without a recorder; all-off retained an empty session ID, zero event sequence/queue and no history, stats collection or recorder. An authenticated playback enable request under the disabled startup policy returned false with `rejected-startup-settings` audit output. The optional `GC_CollectionSettingsTests.c` fixture applies test settings before initialization and ships inactive (`-1`). Workbench 1.9's MCP entity-property operation is unsupported, so this checks runtime startup behavior rather than checkbox editing/persistence. Remote-client traffic and dedicated-server comparisons remain unmeasured.

On intended dedicated hardware/storage, run equivalent telemetry-off/on workloads for at least one hour and a two-hour endurance pass. Include 128 clients, representative AI/vehicles, firefights, turret motion, gear changes, voice and spectators. Measure frame-time p50/p95/p99/max, all collector work including callbacks/checkpoints, process memory, sample cadence, gaps/drops, disk growth/free space and importer lag. Include startup/JIP and mission-end bursts.

Choose an explicit acceptable frame-time regression before calling the impact negligible. If writes consume too much frame budget, prioritize incremental snapshots and publication scheduling while preserving retry and completeness semantics. Keep current bounded failure behavior until a replacement is proven. Contract tests and zero drops in a short native fixture are not production capacity certification.

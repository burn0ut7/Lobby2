# First three hours of playtime

Implemented 2026-11-01. Uses existing connected-during-GAME statistics,
including spectators, across servers sharing PostgreSQL. Gold below 10,800
seconds; admin red has priority. Unknown/stale history does not imply new.

## Validation

- Release importer build: zero warnings/errors. Existing 34 telemetry contract
  cases passed after the ingestion loop refactor.
- `player_history_test.py` passed against fresh disposable database
  `lobby2_history_test_576502345d`: schema applied twice, actual imports and
  mailbox responses, latest cumulative revisions, idempotent retries, exclusion
  of current mission, partial/legacy/interrupted/missing GAME history, confirmed
  lobby-only zero, retained invalid optional telemetry, unique/new/returning
  counts, and unknown-playtime reporting.
- Local real database received migration 003 and SELECT grants for existing
  importer/reader roles. No synthetic player/time records were inserted there.
- Workbench 1.8.0.13 Reload `34fe496adc83489cb1164fccbf5bc3ef` succeeded after
  the client identity fix. Known Arland resource, dedicated plugin and formation
  fixture diagnostics remain unrelated.
- Embedded production `GC_PlayerHistory.Tick` fixture results: 10,799 -> new,
  10,800 -> normal, unknown/stale -> normal, wrong identity rejected. Fixtures
  alter only in-memory history input, not production counters or database.
- Real mailbox path verified with the real external service, a test-profile
  mailbox junction and isolated database. Game-generated request/response
  tokens were consumed by the engine. Server roster: player 1 new/admin,
  player 2 new/non-admin. No in-memory fixture injection was used for this
  positive result.
- Separate diagnostic client received both flags: correlated report
  `history-gold-01`, operation `74a0fb33bcfc4f95bedbe94faf69060c`.
  The server was already new before client arrival (late-join state), then
  the client's own flag was delivered after identity lookup.
- At a fixture prior total of 10,800, production Tick removed player 2's
  flag; client report `history-threshold-01`, operation
  `446e9bf67d27414eb58ad48d8b7e61a1`, confirmed new=0/admin=0.
  Native host captures show gold then white, with admin red unchanged:
  `evidence/player-history-20261101/host-gold.png` and `host-threshold.png`.
- An earlier peer pass exposed a server-only native identity lookup in client
  presentation. It now uses the existing replicated voice identity state.
  Final peer log `logs_2026-11-01_16-04-53`, lines 721-916, contains no matching
  PlayerHistory error. Client screenshot did not show the menu; visual evidence
  is the host roster, while correlated client reports establish replication.

No dedicated 128-player benchmark, three-hour wall-clock soak, or addon-free
mode-switch run was performed. Changes extend only the Lobby2 manager and its
existing presenter; there is no new base-game override. The existing 30-second
stats publication/crash-loss window remains. Production deployment needs the
documented `--serve-profile` process per profile against the shared database;
no recurring service was installed by these tests.

The existing development proof deliberately retains invalid old source batches.
Tests never consumed those originals. Resolve such retained standard batches
before enabling a bridge against that profile, or history remains unavailable.

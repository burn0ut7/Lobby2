# Mission history ingestion

Lobby2 writes cumulative player snapshots and append-only death events every 30 seconds to `$profile:Lobby2Stats`. A `.json.ready` marker publishes a verified payload. A separate process imports batches into PostgreSQL; database/network work never runs on the game thread.

PostgreSQL is the chosen history store: relational joins support players, missions and membership intervals; JSONB preserves the versioned source record. Website and Discord services should query these views through their own service accounts. Neither service should connect to the game server's runtime. Future replay files can use `missionId` and mission-relative seconds as their join keys and remain separate from these low-frequency records.

## Run

Requires .NET 10 and PostgreSQL. Apply `schema.sql` with a migration account. Give the importer account SELECT and INSERT on `lobby2.mission_batches`, plus USAGE on schema `lobby2`.

```powershell
dotnet publish integrations/StatsIngest/StatsIngest.csproj -c Release -o stats-ingest
# Supply LOBBY2_STATS_CONNECTION_STRING through the server's secret configuration.
dotnet stats-ingest/StatsIngest.dll --validate 'C:\server-profile\Lobby2Stats'
dotnet stats-ingest/StatsIngest.dll --once 'C:\server-profile\Lobby2Stats'
```

Run `--once` on a periodic service schedule. Exactly one importer owns the directory. Database commit precedes file deletion. Retrying an identical mission/revision is idempotent; a different checksum is rejected and retained. Invalid batches remain for operator inspection and do not prevent later valid batches from being processed. Never manually change a published payload under the same filename.

`--validate` validates the contract without a database or deleting batches. Payloads are limited to 64 MiB. Monitor retained files and free disk space. A crash can lose up to the last unflushed interval; the writer does not claim power-loss fsync guarantees. An interrupted mission stays non-final. Memory capacity overflow marks history incomplete; the public totals views exclude incomplete or unfinished missions.

## Query surfaces

- `missions`, `mission_players`: latest cumulative snapshot; never sum revisions.
- `player_totals`, `player_roles`: completed and complete mission histories.
- `deaths`: append-only victim/killer/weapon/distance events, including unknown attribution.
- `memberships`: faction, squad, leader, role and slot intervals with activity counters.
- `squadmates`: overlapping membership intervals in the same faction/group.
- `faction_totals`, `ammunition`, `faction_ammunition`: activity attributed to membership at collection time and projectile/magazine/weapon breakdowns.

Final totals are exposed only after all revisions from zero through the final revision have arrived. Personal identity and membership history are sensitive backend data: do not grant website visitors or Discord users direct database access. The in-game owner-only personal report does not publish another player's report; faction reports contain aggregates only.

## Collection semantics

Collection runs during native GAME. Counted play time requires at least five connected players at both ends of the one-second sample; there is no five-minute eligibility requirement. Lobby time is outside GAME. Connected spectators count toward population and receive counted time under this initial policy. Role time uses the same population rule. Other activity counters do not require five players.

Movement is sampled displacement, not an exact travelled path. Passengers do not receive driver/pilot distance. Teleport-sized jumps are discarded. Radio time is a client-reported estimate, bounded by elapsed server time; reports arrive about every ten seconds, with a two-second grace period at mission end. Speech spanning membership changes is apportioned by interval overlap, so faction/squad radio attribution is approximate. Disconnecting can lose the last unsent radio report. CPR and replay telemetry are deferred.

Shots are projectile-shot callbacks, with projectile, magazine and weapon resource names retained. Hand-thrown grenades have a separate counter and also contribute their grenade prefab/type to the ammunition breakdown; they do not increment shots. Mounted turret projectile callbacks use the same collection path. These are aggregate counts, not individual shot timestamps, trajectories, hit locations or accuracy records. Player kills count other human players, including friendly fire; death events retain the engine relationship code for later filtering. Unknown weapon attribution is empty, and unknown distance is -1. CO and squad-leader counts are one per mission if the role was observed; role selections record slots used during GAME, preserving the prefab role name and slot ID. Slot labels currently derive from the role prefab in Lobby2.

Membership records query native faction/group/leader ownership, closing an interval when those values or the selected role change. Teammates are reconstructed by overlapping intervals rather than copying a roster every second. Replay data can later join these records without changing their ownership.

The debrief is available during POSTGAME through the lobby entry and header. Lobby2 replaces its inherited stock end-screen component and sets its prefab auto-reload delay to -1 so the report remains readable; explicit server auto-reload configuration still takes precedence. Personal replies go only to the requesting controller's owner. Faction replies contain totals and distinct participant count, never the roster. The ammunition display lists up to 32 types with bounded labels and explicitly reports additional types; full retained breakdowns remain in history. Replies are paced to at most two per second per controller. Capacity loss is shown as a partial report.

The collector bounds unique mission players to 1024, membership intervals to 4096 total/256 per player, ammunition rows to 8192 total/128 per interval, pending death events to 256, and remembered death entities to 16384. Reaching these limits marks the mission incomplete instead of publishing complete lifetime totals. Disk snapshots still run on the game thread every 30 seconds; only network/database work is external. Size and timing should be measured on the actual dedicated server before a large event.

References: [PostgreSQL JSON types](https://www.postgresql.org/docs/current/datatype-json.html), [PostgreSQL Windows distributions](https://www.postgresql.org/download/windows/).

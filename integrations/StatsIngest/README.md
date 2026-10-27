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

Shots are projectile-shot callbacks, with projectile, magazine and weapon resource names retained. Hand-thrown grenades have a separate counter and also contribute their grenade prefab/type to the ammunition breakdown; they do not increment shots. Mounted turret projectile callbacks use the same collection path. The session log additionally records individual shot and damage timestamps; trajectories, hit locations and accuracy remain out of scope. Player kills count other human players, including friendly fire; schema v2 also counts team kills separately as a subset, using the engine friendly-player relationship. Earlier v1 records have unknown team-kill counts (SQL NULL); death events retain the engine relationship code for later filtering. Unknown weapon attribution is empty, and unknown distance is -1. CO and squad-leader counts are one per mission if the role was observed; role selections record slots used during GAME, preserving the prefab role name and slot ID. Slot labels currently derive from the role prefab in Lobby2.

Membership records query native faction/group/leader ownership, closing an interval when those values or the selected role change. Teammates are reconstructed by overlapping intervals rather than copying a roster every second. Replay data can later join these records without changing their ownership.

The debrief is available during POSTGAME through the lobby entry and header. Lobby2 replaces its inherited stock end-screen component and sets its prefab auto-reload delay to -1 so the report remains readable; explicit server auto-reload configuration still takes precedence. Personal replies go only to the requesting controller's owner. Faction replies contain totals and distinct participant count, never the roster. The ammunition display lists up to 32 types with bounded labels and explicitly reports additional types; full retained breakdowns remain in history. Finalization pushes the personal report and every registered faction summary together to the owning client. Its controller caches these immutable reports, including empty/partial results, for immediate tab switches and menu reopening. A client missing reports requests the full set once; server bursts are paced to at most two per second per controller. Cache state is cleared outside POSTGAME and during presentation shutdown. Capacity loss is shown as a partial report.

The collector bounds unique mission players to 1024, membership intervals to 4096 total/256 per player, ammunition rows to 8192 total/128 per interval, pending death events to 256, and remembered death entities to 16384. Reaching these limits marks the mission incomplete instead of publishing complete lifetime totals. Disk snapshots still run on the game thread every 30 seconds; only network/database work is external. Size and timing should be measured on the actual dedicated server before a large event.

References: [PostgreSQL JSON types](https://www.postgresql.org/docs/current/datatype-json.html), [PostgreSQL Windows distributions](https://www.postgresql.org/download/windows/).

## Admin session logs

Admin Tools → Logs shows the current mission only. Search matches actor or victim name or identity, including disconnected players. The event filter separates team kills; “Deaths (all)” also includes kills and team kills. Live refreshes every two seconds; Paused holds the current page. Older/Newer browse 20 records at a time. The in-memory window holds the most recent 4096 records and explicitly directs admins to the server archive for earlier events.

Every request checks current server-side admin authorization. Repeated denied log-view requests are audited at most once per controller every five seconds; no log rows are sent to the denied requester. Filters are limited to 128 characters. Oversized audit details are explicitly truncated to fit the historical contract; the existing console audit output remains unchanged.

Every accepted damage callback is its own sequence-numbered event, even in the same engine tick. Healing/regeneration and the engine's empty duplicate impact notification are excluded; bleeding damage remains individual. `reportedDamage` is the engine context value, not a claim of final health removed. Shots and grenade throws are also individual events. Gameplay collection runs during GAME; connections and centrally logged admin actions span the session. No damage events are merged to reduce storage or network cost.

Deaths additionally carry `deathCause` (`respawn`, `bleeding`, or `other`), `creditSource` (`lastPlayerDamage`, `engine`, or empty when no other player is credited), and `damageEventSequence` (the original session damage event, or zero). These are additive fields; older records have no recorded cause. The in-game details show “Respawn selected” or “Bled to death”. Respawn means the accepted native respawn/suicide action, including the base game's leave-faction use of the same action; it is not inferred from a self-kill. Bleeding is identified from the native destroyed blood hit zone.

For respawn and bleed-out deaths, credit goes to the last other player who dealt positive non-bleeding damage to that character during the same life. There is no time expiry within that life; healing and self-damage do not erase that credit. The server retains identity, weapon, native faction relation and distance at the damaging hit, so later movement or disconnect cannot change those facts. Native terminal damage remains a separate log event and cannot replace the originating hit. A newly spawned character inherits no credit. Other deaths keep engine attribution. Team-kill classification follows the credited hit's native relation. This changes historical stats only, not native kick/penalty decisions.

The server publishes separate batches (up to 256 events each, at most four batches per second) to `$profile:Lobby2SessionLogs`. The queue retains up to 8192 records; disk failure retries the identical batch and reports unavailable storage. Overflow is explicitly counted as dropped history. Normal shutdown drains pending records and writes a closed marker; process crashes can lose unflushed records. Files are permanent: the importer removes only the `.ready` marker after commit and leaves session JSON on the server. There is no automatic deletion or retention cutoff. Back up the folder and monitor free disk space.

Apply the updated `schema.sql` with a migration account; it accepts v1 and v2 stats. Grant the ingestion account SELECT/INSERT on `lobby2.session_log_batches` and `lobby2.session_events`, plus its existing mission-batch grants. Run the same importer against the session log directory:

```powershell
dotnet stats-ingest/StatsIngest.dll --validate 'C:\server-profile\Lobby2SessionLogs'
dotnet stats-ingest/StatsIngest.dll --once 'C:\server-profile\Lobby2SessionLogs'
```

`session_events` stores one immutable JSONB row per `(session_id, sequence)`, indexed by actor, target and event type. Exact replay is idempotent; a conflicting event or batch rolls back and retains the local file. `session_log_status` exposes closure, dropped count and whether all batch revisions arrived. Session IDs match the mission stats ID; `sessionSeconds` starts at world load whereas `missionSeconds` starts at GAME. Their `startedAt` Unix timestamps provide the offset. A closed, contiguous stream with zero dropped records is the complete historical session; other streams must be presented as partial.

```sql
SELECT session_id, sequence, detail
FROM lobby2.session_events
WHERE detail->>'actor' = 'player-identity' OR detail->>'target' = 'player-identity'
ORDER BY session_id, sequence;
```

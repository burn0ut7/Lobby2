-- Run once with a database migration account. The ingester needs SELECT/INSERT only.
CREATE SCHEMA IF NOT EXISTS lobby2;
CREATE TABLE IF NOT EXISTS lobby2.mission_batches (
    mission_id uuid NOT NULL,
    revision integer NOT NULL CHECK (revision >= 0),
    checksum text NOT NULL,
    payload jsonb NOT NULL,
    received_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (mission_id, revision),
    CONSTRAINT mission_batches_schema_version CHECK ((payload->>'schemaVersion')::integer IN (1, 2)),
    CHECK ((payload->>'missionId')::uuid = mission_id),
    CHECK ((payload->>'revision')::integer = revision)
);

-- Latest cumulative snapshot wins. Summing successive batches would double count.
CREATE OR REPLACE VIEW lobby2.missions AS
SELECT DISTINCT ON (mission_id) mission_id, revision, payload, received_at
FROM lobby2.mission_batches
ORDER BY mission_id, revision DESC;

CREATE OR REPLACE VIEW lobby2.mission_players AS
SELECT mission_id, revision, (payload->>'final')::boolean AS finalized,
       ((payload->>'complete')::boolean AND
        (SELECT count(*) FROM lobby2.mission_batches b WHERE b.mission_id = missions.mission_id) = revision::bigint + 1)
       AS complete, player->>'identity' AS player_id,
       player->>'name' AS player_name, player AS stats
FROM lobby2.missions CROSS JOIN LATERAL jsonb_array_elements(payload->'players') AS player;

CREATE OR REPLACE VIEW lobby2.player_totals AS
SELECT player_id, count(*) AS missions,
       sum((stats->>'playSeconds')::double precision) AS play_seconds,
       sum((stats->>'shots')::bigint) AS shots,
       sum((stats->>'grenades')::bigint) AS grenades,
       sum((stats->>'bandagesOthers')::bigint) AS bandages_others,
       sum((stats->>'kills')::bigint) AS kills,
       sum((stats->>'deaths')::bigint) AS deaths,
       sum((stats->>'walkMetres')::double precision) AS walk_metres,
       sum((stats->>'driveMetres')::double precision) AS drive_metres,
       sum((stats->>'pilotMetres')::double precision) AS pilot_metres,
       sum((stats->>'commanderMissions')::integer) AS commander_missions,
       sum((stats->>'leaderMissions')::integer) AS squad_leader_missions,
       sum((stats->>'radioSecondsClientReported')::double precision) AS radio_seconds_client_reported,
       sum((stats->>'teamKills')::bigint) AS team_kills,
       count(*) FILTER (WHERE stats ? 'teamKills') AS team_kill_record_count,
       sum((stats->>'aiKills')::bigint) AS ai_kills,
       sum((stats->>'deathsByPlayers')::bigint) AS deaths_by_players,
       sum((stats->>'deathsByAI')::bigint) AS deaths_by_ai,
       sum((stats->>'deaths')::bigint - (stats->>'deathsByPlayers')::bigint - (stats->>'deathsByAI')::bigint) AS other_deaths,
       count(*) FILTER (WHERE stats ? 'aiKills') AS combat_breakdown_record_count
FROM lobby2.mission_players WHERE finalized AND complete GROUP BY player_id;

CREATE OR REPLACE VIEW lobby2.deaths AS
SELECT mission_id, (death->>'sequence')::integer AS sequence, death AS detail
FROM lobby2.mission_batches CROSS JOIN LATERAL jsonb_array_elements(payload->'deaths') AS death;

CREATE OR REPLACE VIEW lobby2.player_roles AS
SELECT player_id, role->>'prefab' AS role_id,
       sum((role->>'selections')::integer) AS selections,
       sum((role->>'seconds')::double precision) AS seconds
FROM lobby2.mission_players CROSS JOIN LATERAL jsonb_array_elements(stats->'roles') AS role
WHERE finalized AND complete GROUP BY player_id, role->>'prefab';

-- Membership is an interval, so reconnects and mid-mission group changes remain distinguishable.
CREATE OR REPLACE VIEW lobby2.memberships AS
SELECT mission_id, player_id, player_name, finalized, complete,
       assignment->>'faction' AS faction_id, assignment->>'groupId' AS group_id,
       assignment->>'groupName' AS group_name, assignment->>'leader' AS leader_id,
       assignment->>'role' AS role_id, (assignment->>'slotId')::integer AS slot_id,
       (assignment->>'fromSeconds')::double precision AS from_seconds,
       (assignment->>'untilSeconds')::double precision AS until_seconds,
       assignment AS stats
FROM lobby2.mission_players CROSS JOIN LATERAL jsonb_array_elements(stats->'assignments') AS assignment;

CREATE OR REPLACE VIEW lobby2.faction_totals AS
SELECT mission_id, faction_id, count(DISTINCT player_id) AS participants,
       sum((stats->>'playSeconds')::double precision) AS play_seconds,
       sum((stats->>'shots')::bigint) AS shots,
       sum((stats->>'grenades')::bigint) AS grenades,
       sum((stats->>'bandagesOthers')::bigint) AS bandages_others,
       sum((stats->>'kills')::bigint) AS player_kills,
       sum((stats->>'deaths')::bigint) AS deaths,
       sum((stats->>'walkMetres')::double precision) AS walk_metres,
       sum((stats->>'driveMetres')::double precision) AS drive_metres,
       sum((stats->>'pilotMetres')::double precision) AS pilot_metres,
       sum((stats->>'radioSecondsClientReported')::double precision) AS radio_seconds_client_reported,
       sum((stats->>'teamKills')::bigint) AS team_kills,
       count(*) FILTER (WHERE stats ? 'teamKills') AS team_kill_record_count,
       sum((stats->>'aiKills')::bigint) AS ai_kills,
       sum((stats->>'deathsByPlayers')::bigint) AS deaths_by_players,
       sum((stats->>'deathsByAI')::bigint) AS deaths_by_ai,
       sum((stats->>'deaths')::bigint - (stats->>'deathsByPlayers')::bigint - (stats->>'deathsByAI')::bigint) AS other_deaths,
       count(*) FILTER (WHERE stats ? 'aiKills') AS combat_breakdown_record_count
FROM lobby2.memberships WHERE finalized AND complete AND faction_id <> ''
GROUP BY mission_id, faction_id;

CREATE OR REPLACE VIEW lobby2.ammunition AS
SELECT mission_id, player_id, faction_id, group_id,
       ammo->>'projectile' AS projectile, ammo->>'magazine' AS magazine,
       ammo->>'weapon' AS weapon, sum((ammo->>'rounds')::bigint) AS rounds
FROM lobby2.memberships CROSS JOIN LATERAL jsonb_array_elements(stats->'ammo') AS ammo
WHERE finalized AND complete GROUP BY mission_id, player_id, faction_id, group_id,
       ammo->>'projectile', ammo->>'magazine', ammo->>'weapon';

CREATE OR REPLACE VIEW lobby2.faction_ammunition AS
SELECT mission_id, faction_id, projectile, magazine, weapon, sum(rounds) AS rounds
FROM lobby2.ammunition GROUP BY mission_id, faction_id, projectile, magazine, weapon;

-- Use half-open intervals: touching membership changes do not imply shared squad time.
CREATE OR REPLACE VIEW lobby2.squadmates AS
SELECT a.mission_id, a.player_id, b.player_id AS squadmate_id, a.faction_id, a.group_id,
       greatest(a.from_seconds, b.from_seconds) AS from_seconds,
       least(a.until_seconds, b.until_seconds) AS until_seconds
FROM lobby2.memberships a JOIN lobby2.memberships b
  ON a.mission_id = b.mission_id AND a.group_id = b.group_id AND a.faction_id = b.faction_id
 AND a.player_id <> b.player_id AND a.group_id <> ''
 AND a.from_seconds < b.until_seconds AND b.from_seconds < a.until_seconds
WHERE a.finalized AND a.complete AND b.finalized AND b.complete;

-- Upgrade the schema-version check in databases initialized before team-kill counters.
DO $$
DECLARE constraint_name text;
BEGIN
    FOR constraint_name IN SELECT conname FROM pg_constraint
      WHERE conrelid = 'lobby2.mission_batches'::regclass AND contype = 'c'
        AND pg_get_constraintdef(oid) LIKE '%schemaVersion%'
    LOOP
      EXECUTE format('ALTER TABLE lobby2.mission_batches DROP CONSTRAINT %I', constraint_name);
    END LOOP;
    ALTER TABLE lobby2.mission_batches ADD CONSTRAINT mission_batches_schema_version
      CHECK ((payload->>'schemaVersion')::integer IN (1, 2));
END $$;

CREATE TABLE IF NOT EXISTS lobby2.session_log_batches (
    session_id uuid NOT NULL,
    revision integer NOT NULL CHECK (revision >= 0),
    checksum text NOT NULL,
    payload jsonb NOT NULL,
    received_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY (session_id, revision),
    CHECK (payload->>'kind' = 'sessionEvents'),
    CHECK ((payload->>'schemaVersion')::integer = 1),
    CHECK ((payload->>'sessionId')::uuid = session_id),
    CHECK ((payload->>'revision')::integer = revision)
);
CREATE TABLE IF NOT EXISTS lobby2.session_events (
    session_id uuid NOT NULL,
    sequence integer NOT NULL CHECK (sequence > 0),
    detail jsonb NOT NULL,
    PRIMARY KEY (session_id, sequence),
    CHECK ((detail->>'sequence')::integer = sequence)
);
CREATE INDEX IF NOT EXISTS session_events_actor ON lobby2.session_events ((detail->>'actor'), session_id, sequence);
CREATE INDEX IF NOT EXISTS session_events_target ON lobby2.session_events ((detail->>'target'), session_id, sequence);
CREATE INDEX IF NOT EXISTS session_events_type ON lobby2.session_events ((detail->>'type'), session_id, sequence);
CREATE OR REPLACE VIEW lobby2.session_log_status AS
SELECT session_id, max(revision) AS latest_revision,
       bool_or((payload->>'closed')::boolean) AS closed,
       max((payload->>'droppedRecords')::integer) AS dropped_records,
       count(*) = max(revision)::bigint + 1 AS all_batches_received
FROM lobby2.session_log_batches GROUP BY session_id;
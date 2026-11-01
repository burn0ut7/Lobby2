"""Integration checks using an EMPTY disposable PostgreSQL database and the real importer.

Set PGHOST/PGPORT/PGUSER/PGPASSWORD/PGDATABASE, PSQL and
LOBBY2_STATS_CONNECTION_STRING. PGDATABASE must start with lobby2_history_test.
"""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / 'integrations/StatsIngest/bin/Release/net10.0/StatsIngest.dll'
assert os.environ['PGDATABASE'].startswith('lobby2_history_test')


def sql(statement):
    result = subprocess.run([os.environ['PSQL'], '-X', '-At', '-v', 'ON_ERROR_STOP=1'],
                            input=statement, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return result.stdout.strip()


schema = (ROOT / 'integrations/StatsIngest/schema.sql').read_text()
sql(schema)
sql(schema)
assert sql('SELECT count(*) FROM lobby2.mission_batches') == '0', 'Use an empty database'


def counters(seconds):
    data = dict.fromkeys(('shots', 'grenades', 'bandagesOthers', 'kills', 'deaths', 'teamKills',
                         'commanderMissions', 'leaderMissions', 'walkMetres', 'driveMetres',
                         'pilotMetres', 'radioSecondsClientReported'), 0)
    data['playSeconds'] = seconds
    return data


def batch(identity, seconds, mission=None, revision=0, final=True, complete=True, version=3):
    assignment = dict(counters(seconds), faction='', groupId='', groupName='', leader='',
                      role='', slotId=-1, fromSeconds=0, untilSeconds=seconds, ammo=[])
    player = dict(counters(seconds), identity=identity, name=identity, assignments=[assignment], roles=[])
    return dict(schemaVersion=version, missionId=mission or str(uuid.uuid4()), world='test.ent',
                startedAt=1793491200, revision=revision, final=final, complete=complete,
                durationSeconds=seconds, playtimeBasis='connectedDuringGame', minimumPlayersForPlaytime=5,
                population=[dict(fromSeconds=0, players=1)], players=[player], deaths=[])


with tempfile.TemporaryDirectory(prefix='lobby2-history-test-') as temporary:
    profile = Path(temporary)
    outbox = profile / 'Lobby2Stats'
    mailbox = profile / 'Lobby2PlayerHistory'
    outbox.mkdir()
    mailbox.mkdir()

    def publish(value):
        path = outbox / f"{value['missionId']}-{value['revision']}.json"
        path.write_text(json.dumps(value))
        Path(str(path) + '.ready').write_text('ready')

    mission = str(uuid.uuid4())
    current = str(uuid.uuid4())
    for value in [batch('returning', 5000, mission, final=False),
                  batch('returning', 7000, mission, revision=1),
                  batch('returning', 3000), batch('returning', 800, current),
                  batch('partial', 100, complete=False), batch('legacy', 100, version=2),
                  batch('interrupted', 100, final=False), batch('threshold', 10800)]:
        publish(value)

    log = (profile / 'service.log').open('w')
    process = subprocess.Popen(['dotnet', str(DLL), '--serve-profile', str(profile)], stdout=log, stderr=log)
    try:
        def request(identities, exclude=current):
            token = str(uuid.uuid4())
            path = mailbox / f'{token}.request'
            path.write_text(json.dumps(dict(version=1, token=token, missionId=exclude, identities=identities)))
            Path(str(path) + '.ready').write_text('ready')
            response = mailbox / f'{token}.response'
            deadline = time.monotonic() + 20
            while not response.exists() and time.monotonic() < deadline:
                assert process.poll() is None, (profile / 'service.log').read_text()
                time.sleep(.1)
            assert response.exists(), (profile / 'service.log').read_text()
            value = json.loads(response.read_text())
            assert value['token'] == token and value['missionId'] == exclude
            return value

        result = request(['new', 'returning', 'partial', 'legacy', 'interrupted', 'threshold'])
        assert result['seconds'] == [0, 10000, 100, 0, 100, 10800], result
        assert result['known'] == [True, True, False, False, False, True], result
        assert abs(result['generatedAt'] - time.time()) < 10
        # Same cumulative revision replay: no extra playtime or unique player.
        publish(batch('returning', 7000, mission, revision=1))
        assert request(['returning'])['seconds'] == [10000]
        assert sql("SELECT recorded_play_seconds FROM lobby2.player_history WHERE player_id='returning'") == '10800'
        assert sql('SELECT count(*) FROM lobby2.player_history') == '5'
        assert sql('SELECT unique_players || \',\' || first_seen_players || \',\' || returning_players FROM lobby2.player_population_daily') == '5,5,0'
        # A second day revisits one existing identity: daily returning differs from first-seen.
        next_day = batch('returning', 10)
        next_day['startedAt'] += 86400
        publish(next_day)
        assert request(['returning'])['seconds'] == [10010]
        assert sql('SELECT unique_players || \',\' || first_seen_players || \',\' || returning_players FROM lobby2.player_population_daily ORDER BY day DESC LIMIT 1') == '1,0,1'

        def publish_stream(folder, filename, payload):
            directory = profile / folder
            directory.mkdir(exist_ok=True)
            path = directory / filename
            path.write_text(json.dumps(payload))
            Path(str(path) + '.ready').write_text('ready')

        # Optional replay corruption must not disable usable player history.
        publish_stream('Lobby2Telemetry', 'invalid.json', {})
        assert request(['returning'])['seconds'] == [10010]
        assert (profile / 'Lobby2Telemetry/invalid.json.ready').exists()

        for identity, game in [('missing-game-stats', True), ('lobby-only', False)]:
            session = str(uuid.uuid4())
            event = dict(sequence=1, sessionSeconds=0, type='joined', actor=identity, actorName=identity,
                         target='', targetName='', weapon='', projectile='', magazine='', detail='',
                         reportedDamage=0, distanceMetres=-1, damageType=-1)
            events = dict(kind='sessionEvents', schemaVersion=1, sessionId=session, world='test.ent',
                          startedAt=1793491200, revision=0, closed=True, droppedRecords=0, events=[event])
            publish_stream('Lobby2SessionLogs', f'{session}-events-0.json', events)
            performance = dict.fromkeys(('samples', 'meanSampleFps', 'minimumSampleFps', 'maximumSampleFps',
                                         'samplesBelow15Fps', 'peakSampledPlayers', 'peakSampledActiveAI',
                                         'largestSampleGapSeconds'), 0)
            phases = [dict(phase='PREGAME', sessionSeconds=0, at=1793491200)]
            if game:
                phases.append(dict(phase='GAME', sessionSeconds=1, at=1793491201))
            history = dict(kind='missionHistory', schemaVersion=1, missionId=session, serverId=str(uuid.uuid4()),
                           revision=0, startedAt=1793491200, endedAt=1793491220, observedSeconds=20,
                           serverName='test', missionName='test', author='', world='test.ent', scenarioPath='',
                           description='', hostKind='dedicated', status='interrupted', outcome='unknown',
                           endReason=0, closed=True, complete=True, resultFactions=[], winningFactions=[],
                           factions=[], phases=phases, commanders=[], sessionPerformance=performance,
                           gamePerformance=performance)
            publish_stream('Lobby2History', f'{session}-history-0.json', history)
        result = request(['missing-game-stats', 'lobby-only'])
        assert result['known'] == [False, True], result
        assert result['seconds'] == [0, 0], result
        assert sql("SELECT unknown_playtime_missions FROM lobby2.player_history WHERE player_id='missing-game-stats'") == '1'
        print('PASS: schema idempotency; imports; latest revisions; retries; current-mission exclusion; '
              'unknown/partial/legacy/interrupted/missing histories; threshold; unique/new/returning aggregation; '
              'optional telemetry failure; confirmed lobby-only history')
    finally:
        process.terminate()
        process.wait(timeout=10)
        log.close()

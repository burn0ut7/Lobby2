"""Exercise the real importer against synthetic replay batches; never the live outbox.

Run after building integrations/StatsIngest in Release. Pass --database to also
exercise a disposable database named by LOBBY2_STATS_CONNECTION_STRING. The caller
must apply schema.sql first. These are contract tests, not engine/load evidence.
"""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
DLL = ROOT / "integrations/StatsIngest/bin/Release/net10.0/StatsIngest.dll"
MATRIX = [1, 0, 0, 0, 1, 0, 0, 0, 1, 1200, 40, 2700]
ZERO = [0, 0, 0]


def batch():
    examples = [
        ("spawn", dict(entity="life-1", kind="player", prefab="character.et", playerIdentity="test-player", faction="US", transform=MATRIX)),
        ("ownership", dict(entity="life-1", playerIdentity="test-player")),
        ("appearance", dict(entity="life-1", faction="US", revision=0, visualResource="character.xob", loadoutFormat="SCR_PlayerArsenalLoadout", loadout='{"items":[]}')),
        ("state", dict(entity="life-1", transform=MATRIX, velocity=[1, 0, 0], speedMps=1, aimRadians=ZERO, lookRadians=ZERO, stance=2, command=0, flags=3, lean=.5, dynamicStance=.2, movement=1, weaponPrefab="rifle.et", weaponIndex=1, sightsIndex=0, discontinuity=False)),
        ("seat", dict(entity="life-1", vehicle="", seatIndex=-1, compartmentType=-1)),
        ("skeleton", dict(skeletonId="body-1", visualResource="character.xob", bones=["root", "head"])),
        ("pose", dict(entity="life-1", skeletonId="body-1", localMatrices=[MATRIX, MATRIX], weaponRelativeTransform=MATRIX)),
        ("shot", dict(entity="life-1", projectileId="shot-1", weaponPrefab="rifle.et", projectilePrefab="bullet.et", origin=[1200, 41, 2700], velocity=[0, 0, 800], sessionEventSequence=5)),
        ("projectile-sample", dict(projectileId="shot-1", position=[1200, 41, 2710], velocity=[0, 0, 790])),
        ("impact", dict(projectileId="shot-1", target="", position=[1200, 40, 2800], normal=[0, 1, 0], evidence="engine-event")),
        ("grenade", dict(entity="life-1", projectileId="grenade-1", weaponPrefab="", projectilePrefab="grenade.et", origin=[1200, 41, 2700], velocity=[0, 10, 10], sessionEventSequence=6)),
        ("detonation", dict(projectileId="grenade-1", target="", position=[1200, 40, 2710], normal=ZERO, evidence="engine-event")),
        ("damage", dict(entity="life-1", attacker="", projectileId="grenade-1", reportedDamage=20, healthRemoved=10, damageType=1, position=[1200, 40, 2710], sessionEventSequence=7)),
        ("death", dict(entity="life-1", killer="", damageSequence=13, sessionEventSequence=8, cause="explosion")),
        ("gap", dict(fromMs=140, untilMs=150, track="state", reason="budget", droppedRecords=1)),
        ("despawn", dict(entity="life-1", reason="deleted")),
    ]
    return dict(kind="telemetry", schemaVersion=1, missionId=str(uuid.uuid4()), recordingId=str(uuid.uuid4()),
                revision=0, startedAt=1793145600, world="Arland.ent", gameVersion="1.8.0.13",
                assetManifestId="test-assets", clock="session-world-ms", coordinates="enfusion-world-metres",
                sampleHz=5, firstSequence=1, fromMs=0, untilMs=160, closed=True, stopReason="disabled", droppedRecords=1,
                coverage=dict(actors="players-and-ai", pose="skeletal", shots=True, impacts=True, damage=True, grenades=True, vehicles=False, gear=True),
                records=[dict(sequence=i + 1, timeMs=i * 10, type=kind, data=data) for i, (kind, data) in enumerate(examples)])


def write(directory, value, raw=None):
    path = directory / f"{value['recordingId']}-telemetry-{value['revision']}.json"
    path.write_text(raw if raw is not None else json.dumps(value), encoding="utf-8")
    Path(str(path) + ".ready").write_text("ready", encoding="utf-8")
    return path


def run(directory, valid, database=False):
    result = subprocess.run(["dotnet", str(DLL), "--once" if database else "--validate", str(directory)], capture_output=True, text=True)
    assert (result.returncode == 0) == valid, result.stdout + result.stderr
    return result


count = 0
with tempfile.TemporaryDirectory(prefix="Lobby2TelemetryContract-") as temp:
    directory = Path(temp)

    def check(name, mutate=None, valid=False, raw=None):
        global count
        case = directory / name
        case.mkdir()
        value = batch()
        if mutate:
            mutate(value)
        path = write(case, value, raw(value) if raw else None)
        run(case, valid)
        assert path.exists() and Path(str(path) + ".ready").exists(), "Validation must retain evidence"
        count += 1

    check("all-record-types", valid=True)
    check("future-version", lambda b: b.update(schemaVersion=2))
    check("invalid-clock", lambda b: b.update(clock="wall-time"))
    check("missing-asset-manifest", lambda b: b.pop("assetManifestId"))
    check("invalid-initial-sequence", lambda b: b.update(firstSequence=2))
    check("backwards-time", lambda b: b["records"][3].update(timeMs=1))
    check("out-of-window", lambda b: b["records"][-1].update(timeMs=999))
    check("sequence-hole", lambda b: b["records"][3].update(sequence=99))
    check("unknown-event", lambda b: b["records"][0].update(type="invented"))
    check("truncated-transform", lambda b: b["records"][0]["data"].update(transform=[0] * 11))
    check("invalid-seat", lambda b: b["records"][4]["data"].update(vehicle="vehicle-1"))
    check("invalid-lean", lambda b: b["records"][3]["data"].update(lean=2))
    check("invalid-state-flags", lambda b: b["records"][3]["data"].update(flags=32))
    check("vehicle-state", lambda b: b["records"][3].update(type="vehicle-state", data=dict(entity="vehicle-1", transform=MATRIX, velocity=[0, 0, 12], speedMps=12, discontinuity=False)), valid=True)
    check("estimated-impact", lambda b: b["records"][9]["data"].update(evidence="last-position"))
    check("duplicate-bones", lambda b: b["records"][5]["data"].update(bones=["root", "root"]))
    check("empty-pose", lambda b: b["records"][6]["data"].update(localMatrices=[]))
    check("negative-health-loss", lambda b: b["records"][12]["data"].update(healthRemoved=-1))
    check("unexplained-closure", lambda b: b.update(stopReason=""))
    check("false-closure", lambda b: b.update(closed=False))
    check("empty-open-batch", lambda b: b.update(records=[], closed=False, stopReason=""))
    check("empty-closed-batch", lambda b: b.update(records=[]), valid=True)
    check("too-many-records", lambda b: b.update(records=b["records"] * 65))
    check("too-large", raw=lambda b: json.dumps(b)[:-1] + ', "padding":"' + 'x' * (1024 * 1024) + '"}')
    check("duplicate-field", raw=lambda b: json.dumps(b)[:-1] + ', "schemaVersion":1}')
    check("overflow-number", raw=lambda b: json.dumps(b).replace('"speedMps": 1', '"speedMps": 1e400'))

    if "--database" in sys.argv:
        assert os.environ.get("LOBBY2_STATS_CONNECTION_STRING"), "Use a disposable test database"
        case = directory / "database"
        case.mkdir()
        original = batch()
        original.update(closed=False, stopReason="")
        second = copy.deepcopy(original)
        second.update(revision=1, firstSequence=17, fromMs=160, untilMs=170, closed=True, stopReason="disabled", records=[])
        final_path = write(case, second)
        run(case, True, True)  # Final batch may arrive first.
        first_path = write(case, original)
        run(case, True, True)
        assert first_path.exists() and final_path.exists(), "Replay archive must remain after ingestion"
        assert not list(case.glob("*.ready")), "Committed markers should be consumed"
        write(case, original)
        run(case, True, True)  # Identical retry.
        count += 3
        for name, mutate in [
            ("checksum-conflict", lambda b: b["records"][0]["data"].update(faction="USSR")),
            ("changed-mission", lambda b: b.update(missionId=str(uuid.uuid4()))),
            ("after-close", lambda b: b.update(revision=2, firstSequence=17, records=[], fromMs=170, untilMs=180, closed=True, stopReason="disabled")),
        ]:
            invalid = copy.deepcopy(original)
            mutate(invalid)
            isolated = directory / name
            isolated.mkdir()
            path = write(isolated, invalid)
            run(isolated, False, True)
            assert path.exists() and Path(str(path) + ".ready").exists()
            count += 1

        # Gapped arrival order must check the nearest stored neighbors, even
        # when neither one is adjacent and neither is the global boundary.
        sparse = batch()
        sparse.update(records=[sparse["records"][0]], closed=False, stopReason="")

        def sparse_batch(revision, start, first, drops):
            value = copy.deepcopy(sparse)
            value.update(revision=revision, fromMs=start, untilMs=start + 10, firstSequence=first, droppedRecords=drops)
            value["records"][0].update(sequence=first, timeMs=start)
            return value

        for revision, start, first, drops in [(0, 0, 1, 0), (4, 40, 41, 4), (8, 80, 81, 8)]:
            write(case, sparse_batch(revision, start, first, drops))
            run(case, True, True)
            count += 1
        for name, value in [
            ("sparse-time", sparse_batch(2, 55, 21, 2)),
            ("sparse-sequence", sparse_batch(2, 20, 55, 2)),
            ("sparse-drops", sparse_batch(2, 20, 21, 6)),
            ("sparse-before-time", sparse_batch(6, 30, 61, 6)),
            ("sparse-before-sequence", sparse_batch(6, 60, 31, 6)),
            ("sparse-before-drops", sparse_batch(6, 60, 61, 2)),
        ]:
            isolated = directory / name
            isolated.mkdir()
            path = write(isolated, value)
            run(isolated, False, True)
            assert path.exists() and Path(str(path) + ".ready").exists()
            count += 1

print(f"PASS {count} telemetry cases (synthetic contract evidence only)")

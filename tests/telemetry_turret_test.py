"""Exercise schema-2 turret-state validation in the real StatsIngest CLI.

Run after building integrations/StatsIngest in Release. Uses only synthetic
temporary batches and leaves their evidence in place until the temp directory
is removed.
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


def batch(turrets=None, available=True, schema=2):
    return dict(kind="telemetry", schemaVersion=schema, missionId=str(uuid.uuid4()),
        recordingId=str(uuid.uuid4()), revision=0, startedAt=1793145600,
        world="Arland.ent", gameVersion="1.8.0.13", assetManifestId="test-assets",
        clock="session-world-ms", coordinates="enfusion-world-metres", sampleHz=5,
        firstSequence=1, fromMs=0, untilMs=10, closed=True, stopReason="disabled",
        droppedRecords=0,
        coverage=dict(actors="players-and-ai", pose="inferred" if schema == 2 else "skeletal",
            shots=True, impacts=True, damage=True, grenades=True, vehicles=False, gear=True),
        records=[dict(sequence=1, timeMs=0, type="turret-state", data=dict(
            entity="entity-1", available=available, turrets=turrets or []))])


def entries():
    return [
        dict(managerId=10, slotId=20, seatIndex=0, compartmentType=1,
            hasWeapon=True, directionAvailable=True, direction=[0, 0, 1]),
        dict(managerId=11, slotId=21, seatIndex=1, compartmentType=1,
            hasWeapon=False, directionAvailable=True, direction=[1, 0, 0]),
    ]


def check(folder, name, value, valid):
    case = folder / name
    case.mkdir()
    path = case / f"{value['recordingId']}-telemetry-0.json"
    path.write_text(json.dumps(value), encoding="utf-8")
    ready = Path(str(path) + ".ready")
    ready.write_text("ready", encoding="utf-8")
    result = subprocess.run(["dotnet", str(DLL), "--validate", str(case)], capture_output=True, text=True)
    assert (result.returncode == 0) == valid, name + ": " + result.stdout + result.stderr
    assert path.exists() and ready.exists(), name + ": validation must retain evidence"


with tempfile.TemporaryDirectory(prefix="Lobby2TelemetryTurret-") as temp:
    root = Path(temp)

    def valid_case(name, turrets, available=True):
        check(root, name, batch(turrets, available), True)

    def invalid_case(name, mutate, source=None):
        value = copy.deepcopy(source if source is not None else batch(entries()))
        mutate(value["records"][0]["data"])
        check(root, name, value, False)

    valid_case("two-turrets", entries())
    valid_case("available-empty", [])
    valid_case("unavailable-empty", [], available=False)
    signed_manager = entries()
    signed_manager[0]["managerId"] = -2147470098
    valid_case("signed-manager-id", signed_manager)
    no_direction = entries()
    no_direction[0]["directionAvailable"] = False
    no_direction[0].pop("direction")
    valid_case("direction-unavailable", no_direction)

    invalid_case("duplicate-pair", lambda d: d["turrets"][1].update(managerId=10, slotId=20))
    invalid_case("zero-direction", lambda d: d["turrets"][0].update(direction=[0, 0, 0]))
    invalid_case("nonunit-direction", lambda d: d["turrets"][0].update(direction=[2, 0, 0]))
    invalid_case("missing-direction", lambda d: d["turrets"][0].pop("direction"))
    invalid_case("negative-slot", lambda d: d["turrets"][0].update(slotId=-1))
    invalid_case("unavailable-nonempty", lambda d: d.update(available=False))
    invalid_case("too-many-turrets", lambda d: d.update(turrets=[
        dict(managerId=i + 1, slotId=i + 1, seatIndex=0, compartmentType=1,
            hasWeapon=False, directionAvailable=False) for i in range(65)]))
    schema1 = batch(entries(), schema=1)
    invalid_case("schema1-turret-state", lambda d: None, source=schema1)

    # The production binary tool performs an exact logical-data roundtrip.
    sample = root / "binary-input.json"
    expected = batch(entries())
    sample.write_text(json.dumps(expected), encoding="utf-8")
    output = root / "binary-output"
    encoded = subprocess.run(["dotnet", str(DLL), "--binary-benchmark", str(sample), str(output)],
        capture_output=True, text=True)
    assert encoded.returncode == 0, encoded.stdout + encoded.stderr
    report = json.loads((output / "benchmark.json").read_text(encoding="utf-8"))
    assert report["roundTripExactLogicalData"] is True
    packet = output / "binary-input.l2t"
    assert packet.exists()

    # Exercise the browser-compatible decoder against the encoded turret batch.
    decoder = ROOT / "integrations/StatsIngest/web/telemetry-binary.mjs"
    browser_check = """
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const { decodeTelemetry } = await import(process.env.TURRET_DECODER);
const actual = decodeTelemetry(readFileSync(process.env.TURRET_PACKET));
const expected = JSON.parse(readFileSync(process.env.TURRET_JSON, 'utf8'));
assert.deepStrictEqual(JSON.parse(JSON.stringify(actual)), expected);
"""
    env = dict(os.environ, TURRET_DECODER=decoder.as_uri(),
        TURRET_PACKET=str(packet), TURRET_JSON=str(sample))
    decoded = subprocess.run(["node", "--input-type=module", "-e", browser_check],
        capture_output=True, text=True, env=env)
    assert decoded.returncode == 0, decoded.stdout + decoded.stderr

print("PASS 13 turret-state cases including signed manager IDs, plus binary roundtrip and browser decoder preservation")

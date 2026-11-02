"""Validate real outbox batches and delayed-credit compatibility without a database."""
import argparse
import copy
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--assembly", type=Path, required=True)
parser.add_argument("--profile", type=Path, required=True)
parser.add_argument("--session", required=True)
args = parser.parse_args()
assembly = args.assembly.resolve()

with tempfile.TemporaryDirectory(prefix="Lobby2-credit-") as temporary:
    root = Path(temporary)
    native = root / "native"
    native.mkdir()
    samples = {}
    count = 0
    for folder in ("Lobby2Stats", "Lobby2SessionLogs"):
        for path in (args.profile / folder).glob(args.session + "*.json"):
            marker = Path(str(path) + ".ready")
            if not marker.exists():
                continue
            shutil.copy2(path, native / path.name)
            shutil.copy2(marker, native / marker.name)
            payload = json.loads(path.read_text(encoding="utf-8-sig"))
            field = "events" if "sessionId" in payload else "deaths"
            for index, event in enumerate(payload[field]):
                if event.get("creditSource") == "lastDamage":
                    samples[field] = (payload, index, path.name)
            count += 1
    assert count and len(samples) == 2, "Need native session and mission delayed-credit deaths"

    def validate(directory, accepted):
        run = subprocess.run(["dotnet", str(assembly), "--validate", str(directory)], capture_output=True, text=True)
        assert (run.returncode == 0) == accepted, run.stdout + run.stderr

    validate(native, True)
    cases = 0
    for field, (payload, index, filename) in samples.items():
        variants = [(cause, source, True) for cause in ("bleeding", "respawn", "suicide")
                    for source in ("lastDamage", "lastPlayerDamage", "engine", "")]
        variants += [(None, None, True), ("invalid", "lastDamage", False), ("suicide", "invalid", False)]
        for cause, source, accepted in variants:
            case = root / str(cases)
            case.mkdir()
            changed = copy.deepcopy(payload)
            event = changed[field][index]
            if cause is None:
                for name in ("deathCause", "creditSource", "damageEventSequence"):
                    event.pop(name, None)
            else:
                event.update(deathCause=cause, creditSource=source)
            path = case / filename
            path.write_text(json.dumps(changed), encoding="utf-8")
            Path(str(path) + ".ready").write_text("ready", encoding="utf-8")
            validate(case, accepted)
            assert path.exists(), "Validation must retain batches"
            cases += 1
    print(f"PASS: {count} native batches and {cases} compatibility/rejection cases")

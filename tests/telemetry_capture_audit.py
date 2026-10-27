"""Read-only audit of native telemetry outboxes; never consumes ready markers."""
import argparse
import collections
import gzip
import hashlib
import json
import math
import statistics
from pathlib import Path


def audit(paths):
    batches = sorted([(json.loads(p.read_text(encoding="utf-8")), p) for p in paths], key=lambda pair: pair[0]["revision"])
    first, last = batches[0][0], batches[-1][0]
    records = [record for batch, _ in batches for record in batch["records"]]
    counts = collections.Counter(record["type"] for record in records)
    sequences = [record["sequence"] for record in records]
    times = [record["timeMs"] for record in records]
    states = [r for r in records if r["type"] == "state"]
    tracks = collections.defaultdict(list)
    for record in states:
        tracks[record["data"]["entity"]].append(record["timeMs"])
    intervals = [b - a for track in tracks.values() for a, b in zip(track, track[1:])]
    raw = sum(p.stat().st_size for _, p in batches)
    duration = (last["untilMs"] - first["fromMs"]) / 1000
    shot_ids = {r["data"]["projectileId"] for r in records if r["type"] in ("shot", "grenade")}
    damage_ids = {r["sequence"] for r in records if r["type"] == "damage"}
    damage = [r["data"] for r in records if r["type"] == "damage"]
    deaths = [r["data"] for r in records if r["type"] == "death"]
    return {
        "schemaVersion": first["schemaVersion"], "recordingId": first["recordingId"], "missionId": first["missionId"],
        "seconds": duration, "batches": len(batches), "rawBytes": raw,
        "gzipBytesPerBatch": sum(len(gzip.compress(p.read_bytes(), mtime=0)) for _, p in batches),
        "bytesPerSecond": raw / duration if duration else None,
        "closed": last["closed"], "stopReason": last["stopReason"], "droppedRecords": last["droppedRecords"],
        "contiguousRevisions": [b["revision"] for b, _ in batches] == list(range(len(batches))),
        "contiguousSequences": sequences == list(range(1, len(records) + 1)),
        "monotonicTimes": all(a <= b for a, b in zip(times, times[1:])),
        "recordCounts": dict(counts), "coverage": first["coverage"],
        "spawns": [r["data"] for r in records if r["type"] == "spawn"],
        "stances": sorted({r["data"]["stance"] for r in states}),
        "flags": sorted({r["data"]["flags"] for r in states}),
        "leanRange": [min((r["data"]["lean"] for r in states), default=0), max((r["data"]["lean"] for r in states), default=0)],
        "maxSpeedMps": max((math.sqrt(sum(v * v for v in r["data"]["velocity"])) for r in states), default=0),
        "medianStateIntervalMs": statistics.median(intervals) if intervals else None,
        "maxStateIntervalMs": max(intervals, default=None),
        "maxObservationWindowMs": max((r["timeMs"] - r["data"]["previousSampleMs"] for r in states if "previousSampleMs" in r["data"] and not r["data"]["discontinuity"]), default=None),
        "statesAfterDeath": sum(r["sequence"] > death["sequence"] and r["data"]["entity"] == death["data"]["entity"] for r in records if r["type"] in ("state", "vehicle-state", "pose") for death in records if death["type"] == "death"),
        "bonesPerSkeleton": [len(r["data"]["bones"]) for r in records if r["type"] == "skeleton"],
        "seatChanges": [r["data"] for r in records if r["type"] == "seat"],
        "linkedProjectileDamageCount": sum(d["projectileId"] in shot_ids for d in damage),
        "linkedDamageDeathCount": sum(d["damageSequence"] in damage_ids for d in deaths),
        "firstLaunch": next((r for r in records if r["type"] == "shot"), None),
        "grenades": [r for r in records if r["type"] == "grenade"],
        "files": [{"path": str(p.resolve()), "bytes": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()} for _, p in batches],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outbox", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    groups = collections.defaultdict(list)
    for path in args.outbox.glob("*-telemetry-*.json"):
        if not Path(str(path) + ".ready").exists():
            continue
        groups[json.loads(path.read_text(encoding="utf-8"))["recordingId"]].append(path)
    result = [audit(paths) for _, paths in sorted(groups.items())]
    encoded = json.dumps(result, indent=2)
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded)
    if any(not r["contiguousRevisions"] or not r["contiguousSequences"] or not r["monotonicTimes"] for r in result):
        raise SystemExit("Non-contiguous capture; see audit output")


if __name__ == "__main__":
    main()

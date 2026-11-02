# Lobby2 telemetry contracts

Canonical .NET validation and L2TB codec shared by StatsIngest and the GC website.
The engine recorder and on-disk formats are unchanged by this extraction.

- `TelemetryBatch.Read` reads bounded JSON or L2TB and validates the logical batch.
- `TelemetryContract.Validate` owns schema 1/2 rules.
- `TelemetryBinary` owns L2TB v1 numeric encoding and decoding.
- `TelemetryContinuity.Validate` checks adjacent/nearest batch boundaries. Callers
  must group the same recording and provide counts from validated records. Storage
  owns immutable-byte checksums, transactions and recording-level serialization.

`StatsIngest` references this project directly. The website consumes an immutable
NuGet package built from a committed revision; it does not maintain another codec.

```powershell
dotnet build integrations/StatsIngest -c Release
python tests/telemetry_contract_test.py
dotnet pack integrations/TelemetryContracts -c Release -o <package-output>
```

Change the package version when releasing changed code. Record the source commit
and package digest in the consumer; never replace an already released version.
Synthetic contract checks do not prove capture fidelity or live game performance.

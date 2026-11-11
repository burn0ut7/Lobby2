# Mission voting

## Catalog configuration

The dedicated server reads `$profile:Lobby2Voting.json`, kept private on the machine and outside the addon package:

```json
{
  "url": "https://catalog.example/",
  "apiKey": "<same private machine key>"
}
```

The values are placeholders. Use the website base URL; a trailing slash is added when needed. The key is the same private machine key configured for the website's ingest service. Restrict file access to the dedicated server service account. Players use game owner RPCs; they never receive HTTP access or the key.

The server fetches `GET api/v1/catalog/voting?offset=0` using bearer authentication. The endpoint is server-only and returns pages of 25. Voting candidates are listed available catalog `.conf` headers with valid GUIDs. The game checks installed headers before offering them. Metadata `previewResource` values must name installed game resources, not web URLs.

## Player flow

At `POSTGAME`, the server samples the connected player count once when selecting the ballot. The top three slots randomly select missions with maximum capacity at or above that count; the bottom three randomly select capacities at or below it. Every eligible mission in a group has the same chance: there is no closest-match ranking, 20% cutoff or minimum-player exclusion. Exact-current capacities can appear in either row, once per ballot. If a group is short, it borrows randomly from remaining known capacities, then missions with unknown counts. Only a catalogue with fewer than six installed missions leaves empty slots; those slots cannot receive votes or win. The choices remain stable when players join or leave during voting. Each identity has one vote that can be changed. After 120 seconds, a random tie-break selects a winner; zero votes is also resolved randomly. The winner is revealed for 3 seconds, then the native game transition starts it and retains addons.

The first screen is the vote screen. **Debrief** opens stats; **Back** returns to the vote screen. There is no header tab.

Website session counts and last-started UTC time come from existing `catalog_sessions` records in `running` or `completed` state. The catalog is fetched when POSTGAME starts, without postponing the two-minute deadline.

## Validation

Broad random groups supersede the earlier strict-band checks below. Runtime `44a9a30e52654ce896655701f70e34c4` passed with zero failures: top/bottom capacities far outside 20% remain eligible, sparse groups borrow remaining choices, unknown counts fill shortages, choices are unique, empty slots cannot win, and the production selector uses the native connected count. No nearest-capacity ranking remains.

Earlier strict population row selection (superseded by broad random groups): Workbench 1.9.0.37 Reload compiled without script errors (existing Arland resource GUID diagnostics remain). `GC_TestVotingPopulation` passed 17 checks with zero failures, covering inclusive and fractional bounds, missing/invalid ranges, sparse rows, empty-slot winner exclusion and the actual connected-player selector (one connected player). Operation: `892084c060c546f4a453e5294f47fb66`. An explicitly synthetic 50-player fixture rendered top capacities 60/55/50 and bottom 45/48/40; native card activation recorded one ballot. Screenshot: `Art/References/voting-population-bands/voting.png`. This is embedded Play verification, not a 50-client multiplayer test.

Workbench 1.9.0.37: scripts compiled; production POSTGAME opened voting; six choices loaded through the authenticated website route; voting immediately after polling recorded one ballot; changing and repeating choices kept one ballot per identity; stale and invalid choices were rejected. Continue and Back preserved the ballot and countdown. Long descriptions and tag lists scrolled, and the revised card layout was captured in `Art/References/mission-voting/live-voting-revised.png` and independently reviewed.

A real, unintercepted ballot entered POSTGAME at 04:00:50.421, shut down the old manager at 04:02:53.433 and started the winning fixture scenario at 04:02:54.453. The new world initialized and the local player reconnected in PREGAME. Evidence: Workbench `logs_2026-11-11_02-27-33/console.log`, lines 31810–31963. This establishes embedded offline Play, not dedicated-server or separate-client behavior.

Separate-client replication and dedicated-server transitions remain unverified: the MCP multiplayer native profile does not support this Workbench version. Native investigation did not yet establish all current plugin-owner/reflection offsets; unverified offsets were not dispatched.

The optional test addon and `GC.Infrastructure.Tests --voting-live <profile> <addon>` use a disposable isolated catalog database and temporary private profile configuration. They do not create fictional production missions. The fixture restores the previous profile configuration on normal exit.

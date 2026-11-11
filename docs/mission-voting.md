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

At `POSTGAME`, the server presents six random installed choices, or fewer when fewer are available. Each identity has one vote that can be changed. After 120 seconds, a random tie-break selects a winner; zero votes is also resolved randomly. The winner is revealed for 3 seconds, then the native game transition starts it and retains addons.

The first screen is the vote screen. **Continue** opens stats; **Back** returns to the vote screen. There is no header tab.

Website session counts and last-started UTC time come from existing `catalog_sessions` records in `running` or `completed` state. The catalog is fetched when POSTGAME starts, without postponing the two-minute deadline.

## Validation

Workbench 1.9.0.37: scripts compiled; production POSTGAME opened voting; six choices loaded through the authenticated website route; voting immediately after polling recorded one ballot; changing and repeating choices kept one ballot per identity; stale and invalid choices were rejected. Continue and Back preserved the ballot and countdown. Long descriptions and tag lists scrolled, and the revised card layout was captured in `Art/References/mission-voting/live-voting-revised.png` and independently reviewed.

A real, unintercepted ballot entered POSTGAME at 04:00:50.421, shut down the old manager at 04:02:53.433 and started the winning fixture scenario at 04:02:54.453. The new world initialized and the local player reconnected in PREGAME. Evidence: Workbench `logs_2026-11-11_02-27-33/console.log`, lines 31810–31963. This establishes embedded offline Play, not dedicated-server or separate-client behavior.

Separate-client replication and dedicated-server transitions remain unverified: the MCP multiplayer native profile does not support this Workbench version. Native investigation did not yet establish all current plugin-owner/reflection offsets; unverified offsets were not dispatched.

The optional test addon and `GC.Infrastructure.Tests --voting-live <profile> <addon>` use a disposable isolated catalog database and temporary private profile configuration. They do not create fictional production missions. The fixture restores the previous profile configuration on normal exit.

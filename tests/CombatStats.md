# Player and AI combat breakdown

Player kills, AI kills, player team kills, total deaths, deaths by players, deaths by AI and other deaths are separate debrief rows. Original `kills` and `deaths` fields retain their meanings. New counters flow through snapshots, membership differences, faction totals and cached personal reports. Other deaths are derived from total minus the two credited subsets. AI-only combat without a player participant is outside this slice.

## Source and runtime evidence

Installed catalogue `af6068cb44aa4c0db613213faa82a050`, `scripts/Game/Damage/SCR_InstigatorContextData.c:35–62,260–310`: native control types distinguish AI, players, possessed characters and GM actions. A retained tracker identifies a disconnected player's body before applying AI classification. Shared global hooks were not added.

Workbench 1.8.0.13, PID 48584, Arland Lobby with Lobby2 MCP Tests. Reload `bf59338ef6324eefbc279b5cface5c5f` passed. Session `01a17c4d-4002-86b1-8400-00000023b794` used the host and a separate native peer:

- Player kills a native AI: `r2e34610e368641c6b6bc88b9f0029d4f`; snapshot `rbb153eb2ee9d4e888404393db41fa305` shows aiKills=1, player kills=0, and an AI-victim death record.
- Player kills peer: `re3fa74f8df0d4e4b86dda2bec7bfc83c`; AI kills host: `r857ef4c808744693a83f0e0691e1751e`. Snapshot `r1e9925990e0c49ae868ff4d26ef71c86`: host player kills=1, AI kills=1, team kills=1, deaths=1/byAI=1; peer deaths=1/byPlayers=1. Native weapon references are present for player-victim deaths.
- Restored host takes native AI damage `rf8d674968ec948bbb4e02cd2ab005980`, then stock bleeding `r4a7b2e6ab6fd405797f77b84111b3c06`. Snapshot `r19d90469f6a34b3b88bdaab3108b5aa5` shows host deaths=2/byAI=2. Fresh-life uncredited respawn `rbeb720e58dda496ab0b9adda9230318a`, report `r9b4c7276babb4faf88290424761df8ca`, snapshot `re8d753f3db6d4901be31722d048d1ddc`: deaths=3, byAI still 2, byPlayers=0, so other deaths=1.
- PostgreSQL 17.11 temporary database: applied previous schema, upgraded to current schema, reapplied current schema, then imported 18 actual engine stats/session batches. Player totals matched the above. USSR faction totals: player kills=1, AI kills=1, deaths=4, byPlayers=1, byAI=2, other=1. Copies retained at `%TEMP%/Lobby2StatsPostgres-test/Combat-f82be185edfa48ec8966f8da82061e3a`; test database stopped afterward.

Final reload `50e3c0ae602f4fd199e35f95e8f2c2b0` passed after fixing inverted layout bottom offsets and adding readable AI log labels/actor metadata. Session `01a17c50-ca9f-8ae4-8400-0000002515ae`: native AI kill `r58f9b74953a041f5a85656d2db926828`, AI kills host `rbfec3b53a99e40b1910403bdc11ee5bb`, snapshot `r2330cd25fde8454b8dd359d557a37baf` gives aiKills=1, deaths/byAI=1. Persisted events 7/8/9 separately identify AI kill, AI damage and player death; AI references and prefab fields are preserved.

## Presentation and contracts

The first scroll implementation created negative-height rows; MCP geometry inspection exposed the error and the corrected captures below show rendered rows. Existing concept `Art/References/mission-debrief/concept-v2.png` remains the composition reference; individual combat rows extend its left summary into a native scroll area. Ammunition, privacy/faction selection, report scope and Back remain in place.

- [Top rows](../Art/References/mission-debrief/runtime-combat-top.png) and [bottom rows](../Art/References/mission-debrief/runtime-combat-bottom.png) are actual MCP captures. Native scroll receipt `r60e8b482db8b4b0387eedc85a0f61d5b`; personal readback `rfb727d662cbe416885f019a1b6b9dc36` and faction readback `r65e6c277999543d6a4f3439d84c4ae4f` agree with recorded counters. The faction's final row says Participants; personal says Command.
- Cache cycling `ree13e67377d14049a1173f42438d50b0` passed every rendered value: three cached reports, one server request, three deliveries. No request per selection was introduced.
- [AI kill log filter](../Art/References/admin-logs/runtime-ai-kill.png), filter `r0ba7715a0272423cae2ef65dac7234d3`, readback `r6c28b48c299748d596970aad9f768506`: one readable AI kill row with actor and distance. Deaths (all) includes AI kills as well.
- Release importer build: zero errors/warnings. Eight final-run batches validated. Negative AI count, missing death subset, and subsets exceeding deaths were rejected with files and markers retained (`%TEMP%/Lobby2CombatContract-_wuhnu1d`). Absent new fields in older archives remain supported; SQL coverage counts distinguish their missing breakdowns.

Limitations: fixtures apply native melee damage and stock bleeding, not ballistic AI firefights; no dedicated-server load test or medical-mod model coverage. Scrolling was exercised through the native widget setter; mouse-wheel/scrollbar hit testing and keyboard/gamepad navigation remain unverified because the connected MCP surface does not expose those game inputs. AI-victim weapon attribution is unavailable (empty field); no speculative equipped-weapon attribution is used. Existing last-player-damage precedence on bleed-out/respawn remains intentional even if AI also damages the same life. The final placeholder cleanup restores plain ASCII hyphens for pending rows after review found a text-encoding replacement glyph.

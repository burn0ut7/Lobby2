# Boundary client presentation — 2026-10-25

## Ownership

The server still owns enrollment, eligibility, grace deadlines, deaths, hard-boundary
correction, setup restrictions, admin authorization, and which shapes each player may
receive. The existing 100 ms enforcement cadence is unchanged.

Clients retain one permitted shape snapshot across map closes. Opening, framing,
projection, fill geometry, wall rendering, HUD layout, and countdown animation are
local. No recurring map request or timer heartbeat is needed. The server checks map
revision and live faction/admin visibility every 500 ms per subscriber and publishes
only changes. This deliberately keeps a small authoritative visibility check instead
of adding a second faction cache or a broad event framework.

Timer transitions carry the existing engine WorldTimestamp to account for transport
delay. Possession and entering GAME request current presentation once. Remote map
subscription starts after ownership; the host initializes locally. Map callbacks stop
when the map closes. An unchanged open view does not rebuild geometry.

Vehicle occupant lists are rebuilt from native possession each enforcement pass;
hard-boundary validation scans that mover's occupants and rechecks actual membership.
This is an iteration index, not a second source of player membership. Boundary-wall
rendering was already local; its audience comparison is now linear.

## Verification

Production handlers were exercised through the optional MCP test addon in Arland,
using embedded Play and a separate diagnostic client connected to localhost.
Final reviewed scripts compiled through operation `d336e08379d14d849b1355ab947ecfd9`.
The test addon records calls to production presentation rather than replacing it.
Raw runtime receipts are in `evidence/boundary-client-presentation-2026-10-25.json`.

| Check | Observation / receipt |
| --- | --- |
| Closed-map steady state | All presentation counters zero; setup remained active and counted down. `r40e3f7802e60460e8f14cfbba661b321` |
| Static host map | 989 local sizing ticks, zero snapshots/projections/rebuilds. `rbf2a1a4b9c2445db9660ca240d9e6a84` |
| Manual pan persists | Close/reopen retained zoom 2 and the manually selected center. `re328dffc1eb64b21b9a129b91df68579` |
| Setup and AO together | Both warning timers count down without repeated messages; re-entry clears warnings. `r459b162cdc5441208ca54b547dc76d9c` |
| Phase ordering | Setup label Waiting → Active, setup/combat active on GAME. `r04e154aed2b94bfd818292adaacc6646` |
| Remote subscription | Server subscribed=1; remote receives initial snapshot while map closed. `r1c6b3246f885477695d9337e96d05a23`, `r5afaefa7b0bf4008a7de901ac0321470` |
| Remote briefing | One shape, two draw commands, framed=1. `r7f18a86379c14cf5b4a8021a5751a250` |
| Remote visibility | Changing fixture audience US → USSR removes it from the US player's open map (two shapes → one). `r3ad2352ff8fb42c28225740b0a8fb2bf`, `r2b7920c956f74678a1348dd9efa429d7` |
| Remote steady map | 334 local ticks, zero snapshots/projections/rebuilds/setup updates. `rceee4651336e47caa380b7a09b6bc057` |
| Remote timer changes | Add 30 seconds applied by server and shown remotely; ending setup clears setup/combat. `r334f81316a5f46f9a52aad8b40444aae`, `r21fbbe3f680d47c89da9c3cf00d1ba9f`, `re499efb2d62e40e5b59cc0b5c0b1b94a` |
| Remote AO warning | 59.840 → 48.134 seconds with zero further warnings and zero closed-map ticks. `r0b4f929dd7c4421ca450eac14984b55b`, `r36800c4dd3fb4bc997d7787ff7f4747e`, `rf9053620fe4643ddbf77cfd23f153836` |
| Shared vehicle | Host and remote occupied the same M1025; arranging it beyond the hard AO returned it to the saved inside position with both occupants alive. `r42087d1ec0994432a06f9cd4414178e2`, `r87191c5a6b4042e597084d9287d2dca2`, `rb8f90bd50d7044df9d28fc613e85ff3a`, `r5170da66c4a149d3a782229b62015c66` |
| Reconnect during GAME | New remote controller subscribed, received its initial closed-map snapshot and active setup/combat state; timer showed 9:32. `r91bf1bec7b414c6a8a8a548e2d59b1e4`, `r9fb3c8213361436db949e8846f2c8419`, `r1c72fbe177ac446fa8a46eb548b9ab45` |
| Admin rejection | Unauthenticated setup extension rejected by production gate. `r6bcd46a7fbef47edbce9e053333691b0` |
| Stop after reconnect | Stop-all ended all setups and cleared remote setup/combat. `r4f5ab55fa2c541749fe4461cfe0e5855`, `r168b7b3428534aee9292afe4451275a3` |

MCP screenshots: `Art/References/boundary-client-presentation/runtime-hud.png`
(stacked setup/AO warnings) and `peer-map.png` (separate client, framed map and timer).
These ignored local references do not ship in the addon.

## Findings and limits

Remote testing caught an early map subscription during controller initialization.
Waiting for completed remote ownership fixed the missing initial snapshot. Review
also caught setup-start label invalidation and early GAME-phase timer delivery;
both have targeted lifecycle fixes and live verification above.

The saved peer launch configuration omitted base dependencies and the nested test
addon. Failed attempts produced missing-dependency and addon-checksum errors; the
successful run explicitly included both paths. No connection validation was disabled.

Two explicit final-class casts in admin-freeze scripts fix pre-existing modded-layer
compile errors encountered when compiling the addon; no admin behavior was changed.

These are behavior and call-count checks, not a many-player server benchmark or a
measured CPU/frame-time improvement. The vehicle check uses a controlled boundary
crossing, not a high-speed collision or aircraft stress test. No fresh unrelated-mode
baseline or dedicated-server run was performed for this change. Shared vanilla
overrides were not changed. Existing warnings about loadout managers and deprecated
base APIs remain outside this slice. Reviewed scripts retain callback/RPC/reflection
names and existing tagged naming, tabs, Allman blocks and method separators;
`git diff --check` passes.

Independent review approved the source, receipts and screenshots. Peers were stopped
with no surviving processes; the original peer launch parameters were restored.
Closing World Editor resets the temporary localhost Play setting.

# Network spectator

The NETWORK camera streams one versioned `GC_SpectatorSample` from the observed
player's owner through the server to subscribed spectator owners. The server
checks the spectator role, publisher's playing state, and current character
replication identity. The receiver also checks identity, sequence, source time,
and bounded sample values. A two-second silence returns to free camera.

The sample contains world position, orientation and velocity; stance and dynamic
stance; movement and animation command; signed lean; aim/look angles; raised,
ADS, reload, sprint and freelook flags; camera offset/basis; and sight/zoom/FOV
state. Version 3 retains stock reticle/edge scales and sight-local ADS position,
but keeps view orientation in world space. Units are documented on the class.
The serializer snapshot is 168 bytes
before transport encoding/envelopes. Sampling retains the prefab setting,
default maximum 30 Hz, and stops when the last subscriber leaves. Sampling runs
after the native owner camera update, using its model-space position and world
basis. Frame boundaries can reduce the achieved rate below the configured cap.

Live presentation attaches directly to the native replicated character root,
sharing its render-time parent motion with the weapon instead of using a separate
translation helper. The owner's
camera offset and basis are interpolated over the measured sample interval. During
ADS that offset is resolved against the proxy's current selected sight, so eye
and weapon use the same animation frame. Other views use world-aligned offsets
from the root. Sight changes reset interpolation; an unavailable selected sight
waits up to two seconds for the selected sight index. The packet does not identify
the weapon entity, so this cannot prove owner/proxy equipment identity during
replication lag.
`GC_SpectatorPose` applies stance, lean, ADS and lowered-weapon graph
values and the first-person layer. Other character motion continues through
native replication. It does not replay actions, teleport the target, change
possession, or write character input. Leaving restores native pose values and
the third-person layer, as well as the existing sight/zoom/camera cleanup.

This is a data foundation, not a recording or replay system. A later playback
slice still needs recording, clock/identity and character lifetime mapping,
equipment changes, discrete events, storage/version migration and a renderer
for recorded world transforms. The sample is not a full skeletal animation dump.

## MCP fixture

Load `McpTestAddon` and invoke `GC_LobbyManager.GC_TestNetworkSpectator(action,
value)` through the generic runtime interface. For a connected remote owner,
use `GC_TestPeerCommand(playerId, uniqueToken, "network-spectator",
"action|value")`, then read `GC_TestPeerReport(uniqueToken)`. A dispatched command
is not a passing assertion; inspect subsequent snapshots, logs and screenshots.

Commands:

- `snapshot|0`: local character and spectator sample state.
- `target-pose|playerId`: native character and head-bone state after detaching.
- `observe|playerId`, `free|0`: normal camera lifecycle.
- `request|playerId`: production subscription request, for role rejection checks.
- `unsubscribe|0`: stop the production stream without leaving the camera, for timeout.
- `stance|1`, `stance|2`, `stance|3`: erected, crouched, prone native requests.
- `lean|-1`, `lean|0`, `lean|1`; `ads|0`, `ads|1`; `raise|0`, `raise|1`.
- `move|1`: forward input for at most three seconds; `move|0` stops it.
- `pitch|degrees`, `turn|degreesPerSecond`: bounded look controls.
- `dynamic|0.5`: native adjusted-stance request.
- `zeroing|0..3`, `sight|index`, `weapon|index`: native owning-player selection.
- `optic|0`: scope update counts, FOV, scales and zeroing.
- `performance|0`: current rolling engine timing averages and publisher sequence.
- `measure|0`, `measurement|offset`: 120 bounded camera/profile observations,
  retrieved in 1500-character pages after completion. Profile values are rolling
  engine averages, not individual frame timings or percentile measurements.

Held actions expire after 20 seconds and affect only the opted-in owning player
in active Lobby2. Movement has its own shorter limit. The fixture uses the normal
action manager before `super.OnPrepareControls`, following installed
`SCR_CharacterControllerComponent_Test.c`. Direct `InputContext.SetLean` was
discarded: it changed the local owner pose but bypassed normal multiplayer input
replication and produced a false host-observer discrepancy. `SetWantedLeaning`
alone also did not drive the player action in this callback.

`GC_TestSpectatorSampleContract()` exercises valid/boundary data and rejects an
unknown version, excess lean, invalid stance, degenerate basis, invalid FOV and
out-of-bounds position. Live owner/server/peer transmission exercises the custom
serializer; source field counts alone are not transport validation.

## Validation evidence

Workbench and diagnostic peer version 1.8.0.13, Arland Lobby, 2026-10-27.
Installed PAK revision `af6068cb44aa4c0db613213faa82a050`.

- Valid/boundary contract passed: receipt `r80b52fe96a334045b973ecbe4147a8a2`.
- Owner-to-peer stream: standing right ADS, crouched left ADS, prone ADS-off
  samples and screenshots; receipts `rdf3b1071d50142ba86939efac1d1796c`,
  `r5ce8ab439b25428b8321c4d3f70a8c60`, `r2ed04dbe3f704ff39da2a9277915ff8d`.
- Unsubscribe stopped publisher (`subscribers=0`) and returned the peer to free
  mode after expiry: `r1b8c23203bb941e6a16059fa0fc0fc36`,
  `rd387aba5294c4a559084b3f24dbaeac8`; peer log 11:53:40.294.
- Playing requester could not subscribe to another playing player:
  `rb7371fedf176400581e3f67a84be99cd` reported target publishing=0.
- Corrected player-action fixture: host observing remote owner left ADS had
  inputLean=-1 and matching head orientation on both machines:
  `r2c6a6020bd8643af8c69ad1ecae19c4c`, `r5ec2bc9cfbc64e8d855f1fea4a05a411`.
- Forward movement reached movement=1 and updated sample world positions:
  `ra9030fdc11f84caea940de61c1afd876`, `rfc674734f42e488eb1435005af38623f`.
  That earlier unbounded movement fixture crossed the mission boundary and
  killed the target. Its replacement spectator body had no active stream:
  `re16b881a49d64103b97c34ef4431ef72`. Movement is now limited to three seconds.
- Normal ADS aim actions produced 20-degree pitch (`0.349066` radians) and
  changing heading in both owner and received state:
  `r7ec8f4fbe7b145069ae9bac4e4e699ff`, `rfa0a432b21ee4a24b08320184e0e2777`.
  An independent reviewer captured host and owner at 16:12:16 and 16:12:21 UTC:
  centered iron sights and corresponding hand/weapon pose. Native owner depth
  of field was absent from the spectator; this is not exact post-processing parity.
- Adjusted stance 0.5 reached owner and host with matching camera height:
  `r9415d6c884db4c54b3201e4427a65d5a`, `r1c59ca19905648a89a931f7beea9a7df`.
- Detaching during right-lean ADS restored free camera. The owner then changed
  to left lean and ADS-off; the host's normal character state followed, and
  publishing stopped: `r49af6fab06e24ca396ebaa4b805f9551`,
  `r5691465b154145ecbfb6c1514a05f4c2`. Reselection succeeded
  (`r578f9fa63a9b4ee3be4ddaf9742e0992`) and delivered new samples
  (`r18aae933ca4d4599a1eff1447f4a2900`).
- Production admin reassignment changed the observed player's character.
  The camera left the old target (`r606b28d0647e494caa420b9d4c0e0bcb`);
  selecting the replacement succeeded (`raa05ba92190e440794cbf42a6e4998a3`).

Initial selection before remote character initialization returned false and
logged presentation unavailable; selection after initialization succeeded. A
successful selection does not promise support for every custom animation graph.
Dedicated-server, adverse-network, vehicle, arbitrary modded-weapon, and
addon-free unrelated-mode comparisons are not established by this fixture run.

## Scope and camera correction, 2026-10-27

The prior spectator had competing stock and network scope updates. A live probe
counted 416 stock / 417 network writes; the last stock write replaced an owner
PiP FOV near 8.4 degrees with roughly 25 degrees. Only the observed optic's
`UpdateCamera` and `UpdateScopeMaterial` now defer to the network presentation.
The inherited ADS callback still updates zeroing, the distant observer, settings
and HDR. The scope camera uses its stock mount. Stock-computed reticle and edge
scales are transmitted because recalculating with a manual camera uses a
different focus-FOV reference.

Owner/spectator readbacks matched: PiP FOV 8.42927, reticle 0.948292, edge 1;
receipts `rd8db57f8c7e34a659eb80d2b975cd5ff` and
`r59fd3db0580b4ea0b4d6c00f8decb266`. Zeroing advanced on both from -0.10 to
-0.24 degrees (`ra7e4a7f330e94a01a9e6eda5ecc74adb`,
`r1c047ca9e9ab4529a3712ba2994f45b1`). Parallax/vignette and owner post-processing
are not pixel-identical; spectator parallax remains centered.

The user confirmed the scope was substantially improved but reported remaining
eye/camera movement. `camera-relative-player*.csv` recorded the camera-to-sight
position before sight-relative composition. `aligned-player*.csv` records it
afterward, 120 observations per client:

| Peak-to-peak camera-to-sight position | X mm | Y mm | Z mm |
| --- | ---: | ---: | ---: |
| Owner before | 0.240 | 0.046 | 0.043 |
| Spectator before | 1.589 | 0.833 | 1.990 |
| Owner after | 0.244 | 0.027 | 0.00004 |
| Spectator after | 0.244 | 0.027 | 0.488 |

These are short stationary ADS observations, not a guarantee of zero jitter in
all network conditions. Rendered-camera position application error was zero in
these observations. Crouched left-lean ADS remained in network mode with matching
camera positions (`r4f50f4b303b1413295cdb8883097e7a2`,
`red5546f3554a4bae95aae09b429b1337`); ADS-off retained mode 5 and removed PiP
(`r705f3e505cf54bb68fb4d1d1f2429b01`). Version-2 validity contract passed
`r496697d8d9a943d4b15fd111804252c4`.

After leaving Lobby2, the loaded-addon `Isolation.ent` stock game mode and player
controller were verified (`rd597d1e186274d60ab3fd2eaf949244b`). Optional
`GC_IsolationProbeComponent.ScopeInput` spawns a stock Senior Rifleman, holds ADS
for at most 20 seconds, observes the scope and requests native zeroing. Stock
PiP was enabled, FOV 8.4096, reticle 0.94608, edge 1, with zeroPitch -0.10 then
-0.24 (`r81ca111a221b47dca587bcfb9b9a64d5`,
`r696afb99d35044e1a15e2df03b6b9b9d`). The project remained loaded. A fresh
addon-free PiP comparison was not run; the current MCP project-open operation
refuses an already running Workbench and provides no same-process addon switch.

Live weapon selection switched from rifle to grenade and back while retaining
network mode (`r7d0cd460758048fb937b51f67e80d9de`,
`r73adeb67e56e4814837bbbe591aaf8f5`). Selecting the rifle's iron sight removed
PiP, used a 51-degree view and retained network mode; MCP screenshot confirmed
the view through the irons below the mounted optic
(`r4033f822e73d491bac92b62f42cb7c57`). Magnified sight was then restored.
Unsubscribe stopped owner publication and the spectator expired to free mode
(`r15387f96f67947ca88ea574d52a9ab86`, `r3efcdfcf3fe94c749df2d4cd1036ca26`);
the Senior Rifleman was reselected for handoff.

### Performance scope

Payload rises by 8 bytes, or 240 bytes/second at 30 Hz per active feed before
transport envelopes. The ADS reference-frame change adds no bytes. Sampling
frequency, server routing and subscription lifecycle are unchanged; owner work
adds two scalar reads and bounded matrix operations per published sample.
Spectator rendering removes the competing stock lens calculation and adds
bounded matrix composition per frame. No extra camera/render target is created.

The paired `off-ads` / `on-ads` runs used the same owner aiming through PiP with
the spectator detached/attached. Server rolling simulation averages were
1.72 / 1.61 ms; owner 2.42 / 1.66 ms. Owner FPS was 95.4 / 87.1, while enabling
the spectator also enabled its PiP render on the same GPU. These noisy,
co-resident short runs do not isolate incremental script cost or prove negligible
impact on another machine. Final aligned observations were owner 74.6 FPS and
spectator 80.3 FPS with simulation averages 2.31 / 2.21 ms. No dedicated-server,
many-spectator load or adverse-network benchmark was performed.

Evidence is in `tests/evidence/spectator-20261027`. Workbench hung during one
world reopen after successful compilation; the user authorized recovery. The
same project was reopened and subsequent live checks ran in the new process.

# Network spectator

The NETWORK camera streams one versioned `GC_SpectatorSample` from the observed
player's owner through the server to subscribed spectator owners. The server
checks the spectator role, publisher's playing state, and current character
replication identity. The receiver also checks identity, sequence, source time,
and bounded sample values. A two-second silence returns to free camera.

The sample contains world position, orientation and velocity; stance and dynamic
stance; movement and animation command; signed lean; aim/look angles; raised,
ADS, reload, sprint and freelook flags; camera offset/basis; and sight/zoom/FOV
state. Units are documented on the class. The serializer snapshot is 160 bytes
before transport encoding/envelopes. Sampling retains the prefab setting,
default 30 Hz, and stops when the last subscriber leaves.

Live presentation follows the native replicated character root, with the owner's
world-aligned camera offset and basis interpolated over the measured sample
interval. `GC_SpectatorPose` applies stance, lean, ADS and lowered-weapon graph
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

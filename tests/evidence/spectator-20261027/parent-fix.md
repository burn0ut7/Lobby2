# Native-parent spectator camera fix

Validated 2026-10-27, Reforger 1.8.0.13, Arland Lobby, Senior Rifleman USSR
AK74N/1P29. Host plus two diagnostic clients on one computer. PAK revision
`af6068cb44aa4c0db613213faa82a050`. User confirmed the native-parent version
"works perfect" after reporting that the earlier versions still jittered while
standing still. Independent review approved the final change and recovery.

## Cause and change

The translation-only attachment helper did not share the observed character's
native render-time parent motion. Camera and first-person weapon could move
apart by several screen pixels. NETWORK now parents the camera directly to the
character with AUTO_TRANSFORM and a root pivot (-1), computes its position in
model space, and converts the owner's world view basis into that parent frame.
The eye and weapon share parent motion without inheriting proxy weapon rotation.

The publisher samples the completed native OnAfterCameraUpdate model/world
transforms instead of recovering the eye offset from a later world-space read.
Sample v3 keeps ADS position sight-local and orientation world-aligned. Packet
size remains 168 bytes; server routing is unchanged. Sampling is on demand and
capped at the prefab's configured rate (30 Hz default); frame timing can lower
the achieved rate. There is no additional PiP render pass or smoothing layer.

Parent deletion also deletes the camera child. Its NETWORK destructor releases
presentation state and schedules the controller's existing reconciliation path.
The replacement free camera spawns at the cached last view before its native
movement components initialize. Intentional destruction first enters FREE and
cancels recovery; the queued callback rechecks local controller and current
presentation ownership.

## Rendered evidence

`sequence-motion.json` records the failed helper version; `parent-motion.json`
records direct parenting. Each was derived from 16 MCP PNG captures per client,
normally 16.67 ms apart (some 20.83 ms intervals). Metadata JSON preserves native
frame timestamps. The owner and spectator bursts are sequential, not synchronized.
Translation estimates compare consecutive grayscale patches with Gaussian sigma
0.8 and bounded subpixel least-squares registration. Coordinates are Y/X pixels
at 1280-pixel image width. Motion is not a general-purpose perceptual score.

| Scope-body horizontal displacement | Helper | Native parent |
| --- | ---: | ---: |
| Spectator maximum per captured frame | 2.974 px | 0.099 px |
| Owner maximum per captured frame | 0.064 px | 0.118 px |

Scope-body ROI: x35,y130,w160,h170. Scene: x300,y420,w280,h135.
Reticle: x605,y100,w65,h270. Outside scene: x1220,y400,w55,h280.
The spectator's scope body, reticle and magnified scene jumped together while
the outside scene remained steady. Native parenting removed that difference.
The native-camera source callback and later scope-callback experiment alone
did not fix it; their failed results are retained locally. The temporary later
callback experiment was removed from production and tests.

The committed animated WebP previews are 640x371 derivatives of those MCP PNGs,
encoded losslessly after resizing, with durations rounded from native timestamps.
Their short loops visibly jump at the loop boundary; this is not an in-game frame.
Original full-size PNG sequences remain local in this evidence directory.

## Checks

- Final script reload succeeded (2898 ms); contract PASS receipt
  `rddd75e169120420d842b167b8449fd23` rejects incompatible version and invalid data.
- Detach: NETWORK -> FREE retained view position, receipts
  `rb0b2eb2e23ea416a86b8c0a9356cd565` / `rd90ca077c1184e2e8fa14b5cfa9b4be0`.
- Turn, crouch and left lean: receiver ADS sample stance1/lean-1 and matching
  proxy animation, receipt `r29c7b5cb9f06466fa51ef7607104f32b`.
- Iron sight selection: ADS remained active, PiP disabled, FOV51, receipt
  `rbb0a176e7f3d426fbe45fb4301948a3b`; switching back restored 1P29.
- Production admin respawn deleted/replaced the observed character, receipt
  `r23f2c9787f5f4dc3b94bb748377edf68`. Camera recovered in FREE at
  <1199.19,40.5698,2734.57>, versus pre-respawn <1199.19,40.5702,2734.57>
  (different observation times), receipts `r500ba372395f46fa942a46856735b4ff`
  and `rd7aa0b88b3a442a5a351633500e8eec2`.
- Re-observe after recovery confirmed NETWORK target2, ADS1, v3, active PiP,
  receipt `re42f9ea3ead2405d9e43a86faf8a3c45`.
- Unrelated stock mode after leaving Lobby2, addon still loaded: stock ADS scope
  FOV8.41951, reticle0.947195, edge1; zeroing pitch -.1 -> -.24, receipts
  `rf4556a4a83974b04a3519897b4ed5dcd` / `r89173307d1b3483fbef8913964b82cf4`.
  No fresh addon-free baseline was run in this pass; isolation is not claimed
  solely from source guards. Firing/recoil and variable-magnification scopes were
  not independently re-tested in this pass; 1P29 is a fixed-magnification optic.

## Performance limits

`parent-performance.json` contains four rolling engine observations per peer in
each state. Watching ADS versus free camera: host simulation mean1.801/1.925 ms,
owner1.955/1.807 ms, spectator2.105/1.874 ms; spectator render3.100/2.355 ms.
All three instances share CPU/GPU and the views differ, so these are coarse
observations, not an isolated incremental benchmark of this fix. PiP still has
its normal extra rendering cost. The new attachment removes one helper entity;
source sampling and one cached spectator view read are bounded, client-local.
Unsubscribing stopped publishing and held owner sequence at2715 across all four
baseline observations. No server rate increase or new packet fields were added.

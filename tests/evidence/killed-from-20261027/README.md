# Killed From validation — 2026-10-27

Workbench/game 1.8.0.13; embedded localhost server and two diagnostic clients in
Arland_Lobby. All game/editor actions and captures used Reforger MCP. The optional
Lobby2McpTests addon supplies fixtures; production has no fixture dependency.

## Behavior

The authority retains a complete player-victim damage snapshot, including an AI
killer. The same hit owns killer gear, pose, selected weapon and muzzle. Later
bleed-out does not resample the killer. Only the dead player's owner receives the
snapshot. Local preview actors reconstruct the inventory, initialize the native
character, then freeze all captured bones and the held weapon. No AI victim is
tracked; no continuous server pose stream is introduced.

The implementation fixes three causes of the old broken pose: negative signed
bone IDs were being skipped (97 of 231 in the tested skeleton); the first native
animation update could erase spawn-time offsets; and restoring the weapon before
its parent bones moved it twice. Native animation-state reconstruction was also
tried, but did not reproduce the exact captured pose. The final path uses the
corrected local bone deltas after native initialization, with the weapon last.

## Evidence

| Check | Result / receipt |
| --- | --- |
| Stand, crouch, prone, lean | 231 model-space bones compared for each actor; position errors below 1 micrometre before transport, basis errors below 3e-6. Reads `r21b42afc485e460a90c9f1a474894536`, `rffbaa9d69b14478e8da0625ef6317606`, `r57b8fc5c90794dab8f2b535f8d840cba`, `r3c5021f85ccc4765a3dd7ebba9d45417`. |
| Intermediate stance .5 | Both actors 231 bones; max position 6.9e-7m, weapon position error 0: `re49b5e4eac634ba1b7a7f444c0b29484`. |
| Unarmed | Both actors match; no held weapon; equipment trees match: `r67bf055a285f4cd6a0b078e33ca0be80`. |
| JSON reconstruction | Reduced payload 104152 bytes: `r07ae6590761344d49ea2e0f1038e8b83`. Recreated decoded actors compared to the frozen source: both 231 bones, max position/basis error about 1.5e-5; held weapon position 1.3e-6m, correct managed index 1, both equipment trees match: `r4bf82565376f47078910ac21d55c573e`. |
| Actual rifle projectile | Native rifle fire hit owner at `<1199.21,40.48,2739.35>`; later stock bleed-out retained the upper-body hit and ADS pose. Fire `r74382119bceb4e2183fede8fd1e7a6cc`, bleed fixture `r04d0bba1b6f54c7d9592aaa5a871d787`. Peer1 session `16-02-14`, lines 997/1027 and 1067/1073. Screenshot `projectile-death.png`. This preceded the authority hit-tracker refinement. |
| Gear changed after hit | Local AI wounded remote player (`r561558e767bf498483c96d6f5c44faf1`), helmet removed (`r6357abc238af447b945c0a6d3a14ef3b`), then stock bleed-out (`rfe0fb59fa3414ba1b09b71a6fc4132ac`). Peer2 `16-30-41:965` retained the original 4518-byte helmet loadout rather than the later stripped gear. |
| AI absent remotely and deleted before death | Helmetless local AI wounded player (`r3e1dd620118a423689008eca10536604`), source deleted (`r0c2ec27c7e864d51931110ad6cc1b997`), then stock bleed-out (`r6c84af0cc4dc4304adac2bfae748f0e1`). Owner `raffac43f3de448bbb91b4a8e6666b061` received both 231-bone poses, correct muzzle/hit and 4176-byte killer gear. Final transmission 104851 bytes / 26 parts. See `authority-logs.json`, `owner-logs.json`, `ai-deleted-before-death.png`. |
| Remote changed gear | `r9fdea89a663a49ce841864f1696a2c24`: equipment tree equality true, no helmet in preview hierarchy, 4176 bytes. Literal JSON ordering differs across machines; comparison sorts recursive storage-ID/slot/prefab paths. |
| Owner-only transport | Other player reported no snapshot (`r166ad2e3a2494cae90fb4d6aa6386411`); original owner received all 41 parts before payload reduction. Latest unaddressed peer has no pose-freeze events. |
| Disable/new life | Disable yields no preview (`r6fde587d24aa4069b00a1e0b01470a9a`); normal admin assignment gives living player and `snapshot=none`, partial assembly invalidated (`r00cafb8ac7a1496bae4cdd5ea94249fd`, `r0cdb1cb589074775983b7517f279f3c8`). |

`lean-comparison.png` shows the exact-pose comparison. `prone-comparison.png` is an
earlier experiment: left corrected bones, center original, right approximate
native graph reconstruction. That approximate path was removed. `remote-gear.png`
shows a synthetic offset comparison, so its line endpoints are not a fatal-hit test.

## Cost and boundaries

The measured authority capture was 6–7ms for two infantry actors on this machine
while two clients also ran. It occurs on direct player damage, not every frame;
healing and retained passive ticks return without recapture or per-tick logging.
One latest external snapshot is retained per player. Removing redundant
model-space matrices reduced the sampled death transfer from about 167KB/41 parts
to 105KB/26 parts. Actors apply their bones once; only existing outlines/markers
and the line refresh continuously. Nothing is published to the living attacker.

One owner profile sample was 9.81ms/frame with preview and 11.13ms without. These
noisy samples are not an isolated benchmark and establish no FPS improvement or
large-server budget. Dedicated server load, many simultaneous deaths, high latency,
vehicle poses and modded skeletons remain unmeasured. Existing BlockListApi leak,
Steam token errors and a mission-node configuration warning also occurred; a
successful script reload is not a clean-engine-log claim.

A same-prefab duplicate rifle fixture was rejected by stock inventory insertion,
so that case has source validation of index+prefab selection, not passing live
duplicate-weapon coverage. Normal managed index 1 and unarmed selection were
tested. The semantic equipment assertion checks recursive prefab/slot/attachment
identity, not arbitrary item custom data. Preview initialization is checked
against the captured held weapon and explicitly reports a failed actor if it
cannot reproduce that selection.

## Reproduction helpers

`GC_LobbyManager.GC_TestKilledFromPose(action, value)` supports `arrange` and
`arrange-lat` with 0 standing, 1 crouch, 2 prone, 3 lean, 4 intermediate stance;
`gear` removes the source helmet; `unarmed` uses native unequip; `capture` freezes
the source and schedules both previews. Wait a native frame, then `read` compares
against the source's evaluated matrices. `roundtrip` recreates decoded previews;
read afterward. `wound`/`damage` use native damage against the player ID in value;
`publish` exercises production transport, and `cleanup` removes local fixtures.
Remote `GC_TestPeerCommand` action `killed-from` accepts `on`, `off`, `read`, `gear`;
read the same command token with `GC_TestPeerReport`. No dispatch alone is a pass.

## Final review and recheck

Independent reviewer `killed_from_review` approved the documented infantry scope
after two correction rounds. The final implementation removes all client capture
and cross-machine event merging: both actors come from one complete authority
snapshot. Removing the victim-only terminal tracker object also prevents it from
replacing a complete deletion-time snapshot. No production behavior edits followed
approval.

Final Reload succeeded at 16:41:22 local in 3305ms. Native immediate fatal damage
`rc18788630b4845978fbcbdfcad974319` recorded the actual 1.5m-high hit, both 231-bone
poses and helmetless gear before the destruction callback, then sent 104552 bytes
in 26 parts. Authority capture measured 7ms; both peers have zero tracker-start
events in this cycle. Final unarmed recheck `ra1516a3d974d4fa9ac33794d97b1c743`
and prone recheck `r290f9f6cfb6c41eeb2286ff7f44fb3d3` pass with all 231 bones,
matching equipment and selected/unarmed weapons. No weapon-readiness errors occurred.

Normal reassignment cleared the owner's snapshot (`rc04aba6b61964bb28373c8821bb33ad6`);
the unrelated peer still had none (`r6ebf3b9d01db404a84be4ce3e9ded96a`). A second
native fatal hit (`rdd1cd644ce8245b3ab7e2c1a8a5be491`) produced sequence 2 with
26/26 parts (`rbad456d7315443e8960e4c628f1fed14`). See `final-authority-logs.json`,
`final-owner-logs.json`, and `final-preview.png`. Both clients and Workbench remain
open; player 2 has the killed-from preview enabled.

Saved scripts were checked for tabs, Allman blocks, ownership/guards and accidental
diff changes; `git diff --check` passed. Existing compact getters and public engine,
RPC, serialized-field and reflection names were preserved. The live gear mutation
coverage is removal of authored headgear; arbitrary newly added/modded equipment
has not been exhaustively exercised.

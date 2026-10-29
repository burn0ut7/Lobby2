# Killed-from rotation and last-source fixes

2026-10-29; Workbench 1.8.0.13 PID 17588; existing Arland_Lobby and optional MCP test addon. All engine interactions/captures used Reforger MCP.

## Changes

Capture the turret movable base separately from the gun aim, restore it before freezing the seated actor, and hold both during presentation. The M1025 test now exercises a -90 degree base with independent 0.15/0.12 rad gun aim. Its prior test only exercised the limited gun arc.

One latest damage snapshot owns attribution. Bleeding/healing preserve it, including native bleed-out's terminal TRUE notification when the blood zone is destroyed. Respawn preserves that scene and blocks ForceDeath notifications from replacing it. With no damage it captures self-death. New direct damage, including unknown source, replaces the previous scene; empty same-frame same-source same-type notifications are treated as duplicates. Direct self-damage shows YOU KILLED YOURSELF, without a killer or shot line. Statistics policy is unchanged.

## Validation

- Final saved script Reload `3bb5ba110c62473bb721836d77423076`: compilation completed (cycle begins console line 36777 at 18:54:04.486; completion line 36915). No script compile errors; existing wrong-world-GUID and SCR_DedicatedServerPlugin diagnostics remain.
- Production handler fixture `r30c2190cdbed481ea9dc23215757098e`: paired duplicate, bleeding, healing, Respawn retention, post-Respawn terminal rejection, direct self, newer external source, unknown source and no-damage Respawn all true. Earlier duplicate failures came from the fixture leaving native default direction/velocity populated; explicit zero vectors fixed the fixture, not the expected result.
- JSON roundtrip and rendered clone `r8195c311ded544c8bde08f1d9868dcb9`: source/captured/cloned base -90 degrees, vehicle matrix error 0, gun aim error 0, muzzle error 3.8147e-6 m, gunner root matrix error 2.98041e-8, occupied seat true. `turret.png` captures owner publication after moving source entities away.
- Direct self with zero vectors `re6a31dc2992f4a159ee518523d7d57ba`: self=1, no killer, no shot line, 231 victim bones. Separate normal self publication is visible in `self.png`.
- Native wound then actual owner Pause-menu Respawn entry: wound `r543720c725fa46aeade9d5c947944b29`, request `r69473f36faaf4052998260d4ef529053`; owner `rb763eb201a70407f836486018197c360` received 26/26 parts, external AI pose and original hit (1198.98,40.5,2734.37), self=0.
- Native wound then stock bleed-out: wound `re20a90861fc94eeea469a6716ca0878c`, low-blood/stock bleeding `rc34f0aa8711e4f0fae4b67e6107ba10e`; owner `r0d2616b5b0874586907fe13fd4c58bc8` received sequence 4, 26/26 parts with original hit (1201.98,40.5,2731.37) and external AI pose. Terminal notification did not replace the scene.
- Separate connected player 2: native wound `rb1d0a95159fd4f779b936fdc311fdb8e`, owner Respawn `rda15d6ad89e64d9f984e649e25c73b62`, remote read `r66628294348d4ff6a18870eebf6a1e00`: 26/26 parts, original external AI and hit, self=0. The local AI source is absent on the remote machine.
- New playable assignment followed by no-damage native Respawn: `r724548826a894561927689ff54bda4ca`; read `rdb5cce71af1c4589a6ccd8e4d826b3cb`: sequence 5, 13/13 parts, self=1, no killer/line. `native-respawn.png` shows one labelled reconstruction beside the real ragdoll, not a second attacker.

## Scope and reproduction

`GC_LobbyManager.GC_TestVehicleDeath`: arrange, capture, checks, read, publish, view, move-sources, self, self-empty, respawn, respawn-empty, bleed, cleanup. Respawn/respawn-empty terminate the fixture tracker; arrange starts a new life. The vehicle fixture uses explicit damage facts through the production tracker; it is not live ballistic firing. Native attribution tests used engine melee wounds followed by native Respawn or stock bleeding. Dedicated-server, remote mounted-gunner capture, multi-turret and modded vehicle coverage are not claimed. No shared modded production override changed in this slice. Existing engine/resource warnings remain outside the fix.

Independent review `review` approved code, tests and rendered states within the documented scope. Its evidence follow-up was resolved by saving all subsequent pages at the same pinned log revision in `runtime-log-continuation.json`; the original first page is preserved. This includes the native owner and remote publication events through line 37674. Original failed fixture results are recorded above rather than represented as passes.

Optional vehicle and infantry fixtures were cleaned through their owning MCP helpers (`r54806314848b4fefaea8c3b76b211703`, `r8b4b46baf9ef40f79e01a3b81fd11d41`). Workbench, the same Arland world, test addon and connected peer remain open. No authored world or multiplayer settings were changed. `git diff --check` passed.

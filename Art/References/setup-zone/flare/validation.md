# Setup flare regression

2026-10-25, Workbench 1.8.0.13, embedded host. Base Lobby2 revision 81b128a.
User reported firing a potentially damaging flare during setup.

## Cause and change
Vanilla SCR_FlareAnimationComponent registers a CharacterFire DOWN listener.
FireFlare calls TryUseItem directly; a later Fire animation event sets
CharacterInputContext.SetWeaponPullTrigger(true). It bypasses the character's
OnPrepareControls action-value filter.

The production override gates CanContinueTheAnimation using the same affected-player
combat policy as character/turret inputs. Vanilla invokes that gate both before
starting and during animation events, and owns cancellation via CancelItemUse.
No additional polling, listeners, global state or damage suppression is introduced.
The shared override delegates unchanged when Lobby2 is absent or the player is
unrestricted. Safe Start remains configurable; admin freeze uses the same policy.

Installed source catalogue 4942ab6194e34c74bd5972e7761785e2:
game/data/data007.pak/scripts/Game/Components/Flares/SCR_FlareAnimationComponent.c,
lines 35-47, 51-122 and 169-209; source SHA256
b2c25d7533e74bba64c77bf363e992b524a2aedf669183ee1ed54b19fb666b74.
Vanilla Flare_RSP30_white.et uses SCR_WeaponComponent/SCR_MuzzleInMagComponent.
The wiki index was unavailable; installed scripts/prefab data supplied the evidence.

## Live checks
The optional fixture invokes the actual FireFlare input callback (not muzzle Fire,
trigger writes, or a substitute restriction implementation), then waits for vanilla
animation, ammo and equipped-item changes. Input hardware itself was not simulated.

- Addon-free IsolationProbe: stock mode/controller, possession confirmed;
  loaded/raised flare has ammo=1, enters usingItem=1, then magazine absent
  (ammo=-1), usingItem=0, currentFlare=0. baseline.png shows the resulting trail.
- Lobby2 Empty_Lobby, US slot 11: normal lobby/faction/slot/briefing/game journey.
  20-second setup, Safe Start=true, soft mode. Fire attempt with raised flare:
  ammo stays 1 and usingItem stays 0 after 6.5 seconds. Setup still active.
  blocked.png shows the flare and countdown.
- Natural deadline release: active=0; same flare callback then consumes magazine
  and unequips the flare normally.
- Safe Start=false with setup still active: flare fires and is consumed.
- Start firing unrestricted, observe usingItem=1, then enroll in safe setup:
  after 6.5 seconds ammo=1, usingItem=0, currentFlare=1, setup active.
  Remove setup and the retained flare fires normally.
- Switch from Lobby2 to stock Isolation.ent without unloading the addon:
  stock mode/controller; same firing sequence consumes flare exactly as baseline.

Raw receipts and receipt-correlated logs are adjacent. The initial attempt to
prepare a native character component was explicitly unsupported; no method ran.
The test was exposed through the existing ScriptComponent fixture interface.
Initial fixture spawns used a bare resource path and emitted zero-GUID diagnostics
despite successful spawns. Final fixture resolves the canonical resource through
Workbench.GetResourceName, following vanilla SCR_BuildingSetup usage.
Other existing map/loadout/resource/Steam errors are preserved in errors.json.

## Limits
Embedded-host coverage only; no remote client/dedicated/JIP test, custom mod flare
or malicious-client bypass claim. No damage test is needed to establish blocked
firing, but target damage was not measured. Existing setup expiry/enrollment remains
authoritative; this fix is an owner-specific firing guard, not new anti-cheat.

Final fixture validation: reload succeeded; canonical name resolved to
{327103CB218E7CA1}Prefabs/Weapons/Flares/Flare_RSP30_white.et. Spawn/equip/firing
passed again in stock mode with Lobby2 loaded. final-errors.json has no flare
resource warning; unrelated fixture/vanilla errors remain.

The first four isolation setup receipts precede a test-wrapper reload and lack
saved matching log lines. They are preliminary setup only, not the passing flare
baseline; the subsequent complete baseline and all flare calls have saved logs.

Independent review approved production and final fixture evidence with no blockers.
Parent Lobby2 Arland world restored with Play stopped after testing.

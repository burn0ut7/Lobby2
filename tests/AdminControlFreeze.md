# Admin control freeze — implementation checkpoint

Implemented in Admin Tools → Freeze, with separate Freeze players and Unfreeze
players actions. Selected players, Everyone, Nearby (1–1,000 m) and Faction reuse
Heal's server-side audience validation. Selected identities are checked together
before changes; every request authenticates the requesting controller on authority
and reports through the central admin logger. Everyone/faction mean the connected
audience at execution, not future joiners. Locks last for that controller's session
and follow possession; disconnect/mode shutdown clears them. Unfreezing does not
cancel separate setup or staging restrictions.

Authenticated admins are skipped in every freeze scope, including explicit
selection. The controller setter and effective control checks also enforce this
exemption. An admin login clears any earlier lock and reconciles held bodies;
logging out does not resurrect that lock. This exemption applies to Admin Tools
freeze, not independently configured setup/staging restrictions. A vehicle held
because of another frozen driver/gunner still physically carries all occupants.

The affected player's action manager clears gameplay actions; menus and
communication remain available, including recovery through Admin Tools. A native
input context also suppresses lower-priority gameplay contexts. Shared character
callbacks preserve inherited behavior outside active Lobby2.
The context is an invisible ActionManager rule, not a popup, widget or menu.
It explicitly permits MenuOpen/MenuOpenWB/MenuBack and VONDirect/VONChannel,
preserving the user's configured pause, local-talk and radio-talk bindings.
The shared OnPrepareControls filtering follows the existing safe-start path;
the context additionally blocks gameplay listeners such as inventory opening.

Authority derives body holds from native occupied seats. On foot, it holds the
living character. A pilot/driver or turret occupant holds the vehicle root; a cargo
passenger does not select that body. Multiple frozen occupants share one hold, so
releasing one cannot release a vehicle still required by another. Body/seat changes
are reconciled every 100 ms while anyone is frozen.

The existing editable body component carries one replicated hold flag. Each peer
retains its native physics state, removes dynamic simulation using COLLISION,
and clears linear/angular velocity on dynamic bodies. COLLISION preserves
collisions; NONE would also remove the body from the collision world. Release
restores the prior simulation state only if the body still has the state this
feature set. No teleport, position correction, force impulse, camera fade, or
whole-vehicle hold caused by a passenger is introduced. Missing eligible-body
physics is an explicit partial result and a centrally logged transition.

## Evidence and limits

The user explicitly deferred compilation and all live testing on 2026-10-25.
This is source implementation, not a verified gameplay result. The reported
flashing/back-and-forth movement was not reproduced or attributed; the existing
setup-zone correction remains untouched. Do not claim this resolves that report.

Native declarations/examples inspected through Reforger MCP: PAK revision
`bf051a869cde4dccb529ebc970249123`, Core `Physics` and `SimulationState`, game
`SCR_DestructionBaseHandler` and `SCR_AIGroupVehicle`; prior revision
`4942ab6194e34c74bd5972e7761785e2`, `ActionManager`, `SCR_PlayerController`,
`SCR_CharacterControllerComponent`, and their installed call sites. Declarations
establish signatures, not native character/vehicle behavior.

Saved local visual concept and design rationale:
`Art/References/admin-control-freeze/concept-v1.png` and `brief.md`.
The revised description explicitly says drivers/gunners stop their vehicle.

## Deferred MCP validation

Optional `GC_TestControlFreezeRequest(scope, targetId, frozen, radiusMeters,
factionKey, staleIdentity)` exercises production requests without granting roles.
`GC_TestControlFreezeSnapshot(playerId)` reports server/owner policy, native seat,
position, ammunition, body simulation state and dynamic velocities. Existing
`GC_TestClick` exercises the rendered Freeze/Unfreeze buttons. Flags and successful
dispatch alone are not passing evidence.

Required checks before gameplay acceptance:

- Compile production and optional test addon; inspect actual UI, focus, disabled
  pending actions, empty/long selections, all scopes and close/reopen.
- Confirm admins are skipped in every scope, including self/explicit selection;
  login while frozen releases controls/body, and logout does not restore the lock.
- While frozen, open/close pause, transmit/release local voice and radio with the
  configured keys (including T and Caps Lock where bound), and verify reception on
  another peer. Check that no freeze popup or cursor capture appears.
- Hold movement, aiming, fire, grenade, melee, interaction and inventory inputs
  before/during/after freeze; observe actual positions/ammunition/actions.
- Use a moving vehicle with separate driver, gunner and passenger peers. Verify
  driver/gunner holds and zero momentum; passenger-only freeze must preserve the
  other occupants' control and vehicle motion. Exercise seat switches and multiple
  frozen occupants, then partial/final unfreeze.
- Verify server/owner state, distant targets, joining/streaming peers, physics
  ownership changes, death/respawn, disconnect and character replacement. Check
  collision-only native character behavior and restoration after destruction.
- Reject logged-out/unknown requesters and stale selections without mutation,
  with central audit evidence. Verify radius inclusion/exclusion and faction scope.
- Check overlap with setup/staging, unrelated mode with addon loaded, mode exit,
  and addon-free baseline. Inspect for flicker, correction loops and stuck physics.

Independent source review approved the implement-only checkpoint after revisions
to context ordering, unavailable-body reporting and simulation/mode lifecycle
handling. The saved-script readability review and git diff whitespace check passed.
Live acceptance is deferred by the user's instruction, not implied by source review.

Follow-up source evidence: catalogue `4ee49efc58494a12b2fde693a84d62e9`,
`game.c` subscribes to UIConstants.MENU_ACTION_OPEN and MENU_ACTION_OPEN_WB;
`Global/Constants.c` resolves these to MenuOpen and MenuOpenWB. The freeze context
previously omitted those actions; both are now permitted. Existing VONDirect and
VONChannel bindings/listeners remain unchanged. This follow-up is also uncompiled
and untested in gameplay at the user's request.

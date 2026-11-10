# Delete Entities and Assign Waypoints

These are separate actions alongside Spawn Action. Add them directly to an event's Actions list. The event still owns conditions, delay and repeatability.

## Delete Entities Action

**Entities** lists exact entity names. Each matching entity and its attached children are removed through the engine's replicated deletion helper. Missing/already deleted names are skipped, and duplicate targets are processed once. A list containing the active game mode or one of its parents is rejected before anything is deleted.

This is named-entity removal, not a prefab search or an AI-group member search. Naming an AI group does not explicitly enumerate its soldiers; engine lifecycle behavior still applies. Deleting a vehicle can affect occupants through its normal engine cleanup. Separately owned building interiors are not specially enumerated. Use this for intentionally named mission objects, with those ownership boundaries in mind.

Example: Entities = `old_truck`, `temporary_barrier`. Repeat execution skips objects already gone. An entity later spawned with one of those same names is a new matching target on the next execution.

## Assign Waypoints Action

- **Group Name**: exact name of an existing AI group, not a vehicle or individual soldier.
- **Waypoints**: existing AI waypoint names, in the desired order.
- **Replace Existing**: unchecked appends orders; checked removes current assignments before adding these orders. Checked with an empty list clears the group's waypoint orders.

Every new waypoint resolves before any existing assignment is removed. A missing group, missing waypoint or wrong entity type rejects the action without changing orders. Empty append lists are invalid. Clearing orders does not disable AI combat reactions.

The engine may delete group-owned editable waypoints when their assignments are removed. Replacement therefore rejects an already assigned, group-owned editable waypoint in the new list before changing any orders. Use new waypoint entities for that replacement. Ordinary authored waypoints without this editable ownership can be reused. This keeps native waypoint ownership intact without global overrides or temporary reparenting.

Example: spawn an AI group with Entity Name `reinforcements`. A later event assigns `rally_point`, then `defend_base` to Group Name `reinforcements`. Check Replace Existing to replace that group's previous route.

## Implementation and verification

Both actions use the existing current-framework/active-Lobby2/server authority gate. There are no client RPCs, world scans, polling loops or delayed callbacks. Lists are limited to 128 nonblank names. Names resolve at execution, supporting entities spawned earlier in the mission. Spawn Action and Assign Waypoints share the same waypoint implementation.

Reviewed TilW `TILW_DeleteEntitiesInstruction` and `TILW_AssignWaypointsInstruction` at commit `4a5300f94fb9323ce4942ef296bcffcdda922626`. Installed engine evidence, catalogue `c52648a62fcb4229ad089610d060ea44`: `scripts/Game/Helpers/SCR_EntityHelper.c:174-179` delegates deletion to `RplComponent.DeleteRplEntity`; `scripts/Game/Commanding/Commands/SCR_StopMovementGroupCommand.c:18-23` removes existing waypoint assignments. `scripts/Game/Editor/Components/EditableEntity/SCR_EditableGroupComponent.c:214-221` can delete removed group-owned editable waypoints, and `scripts/Game/Entities/SCR_AIGroup.c:2401` explicitly documents deletion by removal listeners.

Verification is source/logic review and Workbench compilation/reload. Optional `GC_EntityActionTests.RunEntityActionValidation` contains ten validation checks; it compiles but was not executed. No in-game tests were run, as requested. Actual AI movement, replicated deletion, prefab-specific cleanup and picker presentation remain untested.

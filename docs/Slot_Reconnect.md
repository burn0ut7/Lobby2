# Slot reconnect grace period

Set **Reconnect grace period in seconds** on the game mode's `GC_LobbyManager`
component in Workbench. Default: **120**. **0** releases immediately; negative
values are treated as zero. This game-mode setting takes precedence over the
native dedicated-server reservation timeout. Changes affect new disconnects.

A disconnected player keeps their identity-based slot claim and living character.
Rejoining before the deadline restores the same slot automatically. During GAME
this restores possession of the same character; during slotting/briefing it
restores the assignment and follows that phase's normal presentation. Other
players cannot take the held slot through ordinary slot selection.

Expiry clears the player claim. During GAME the existing living character remains
available through that slot. Before GAME, staged characters are removed. Existing
admin locks and KIA rules still apply: expiry does not unlock an admin-locked slot
or revive a dead character. Cleanup runs once per second; reconnect checks the
exact deadline too. World teardown cancels reservations.

Persistent UID import/export and automatic initial pre-slotting are a future
slice. No persistent reservation format or slot-pinning behavior is added here.

## Validation - 2026-10-23

Workbench 1.8.0.13, optional Lobby2McpTests, embedded network host plus one native
Peer Tool client with distinct identities. Production scripts and test fixture
compiled. Reforger MCP drove all editor, runtime and peer actions.

- GAME reconnect at default 120: same entity, position and health, owner possession
  restored automatically; another player's normal slot request rejected while held.
- Configured 5-second expiry: claim released, same character retained, ordinary
  selection by another player succeeded; original player's later return did not
  reclaim that slot.
- Configured 0: immediate release, no timer, living character retained.
- SLOTTING reconnect: unspawned claim restored, reserved spectator reused.
- BRIEFING disconnect followed by GAME start and reconnect: same staged character
  possessed, reserved spectator count returned from 1 to 0; owner observation agrees.
- Production deadline comparison: before=false, equal=true, after=true.

Initial network test exposed duplicate native/Lobby2 reconnect components and
character deletion. The prefab now replaces the native component; a new runtime
inventory confirmed exactly one reconnect component before the passing tests.
The known ThermalProfile resource-leak assertion occurred during reload and is
not repaired or counted as a clean stability result. These tests do not establish
standalone dedicated-server, network-loss timing, seated/dead-character or
large-session coverage.

Raw requests/results, correlated logs, source reads, hashes and screenshots:
MCP repository `.local/slot-reconnect/`. Key receipt prefixes: default restore
`1792782274269878800`, expiry `1792782290242449900`, new claimant
`1792782291432670600`, zero `1792782323136180400`, slotting
`1792782472893500200`, briefing-to-game `1792782498603052800`, remote owner
`1792782504596877200`. The initial failure remains in that evidence directory.

The optional `GC_ReconnectTests.c` provides timeout arrangement, slot/entity
observations and deadline comparison. It does not simulate a network reconnect
or bypass production slot/administration checks. Existing production admin kick
requests caused the tested disconnections; native Peer Tool stop/start rejoined.

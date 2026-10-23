# Game Master playable groups

In an active Lobby2 mission, an authenticated admin can place a normal group in
Game Master, edit its properties, select **Lobby**, and set **Playable** to **Yes**.
The group’s existing soldiers become slots; their entities, equipment and placement
are retained. Individual soldiers in a faction group support the same property.
Repeated enabling does not duplicate slots. Setting **No** returns unclaimed
soldiers to AI. Occupied or reserved slots reject removal with an owner-visible hint.
Registration is session state, not an editor-save attribute.

The native editor manager identifies the requester. The server checks admin login
and writes the result through the central audit logger. Native groups remain the
authority for faction and membership. Lobby2 records only registrations it owns
so removing slots does not delete a live Game Master group or unregister another
system’s group.

Open lobby cards observe topology and presentation events. Group rows, faction
slot totals and claims refresh without reopening. The header reads connected
players from the native player manager on attachment and connection events.

## Validation, 2026-10-23

Workbench 1.8.0.13, optional Lobby2McpTests addon, AdminMultiplayer world, through
Reforger MCP. Production and optional test scripts compile. The fixture includes
one ordinary four-soldier group; the placement helper also uses native Game Master
creation, with its catalog, permissions and budget checks intact.

Observed on the embedded host:

- Native Game Master creation logged `Group_USSR_Team_GL.et` at `<145,1,140>`.
  Its native properties dialog displayed Lobby / Playable / No. Native confirmation
  registered four soldiers: seven groups / 42 slots became eight groups / 46 slots.
- The property handler rejected an unauthenticated requester without changing AI,
  entities or roster. Valid execution used the editor owner even when passed an
  invalid native RPC player argument. Central audit recorded both outcomes.
- Entity identity and placement were retained; AI was deactivated. Repeated enable
  kept the same slot count. Disable retained bodies and reactivated AI.
- With the lobby open, faction totals changed 10 → 14 → 10 → 14. Slot claiming
  changed occupancy to 1 / 14; disabling an occupied group was rejected.
- The updated header displayed one connected player instead of its old static 128.

Raw MCP receipts, logs and screenshots are under
`reforger-script-tools/.temp/evidence/gm-playable/live/`, including
`playable-checkbox-final.png` and `lobby-enabled.png`.

The editor encountered its previously recorded resource-teardown assertion and
`BlockListApi` leak during reload. Acknowledgment is not a fix. The fixture also
emits pre-existing map/resource warnings. Peer validation initially failed because
the nested test addon was absent from its search path, then because Reforger
rejected duplicate native identities. These failures do not establish multiplayer
correctness by themselves. A clean Workbench restart restored the host's nonzero
native identity. The subsequent host-plus-peer run passed:

- The late-joining non-admin client received all 46 slots.
- Without reconnecting, its roster changed 46 → 42 → 46 on disable/re-enable.
- Its production slot request claimed new slot 47. Server state and the actual
  client screenshot agreed: Soviet Army 1 / 14, new group 1 / 4, named occupant.
- Disabling that occupied group was rejected. After release and disconnection,
  native connection/controller/slot/voice records were absent and the header
  changed 2 → 1. `peer-claimed.png` records the connected client presentation.

After leaving Lobby2 in the same process, the stock Isolation fixture reported no
Lobby2 manager or mode, rejected a Lobby2 phase transition, spawned four native
group members, and allowed native possession. A fresh process with only core,
ArmaReforger and IsolationProbe loaded produced the same group/possession results.
Both runs also matched the existing script voice callback probe (one receive and
capture callback, two selectable entries after possession). This is callback
coverage, not a microphone/network audio test. Dedicated-server coverage remains
separate from the tested embedded host plus standalone client.

Peer processes were stopped, original peer parameters restored, and temporary
localhost Play mode cleared by closing World Editor.

## Leader ordering regression (2026-10-23)

The reported session's audit showed a mixed selection applying Playable to the
automatic rifleman before its group. Creation order therefore put that soldier
above the native leader. The native properties dialog reproduced this with
`GC_TestOpenPlayableProperties("GC_GMPlacedGroup", true)` and confirmation.

Adopting a non-authored group's native leader now inserts its slot at index zero,
shifting existing slot indices without replacing their identities or claims.
The roster presents the transmitted indices in order, independently of creation
or network arrival. Authored playable-group indices remain unchanged.

`GC_TestEditorPlayable("GC_GMPlayableFixture", true)` exercises the individual
soldier then group property writers. It failed against the previous production
code and passed with the fix, checking member order, existing slot identity,
entity/position preservation, AI deactivation, authorization and repeated enable.
Pass `false` for the original group-only case. `GC_TestPlacePlayableGroup` now
accepts the prefab argument; the six-member reproduction uses
`{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et`.

Workbench compilation passed. An embedded localhost host and standalone non-admin
client verified late-join ordering, live registration/count updates, and a normal
client claim of the leader slot. The mixed rifle-squad selection retained the
earlier automatic-rifleman slot 47 at index 1 and placed leader slot 48 at index 0.
`leader-host-fixed.png` shows all six rows after claim/disconnection;
`leader-peer-jip.png` shows the four-member late-join case. The live peer claim
screenshot verifies counters/ownership but its squad rows are below the viewport.
Evidence remains in the directory above. Dedicated-server testing was not repeated.
An additional peer relaunch exited with code 0 before its six-member screenshot;
that attempted capture is not counted as passing evidence.
Peer settings were restored and test clients stopped. Production-only compilation
passed (`345c1fb1203449c4aee7c25739434388`); Arland was reopened without the test addon.

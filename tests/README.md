# Lobby2 MCP tests

This optional project owns Lobby2-specific test controls and checks. The MCP
server remains addon-agnostic. Production Lobby2 does not depend on this addon,
and loading it alone runs no tests. Do not publish the test addon.

## Load and unload

1. Save authoring work and stop Play. Close Workbench normally through MCP.
2. Make `tests/IsolationProbe` discoverable in Workbench's addon search directory.
   For this checkout, `addons/Lobby2IsolationProbe` is a directory junction to
   that repository folder; it is one source tree, not a copied addon. Alternatively
   configure the test directory explicitly in the server's addon search paths.
   Merely opening the nested probe once does not register it for dependency discovery.
   Use `workbench_open_project` with the absolute path to
   `tests/McpTestAddon/addon.gproj`. Workbench must already know the parent Lobby2
   project (opening its `addon.gproj` once registers it locally).
3. Obtain the PID from `workbench_script_status`. Confirm `Lobby2`,
   `Lobby2McpTests` and `Lobby2IsolationProbe` are loaded with `workbench_project_context`.
4. Require `workbench_runtime_setup(action="status")` to report ready. If needed,
   install the generic development adapter through its documented setup flow.
5. Open `$Lobby2:worlds/examples/MPTest/Empty_Lobby.ent` using
   `workbench_open_world`, then start `workbench_play`. Answer Workbench's script
   authorization using the user's preference. Poll pending operations; never
   replay an uncertain mutation.
6. To unload, stop Play, close Workbench, and reopen the **parent** `addon.gproj`.
   Verify `Lobby2McpTests` is absent from loaded projects. No source copying,
   production component attachment, settings switch, or alternate server is used.

Most helpers require `WORKBENCH` and run only in embedded Play.
`GC_AdminMultiplayerTests.c` also compiles in test-addon peers. It grants no roles
or permissions and runs nothing automatically. Never include it in production.

## Slot lotteries

See [SlotLottery.md](SlotLottery.md) for concurrent draws, native `/join`,
context actions and the test entry points. Runtime evidence and limitations are
recorded in [validation](../Art/References/slot-lottery/validation.md).

## Faction readiness

`GC_TestReadinessSnapshot()` reports phase, revision, command slots, commanders,
ready state and local request status. `GC_TestReadinessRequest(key, slot, revision,
ready)` exercises the owning production controller, including stale/unauthorized
requests. `GC_TestReadinessDisconnect()` is explicitly a simulated handler check;
use a connected peer and the existing kick fixture for actual network departure.

The existing correlated peer transport accepts `readiness-snapshot`,
`readiness-join` (slot ID), `readiness-open`, `readiness-ready` and
`readiness-unready` (faction key). Read a fresh snapshot after asynchronous
requests; the immediate report is not acceptance. `readiness-focus` sets native
focus to the named faction action; `readiness-long-name` and
`GC_TestReadinessLongName()` change only visible label text for layout inspection.
These fixtures do not establish physical mouse/gamepad navigation coverage.

See `Art/References/faction-readiness/validation.md` for observed results.

## Connected admin test fixture

See [AdminReplicationScope.md](AdminReplicationScope.md) for the per-action
replication audit, separate-client absence checks, regression helpers and
validation limits. Server-wide scopes must be exercised from a separate client
with the target's character and slot replication item confirmed absent.

`GC_DedicatedAdminSuite` is an explicitly selected native autotest entry point
(`-autotest GC_DedicatedAdminSuite` on the dedicated server). Launch through the
MCP Dedicated Server Tool with the test addon and the Everon Lobby2 world loaded.
It requires a fresh disposable PREGAME session and exactly two clients. Backend
identities are not required for the global/faction coverage. It temporarily grants
the first client's native session-admin role with explicit user authorization,
and revokes its owned role in teardown. Do not run it on a shared server. It changes
phase, slots, health, positions and freeze state through test fixtures and real
production requests. The target moves over 9 km away on Everon.

The sequence checks owner-client replication absence, rendered Everyone/Faction
heal handlers, nearby exclusion, messages, moderation roster, KIA recovery, freeze
and resumed firing, reservation export, map-plan save/phase rejection, admin chat,
menu tabs and logged-out rejection. Missing identities are tested as rejection
cases for selected actions/teleport/moderation; they do not establish successful
identity-dependent behavior. The `dedicated-action` transport captures absence
before each request and correlates its production result. Native ban-list retrieval
crashed this unconfigured dedicated session after submission; the automatic suite
records that limitation without replaying the crash. The helper retains the
explicit `ban-list` action for a suitably configured follow-up.

Require the final native test result, peer observations and central audits together;
a dispatch or the early `autotest_succeded.log` file is not acceptance. This does
not enable standalone `workbench_runtime_invoke` support. See the dated dedicated
runs in [AdminReplicationScope.md](AdminReplicationScope.md), including the initial
identity-gated failure and the later expanded coverage.

### Game Master recovery — 2026-10-24

`GC_HealSlotContextAction` adds **Heal / Respawn slot** to the native Game Master
selection menu. It uses the existing authenticated slot request; it does not
replace vanilla Heal or register arbitrary GM soldiers as slots. The action is
hidden without an active Lobby2 slot or authenticated admin and cannot execute
in POSTGAME. Its config is additive to the native `TempEdit.conf` action list.
Reference: the official community wiki's *Game Master: Context Action Creation*,
captured revision 370954, and installed `SCR_SelectedEntitiesContextAction`,
`SCR_HealEntitiesContextAction`, `SCR_BaseContextMenuEditorUIComponent` sources
(PAK catalogue `5c4befda38ee44a29a2211ac42bdbfc7`).

`GC_TestGameMasterRecovery(slotId, operation)` opens the actual native popup via
its owning handler using native hover/selection filters. `click` invokes its
rendered recovery button's normal handler. Embedded Workbench tests verified one
recovery button, healing from 0.6 to 1.0 for claimed and unclaimed slots, and KIA
recovery to a new healthy character with central audit evidence. The respawn
occurred before the scripted click check, which correctly returned unavailable;
it is observed UI/state/audit evidence, not a correlated scripted-click pass.
The subsequent controlled `open-click` run rendered and activated the native
button in one invocation (`rda359c55a3d9436397aaf803f74507a2`). Fresh state changed
slot 1 from KIA entity -2147481115 to healthy entity -2147480864, preserving its
unclaimed status, with one applied central audit at 09:18:09.108. This separately
establishes scripted respawn activation; it does not reclassify the earlier run.
`GC_TestGameMasterRecoveryLoggedOut` revokes the connected host's roles and checks
hidden UI plus server rejection without mutation; it never restores/grants roles.
End the disposable Play session afterward. This returned connected=1, admin=0,
visible=0, rejected-not-logged-in, unchanged=1, with the central rejection log.

`GC_TestGameMasterEntityComparison(slotId, groupName)` inventories live components,
native editability, damage, AI and action predicates. A native GM-placed USSR
senior rifleman and a lobby-spawned rifleman used the same prefab and identical
55-component multisets, including editable-character and damage components.
Both had editable flags 71. The lobby character had slot 1 and inactive AI;
the GM character had no slot and active AI. After the existing Playable property
adopted the GM group, that same character gained slot 43, retained its components
and editable flags, and became eligible for recovery. Both healthy characters
failed vanilla `CanBeHealed`, which hides stock Heal in the native menu.

Evidence is in MCP workspace `.temp/local/evidence/game-master-recovery-20261024`;
the visible-popup capture is `1792847233238266000.png`. Initial test-helper compile
errors were repaired and native reload passed. Retained resource/engine warnings
are not a clean-log claim. Coverage is embedded host, one native GM group and
slot fixtures; remote peers, gamepad and multi-selection execution remain unverified.

The addon-independent `GC_IsolationProbeComponent.ContextMenu` fixture compared
the native popup on `IsolationCharacter` in `Isolation.ent`: first with Lobby2
loaded after leaving its active world (PID 40968), then without Lobby2 loaded
(PID 33436, project context verified). Both rendered the same action multiset:
`SCR_TeleportCameraContextAction`, `SCR_PlaceEntityContextAction` and
`SCR_LightningContextAction`, with no recovery entry. Receipts
`r3bb1f0e23320445895fe6c7517d54b6d` and `r458b2540cdad4104a2937128975989c7`
record this narrow action-list isolation comparison. Stock Heal was unavailable
for this preplaced target in both runs despite health 0.6; both reported that
explicitly and left health unchanged. This checks matching popup availability,
not stock-Heal execution in unrelated modes. The Lobby2 popup capture separately
shows stock Heal and the new recovery action coexisting.

`GC_TestAdminEntityContext(int slotId, string operation)` exercises the production
spectator entity context. Use `inspect` to list eligible actions, `open` to execute
Open Admin Tools and read its actual heal-picker selection, `heal` to dispatch the
existing heal/respawn action, and `stale` to invalidate only the captured claim and
check action rejection. It grants no permissions. Check living and KIA slots,
connected claimants whose bodies are no longer controlled, unclaimed AI, changing
claims, logged-out admins, and POSTGAME. For a connected claimant, `open` should
report exactly that player selected. Verify heal/respawn effects with fresh health,
slot and possession state plus central audit logs; the immediate report is not a
success receipt. Capture the real menu and test physical right-click navigation.

The 2026-10-24 changes compiled in a fresh Workbench session. Embedded host tests
in `AdminMultiplayer.ent` verified entity-to-slot resolution, exactly one selected
connected player in Admin Tools (BRIEFING and GAME), healing from 0.6 to 1.0,
and KIA recovery into a new healthy character through the production context action.
Central audit logs recorded both heal/respawn requests as applied. Stale captured
claims disabled both actions; server calls with an unresolved requester or stale
claim rejected without mutation. An unclaimed AI entity exposed Heal; the Admin
Tools picker remains limited to connected players. An MCP screenshot confirms the
selected player and enabled Apply button. Raw receipts, logs and screenshots are
in the MCP workspace `.temp/local/evidence/admin-context-shared-20261024`.
This run does not establish physical right-click navigation, remote-peer behavior,
connected-admin logout or POSTGAME coverage. A popup attempt without a menu root
returned `opened=0`; it is not a passing popup-rendering test.

Nearby heal accepts 1–1,000 metres (default 15), measured from the authenticated
requester's living character. `GC_TestAdminHeal(scope, selectedPlayerId,
expectedIdentity, radiusMeters)` sends the production request. `heal-nearby` is a
connected-peer command with the radius in its message. `GC_TestAdminHealAtDistance`
takes a distinct target player ID, fixture distance and requested radius; it places
and damages the target, then dispatches in the same tick for exact boundary checks.
Inspect real health snapshots and receipt-correlated audit logs, including rejection.
`GC_TestAdminRadiusSlider(meters)` uses the visible enabled native slider's SetValue
handler; `GC_TestAdminRadiusState()` reports visibility, value and live labels.
These semantic widget checks do not establish mouse hit testing.

`$Lobby2McpTests:worlds/AdminMultiplayer.ent` inherits the production MPTest
example and adds stock radio, loadout and chat managers in its own layer.
It retained 7 groups, 42 slots and 21 briefings and spawned the local spectator.
Localhost host/peer checks observed join, all-player and selected-player popup
delivery, slot assignment/release and client voice-channel updates. These are
partial checks, not whole-feature acceptance. It lacks terrain map configuration.

Before Play, select Game > Play Game Options > Server localhost through MCP
window tools. It resets when World Editor closes. Verify rplMode=2 and the
listening endpoint in the host log before launching peers. Reopen newly edited
saved worlds before networking: the live entity-creation session produced an
inconsistent replication table; reopening allowed join. Preserve that failure.
Record and restore changed peer profiles, parameters, frame limits and Play mode.

`GC_TestPeerCommand(int playerId, string token, string action, string message)`
dispatches to that controller's owner. Actions: snapshot, message-all,
message-admins, mute-self, priority-self. Use a unique token of at most 64
characters, then read `GC_TestPeerReport(string token)`. Reports are explicitly
client-reported and controller-correlated. Require production server state,
central audit and screenshots as appropriate. Initial reports describe dispatch
time; take a separate snapshot after mutations. Never replay an outstanding token.

Connected non-admin requests to send admin messages or change mute/priority were
rejected by production server gates without mutation. Admin-only messages stayed
absent on the peer. Admin mute/priority replicated, and clearing both restored
voice eligibility. Audio suppression and real login/logout remain unverified.

## MCP calls

Use `workbench_runtime_prepare` then `workbench_runtime_invoke` with the returned
single-use token. Target `GameMode_Lobby3`, component `GC_LobbyManager`. Each
method takes the complete argument list below. `GC_RunCleanupChecks` returns
`bool`; the others return `string`.

| Method | Arguments | Purpose |
| --- | --- | --- |
| `GC_TestSnapshot` | none | Phase, canonical values, visible UI text/buttons, draft state, slots; bounded output explicitly reports truncation. |
| `GC_TestClick` | string widgetName | Click a uniquely named visible button through its production `OnClick`. Missing, hidden, disabled, or ambiguous targets reject. |
| `GC_TestSelectFaction` | string factionKey | Click the visible faction button, e.g. `US`. |
| `GC_TestSelectSlot` | int slotId | Click a visible slot row; production controller and server validation still apply. |
| `GC_TestOpenBriefing` | int displayOrder | Click the briefing row by its displayed one-based order. |
| `GC_TestOpenView` | string view | Invoke the controller's existing pause-menu action for `lobby` or `briefing`, including during GAME. Existing phase/faction checks remain in effect. |
| `GC_TestScroll` | string widgetName, float position | Set a visible scroll layout's vertical position from 0 (top) to 1 (bottom). |
| `GC_TestEditParameter` | int definitionIndex, string action, string value | Exercise `text`, `toggle`, `next`, `previous`, `select` (zero-based option index), or `slider` controls. Supply an empty value for toggle/step actions. |
| `GC_TestWidgetGeometry` | string widgetName | Report screen rectangles and visibility/enabled flags up the ancestor chain. |
| `GC_RunCleanupChecks` | none | Focused reverse-arrival roster, mode-end retention and parameter result-order checks. Not a full phase transition. |
| `GC_TestEndDataLifetime` | none | Diagnostic comparison of a factory result assigned to a function parameter versus an owned reference. |

For example, prepare `GC_TestClick` with `returnType: "string"` and
`arguments: [{"type":"string","value":"Lobby Button"}]`.

`dispatched` means the handler ran, not that authority accepted the operation.
Read state on a subsequent call and inspect receipt-correlated logs. These are
semantic widget calls, not mouse hit tests: `IsVisible` does not prove a widget
is inside its scroll clip. Use geometry and MCP screenshots to verify layout.
No Computer Use or OS input injection is required.
Tests use the player's actual engine roles and do not grant admin permissions.
The embedded host already has an administrator role; clearing only its session
administrator flag does not simulate logout. Authentication tests require a
real login/logout session and remain separate.

## Repeatable embedded-host regression sequence

Start a fresh Play session for each run. Do not run alongside another operator.
The MPTest fixture currently has 7 groups, 42 slots and 21 briefings. Its eight
parameter definitions belong to US. Fixture identifiers stay in this test suite.

1. Snapshot: require PREGAME and revision 0. Run `GC_RunCleanupChecks`; require
   true and all six `GC | Cleanup Test` log assertions. This is not a JIP peer test.
2. Click `Lobby Button`; require SLOTTING and active view 1. Select faction `US`,
   select slot 11; require that row to report the local player and count 1 / 9.
3. Click `Briefing Button`; require BRIEFING and active view 2. After layout
   settles, require `BriefingTitlesScroll` to have room for a title row and
   confirm the full index is readable in an MCP screenshot.
4. Click `BriefingParametersTab`. Require US selected and editable parameters.
   Toggle definition 0 and enter `MCP CHECK` in definition 6 using `text`.
   Require dirty=1, pending=0, canonical values unchanged. Attempt faction USSR;
   require US remains selected and the draft remains intact.
5. Click `ScenarioParametersApply`. On a later snapshot require revision 1,
   dirty=0, pending=0, canonical 0=false and 6=`MCP CHECK`, and status
   `Parameters updated.` Confirm a succeeded central admin audit entry.
6. Select USSR; require no editable parameters. An edit attempt must reject and
   leave revision 1 unchanged. Return to US and exercise each remaining control
   type, reading draft values before submitting.
7. Test a missing widget and an ambiguous name (`BriefingTitleRow`); both must
   reject without changing phase or revision. Never pick the first ambiguous match.
8. Click `Game Button`; require GAME. Use `GC_TestOpenView("briefing")` to return through the existing lobby/briefing
   views and verify parameters are locked. End the match through the existing
   phase request and require POSTGAME with roster/briefings retained and no VM
   exception. A true transport result alone does not pass this check.
9. Stop Play. Preserve exceptions/assertions and screenshots with the exact
   source revision. Do not count an acknowledged engine assertion as clean teardown.

MPTest lacks a map entity and has existing authored-resource diagnostics. Those
are recorded fixture limitations, not proof that production maps work. Separate
remote-peer, delayed JIP, multi-spectator camera, unrelated-mode, and performance
checks remain necessary for accepting the broader cleanup.

## Unrelated-mode comparison

`IsolationProbe/addon.gproj` depends only on Arma Reforger. Open it to obtain an
addon-free baseline, then open `McpTestAddon/addon.gproj` and repeat in the exact
same `$Lobby2IsolationProbe:worlds/Isolation.ent` world. Confirm loaded projects:
the engine can open a dependency when a requested project's dependency is missing,
so a launched process is not sufficient. Linked-root declaration inspection is
unsupported by MCP; native loaded-addon observations and direct source inspection
remain distinct evidence.

The fixture uses stock `SCR_BaseGameMode`, `SCR_PlayerController`, a playable
four-member group with automatic spawning disabled, and a stock US character.
Its explicit base spawn policy allows controlled possession without a deployment
menu. It is a focused component fixture, not a complete mission or network test.

Target entity `IsolationProbe`, component `GC_IsolationProbeComponent`. Prepare
each method with no arguments and string return type:

Run `Snapshot`, `SpawnGroup`, then a later `Snapshot` before
`PossessCharacter`, `VoiceCallbacks`, and a final `Snapshot`. Possessing first
can let stock group management populate the group before the explicit spawn
check; the helper then correctly rejects a second spawn. Start a fresh Play
session to repeat the sequence.

- `PossessCharacter`: assigns the fixture character through the native player
  controller; require `possessed=1`.
- `Snapshot`: mode/controller types, group agent count, native VON disabled state
  and selectable entry count. Compare baseline and addon-loaded runs.
- `VoiceCallbacks`: invokes the real script receive/capture callbacks and checks
  their events and timeout state. This does **not** send audio or prove remote
  reception, audibility, radio routing or microphone capture.
- `SpawnGroup`: calls stock `SpawnUnits` once on an empty group; require four
  agents in a later snapshot. Repeated invocation on populated groups rejects.
- `CheckInactiveMode` (McpTestAddon only): require no Lobby2 manager/mode, reject
  the Lobby2 phase setter and preserve the unrelated mode's state.

In the Lobby2 MPTest world, `GC_TestPlayableSpawnGuard` on `GC_LobbyManager`
checks that an empty playable group stays empty. `GC_TestShutdownIsolation`
calls the actual manager shutdown and reports controller-owned resources and
native VON state; it is terminal, so stop Play afterward. Then reopen the
unrelated fixture in the same process and repeat the comparison to check carryover.
Keep failures and logs, and test actual remote voice separately.

## Admin slot creation checks

`GC_TestAdminCreateSlot(templateSlotId, position)` invokes the owning controller's
production request. Select a registered slot as the approved template; no resource
name is accepted from the client. `GC_TestAdminCreateReject(templateSlotId)` checks
unknown requester, stale group and outside-world position rejection. These embedded
checks do not substitute for a connected non-admin or remote replication test.

`GC_TestSlotCreationState()` reports roster count and server-owned original anchors.
For current/original respawn, create and assign a slot, use
`GC_TestMoveSlotCharacter(slotId, position)` to position its uncontrolled test
character, then `GC_TestAdminSlot(slotId, 7)` (current) and later action `6`
(original). Inspect `GC_TestSlotEntityDetails` after each. Movement is fixture setup;
respawning goes through the production admin controller and authority checks.

The tools UI exposes Create slot from the selected template, three world-coordinate
fields, Review creation and Confirm/Cancel. `GC_TestAdminToolsEdit` also accepts
`AdminCreateX`, `AdminCreateY`, and `AdminCreateZ`. Test malformed coordinates, Cancel
without roster mutation, successful confirmation, selection of the new slot, and
stale-claim rejection after another assignment. Creation currently stages a new
character; conversion of an existing compatible character remains outstanding.

## Player administration checks

`GC_TestAdminPlayerReject()` invokes the production authority validation for
unknown requester, stale account, missing reason, negative ban duration/page and
malformed unban identity, then checks that the local player remains connected.
It does not prove connected non-admin denial or remote disconnection.

The tools panel provides Kick, permanent Ban with required reason/confirmation,
Ban list with pagination and exact-account Unban, plus Server status / Search
session history. The target search filters the history report when requested.
History retains the latest 256 connection events and returns up to 64 newest
matches; account identities are recorded after successful player audit, not early
registration. Reports and ban pages use owner-only responses with admin checks.

Native bans are dedicated-server operations. Embedded Play must report
`unsupported-ban-dedicated-only`, never persistence success. Native local-host
kick produced no disconnect in the embedded fixture; production now returns
`unsupported-local-host-kick` explicitly. Remote kick, ban persistence/reconnect,
ban-list pagination, unban, backend failure/timeout and delayed role revocation
still require dedicated-server and peer validation. No fake ban database is used.

The asynchronous ban service submits once and observes the native callback;
a timeout is uncertain, not proof that the operation did not occur. Inspect the
ban list before another mutation. The API has no cancellation method; mode exit
logs pending operations as unobserved rather than claiming cancellation.

## Additional admin test controls

All methods below target GameMode_Lobby3 / GC_LobbyManager through generic MCP.

| Method | Arguments | Purpose |
| --- | --- | --- |
| GC_TestNativeChat | string action, string channelName, string text | Native chat input/channel/send/history on the local player. See native-chat validation below. |
| GC_TestAdminMessageUi | none | Read notification popup and confirm the removed sidebar widgets are absent. |
| GC_TestAdminMessage | int audience, string message | Production announcement request: 0 legacy admins (rejected), 1 all, 2 selected local identity. |
| GC_TestAdminMessageSnapshot | none | Owner-local message count and latest receipt. |
| GC_TestAdminMessageReject | none | Unknown requester/stale recipient rejection; not a connected non-admin simulation. |
| GC_TestAdminToolsEdit | string name, string value | Edit permitted target search, message, reason, X/Y/Z creation or freeze seconds fields. |
| GC_TestAdminToolsSlot | int slotId | Click the production slot dropdown; does not assign the player. |
| GC_TestAdminToolsSnapshot | none | Read targets, selection, recipients, confirmation, feedback and freeze text. |

The user rejected the current menu design and deferred its redesign to a future
slice. These handler checks do not establish menu visual acceptance.

## Mission freeze checks (in progress)

The production game mode includes GC_LobbyFreezeComponent. Author m_aAreas with
GC_LobbyStagingArea entries: safe world-space center including height, horizontal
radius 1-5000 metres, and optional base-game faction key. Every registered faction
must have an applicable valid area. Defaults have no countdown or areas; enabling
a nonzero countdown without valid areas rejects. Test setup is not scenario authoring.

Target GameMode_Lobby3 / GC_LobbyFreezeComponent through generic MCP:

- GC_TestStagingArea(vector center, float radius): installs an unrestricted runtime
  fixture area only on inactive, unconfigured authority; changes no role or phase.
  AdminMultiplayer fixture uses [130,1,130], radius 50.
- GC_TestFreezeRequest(string action, int seconds): production controller request;
  configure before GAME, start during GAME, extend while active, end with zero.
  Maximum duration 3600 seconds.
- GC_TestFreezeSnapshot(): configuration, remaining time, revision, active/areas.
- GC_TestFreezeAdvance(): requests next phase through production controller;
  invoke only when the transition is the intended test step.
- GC_TestFreezeMove(vector position): stock teleport fixture setup; does not call
  boundary enforcement. Observe GC_TestSlotEntityDetails on a later frame.

Embedded checks observed configure/start-on-GAME, elapsed countdown, extension,
manual end, natural expiry, unknown-requester/missing-area/stale-revision rejection,
and boundary correction. Inside position [145,1,140] persisted; outside position
[210,1,130] returned to the last safe position while frozen and persisted after end.
These are teleport fixture checks, not walking-input or vehicle checks.

Admin control tests observed invalid duration rejection, Cancel without mutation,
configuration, live remaining time, extension and ending through UI handlers.
The menu design remains rejected/deferred; no all-player countdown HUD exists yet.

### Freeze firing regression

GC_TestFreezeWeapon(string action) supports raise, fire and snapshot for the local
possessed character's selected weapon. Fire injects native CharacterFire for one
second in the optional test addon's OnPrepareControls, without calling muzzle
firing. Read ammunition after the interval. Nothing runs automatically.

The first ammo test exposed a production bug: frame filtering preceded character
control preparation, so ammo decreased while frozen. The corrected production
filter runs in a guarded shared character-control hook and preserves inherited
handling. Tested rifle ammo changed 29 to 28 before freeze, stayed at 28 during
freeze, then changed 28 to 27 after ending freeze. This is local embedded rifle
coverage, not remote, grenade, melee, turret or vehicle coverage.

Required follow-up: connected peers and JIP, other weapon actions, movement and
vehicles at boundaries, faction-specific areas, login/logout, mode exit and the
shared-hook unrelated-mode/addon-free comparison. Independent review remains due.

### Connected freeze checks

GC_AdminFreezeTests.c now also compiles in optional test-addon clients. Its bounded
input helper still does nothing automatically. GC_TestPeerCommand additionally
accepts freeze-end, freeze-raise and freeze-fire; every report includes freeze
and selected-weapon state. Read a separate snapshot after the one-second input.
GC_TestAdminAssignPlayer(playerId, slotId) requests assignment through the host's
actual production controller, with current target identity and slot claim.

A client joining an already frozen mission observed configured=600, remaining=589,
revision=4 and active=1. Its non-admin freeze-end request produced the server's
rejected-not-logged-in audit without ending the countdown. After assignment and
force entry into slot 11, the client-reported rifle magazine stayed at 29 across
a frozen firing attempt, then decreased to 28 after the admin ended the freeze.
The client observed remaining=0, revision=5 and active=0 before released firing.

These are owner-client reports, correlated with controller identity and central
server audit. The area fixture is installed only on authority, so the client's
areas=0 does not contradict the replicated countdown or firing state. This run
did not test remote boundary correction, other weapon actions, or unrelated modes.

### Remote staging correction and server ammunition observations

The follow-up connected run used one staging circle centered at (130, 1, 130),
radius 50, and the actual non-admin peer in slot 11. The optional peer actions
`freeze-move-inside` and `freeze-move-outside` request stock teleport locally on
the owning client to (145, 1, 140) and (210, 1, 130), respectively. They only run
when explicitly called through the test harness. Read independent authority
position/ammunition with `GC_TestFreezeServerPlayer(playerId)` on the freeze
component, and a separate peer `snapshot` afterward.

The prior server-only teleport returned success but did not move the remote
character. Production correction now sends the authority-selected safe position
to the character's owning controller, validating current character and freeze
revision before applying native teleport. In the follow-up run, server and peer
both retained the inside position, returned to it after an outside movement,
and remained at the outside position after the admin ended the freeze. Restarting
the freeze while outside returned the player to the authored area center. Server
magazine observations remained 29 after frozen input and fell to 28 after released
input. This strengthens the earlier client-reported firing evidence.

Raw receipts and final script hashes: MCP repository
`.local/admin-implementation/freeze-boundary-progress.json`. The timeout and
failed server-only teleport evidence are retained there. Peers and Play were
stopped, localhost mode disabled, and all three changed peer settings restored.
Vehicle occupants/weapons, grenade/melee inputs, unrelated modes, performance,
and final independent review remain unverified.

### Admin result ordering

`GC_TestAdminResultOrdering()` on the manager requires the actual local logged-in
admin. It submits two invalid freeze requests through the production controller,
observes their production responses, then injects older and current feedback
payloads through the production owner receivers. It neither grants roles nor
changes gameplay state. Expected: distinct consecutive request IDs, two initial
results, no extra result or ban-page event for the older request, and exactly one
additional result and page event for the current request.

Embedded Play returned `first=1 second=2 delivered=2 afterStale=2 stalePages=0
finalResults=3 finalPages=1 last=current-fixture`. A subsequent real configure
request set the authority countdown configuration to 90 seconds. General action
requests now echo controller-generated IDs, including asynchronous native ban
results/pages and authorized follow-camera instructions. IDs match presentation
only; server authorization remains mandatory. Messaging retains its separate
existing request flow. Native dedicated ban completion and remote-client ordering
remain unverified. Evidence: MCP repository
`.local/admin-implementation/admin-result-progress.json`.

The reload preceding this run reproduced the native resource-leak assertion.
Compilation completed after acknowledging it, but that does not resolve the
underlying stability issue. Preserve the assertion receipt in final review.

### Post-admin isolation regression

After current admin and request-correlation code ran in embedded Lobby2 Play,
the same Workbench process opened the stock-mode IsolationProbe world. The
current loaded-addon observation includes Lobby2, McpTestAddon and IsolationProbe.
Results match `docs/tests/isolation/calls.json` run `baseline-final`: stock mode
and controller, explicit group population 0 to 4, successful stock possession,
VON disabled=0 with two entries after possession, one receive and one capture
callback, transmitting=1 and radioTransmitting=0. These direct callbacks exercise
the real script inheritance chain, not microphone or network audio.

Current receipts and hashes are in MCP repository
`.local/admin-implementation/admin-isolation-progress.json`. Baseline results are
reused from the recorded earlier addon-free run. Freeze combat-input and vehicle
isolation still need direct checks. The recurring GameApp.cpp:1287 assertion also
exists in the earlier baseline reload receipt. Its process1672 native loaded
context lists three loaded projects and only the probe in the filtered addon
result; the console's available-addon inventory is not a loaded-addon inventory.
Current crash.log records the assertion but no leaked resource names or counts.
This does not identify or resolve its cause.

### Freeze firing isolation comparison

The optional IsolationProbe now exposes `WeaponInput("raise"/"fire"/"snapshot")`.
It injects a one-second normal CharacterFire input only after explicit invocation
on the possessed `IsolationCharacter`; it never calls weapon Fire directly.
For the Lobby2-loaded run, use `GC_TestLoadedFire()` instead of `WeaponInput("fire")`.
That method uses McpTestAddon's existing freeze-input injector, whose callback
runs before the production freeze hook. The probe dependency's inner injector
could otherwise overwrite suppression and give a false isolation pass.

A fresh baseline process with exactly core, ArmaReforger and IsolationProbe
loaded consumed 10 rounds (200 to 190). The corrected Lobby2-loaded run consumed
the same 10 rounds with weapon raised and CanFire true. Both loaded-project
inventories were observed through MCP. Raw receipts and hashes:
`.local/admin-implementation/freeze-input-isolation-progress.json` in the MCP
repository. The earlier loaded run with weaker hook ordering is retained but
is excluded from this comparison. These checks cover this infantry weapon in
embedded Play; vehicle, grenade and melee behavior remain unverified.

### Connected spectator follow

`GC_TestAdminFollowPlayer(targetId)` resolves the current identity and uses the
actual local controller request. `GC_TestAdminFollowSnapshot()` reads camera
mode, target player, position, eligibility and whether its server receipt remains
pending. It does not change roles or attach the camera directly.

In the connected host-admin test, player2 was assigned and entered slot11 through
production admin handlers. Follow entered mode3 with targetPlayer2. Moving the
remote owner to (210,1,130) moved the attached camera to (221.401,6.46796,133.275).
After disconnect detection, mode returned to0 and targetPlayer to-1. The server
central audit recorded authorization and a matching client-reported-applied
receipt. Receipt matching is one-use, current-admin checked, bounded to10 seconds,
and accepts only defined outcomes; it never represents client camera state as
authoritatively observed.

Receipts, hashes and raw audit are in MCP repository
`.local/admin-implementation/follow-connected-progress.json`. The screenshot
shows the attachment hint and scene but does not show the target in frame, so
useful default framing remains unverified. Remote-admin owner execution, target
death, role revocation, and receipt invalid/expiry/replay branches still need
coverage. Peers and Play were stopped and changed settings restored.

### Priority voice lease integration without microphone capture

`GC_TestVoiceLease(action)` explicitly exercises production authorization and
lease handling on the local authority controller. Actions are grant, observe,
release and snapshot. Grant invokes the production Lobby chat authorization
handler while the owner has no held input, so its grant cannot start native
capture. Observe calls the production server observation handler directly; it
is simulated speech activity, not evidence that native OnVoNUsed fires at the
required cadence. The fixture refuses to run over an active user capture.

Observed: ungranted activity ignored; granted priority activated; expiry cleared
activity; admin mute prevented activation; unmute allowed it; release cleared
it immediately. After production phase transitions and slot11 entry, the normal
character could not acquire a Lobby capture grant and the activity observation
remained inactive despite retaining its priority assignment. All snapshots had
held=0 and capture=0. Unknown requester rejection and concurrent-priority policy
tests also passed. Raw receipts: MCP repository
`.local/admin-implementation/voice-lease-progress.json`.

The installed generated VoNComponent declaration describes OnVoNUsed as a server
event each time VON is used but specifies no continuous-speech cadence. Exact
lease timing, real native activation/renewal, audible suppression, and concurrent
actual speech remain unverified. The expiry snapshot was taken roughly5.5 seconds
after activity, so it does not measure the configured750ms timeout precisely.

### Assignment consistency

`GC_TestAssignmentState(playerId)` counts roster claims and compares the assigned
slot against native group membership, native faction, controlled entity and voice
group. `GC_TestDifferentGroupSlot(sourceId)` only finds an available test target.
`GC_TestAssignmentStaleReject` calls the production authority handler with a stale
identity and compares the observed relationships before and after.

Embedded host checks assigned slot11 before GAME, entered it at GAME, rejected a
stale transfer unchanged, transferred to different-group slot1, released slot11,
and repeated slot1 assignment without adding a claim. Each GAME assignment had
exactly one claim and matching group/faction/controlled entity/voice group. Admin
Free then left zero claims and no native group or faction, with slot1's character
still present. Raw receipts and hashes: MCP repository
`.local/admin-implementation/assignment-consistency-progress.json`. These checks
do not exercise remote propagation or forced engine-failure rollback.

### Ban callback session binding (2026-10-22)

Delayed native ban-service feedback now requires the original requester controller
instance as well as current connection, identity and admin status. Request IDs
are controller-local, so a replacement session must not receive an older result.
MCP reload receipt `1792697672948538100` compiled this change after the recurring
engine resource-leak assertion (`1792697665564476900`) was acknowledged. This is
compilation evidence only: dedicated backend completion and reconnect behavior
remain unverified. Raw checkpoint: `.local/admin-implementation/ban-session-progress.json`
in the MCP repository. The admin menu remains rejected and deferred to a later slice.

### Connected player kick (2026-10-22)

`GC_TestKickRequest` invokes the local controller's production admin request and
never grants a role. `GC_TestKickSnapshot` observes native connection/controller,
slot claim and voice assignment separately. With one real Peer Tool client and
host admin, a missing reason left all four present (`1792697854452443200`). A valid
reasoned kick removed all four (`1792697862708490800`), released slot 11
(`1792697863423297600`), and produced a matching identity in session history
(`1792697902227417000`). The action audit reports dispatch, not claimed completion.

Client teardown did not finish cleanly: MCP screenshot `1792697869822469700.png`
shows the native GameApp.cpp:1287 resource-leak assertion. Its console names
`ThermalProfile system/thermal/thermalProfileTerrainDefault.conf`; attribution
remains unresolved. Do not report clean disconnect UX as passed. Raw checkpoint
and filtered host/client evidence are in the MCP repository's
`.local/admin-implementation/kick-connected-progress.json`, `kick-host-audit.txt`
and `kick-peer-audit.txt`. Peer processes and Play were stopped; original peer
settings (including automatic reconnection) and localhost mode were restored.

### Connected faction rules and joining player (2026-10-22)

The optional peer probe now supports ordinary slot requests and an attempted
admin unlock through production controller requests; it grants no permissions.
With a host admin and ordinary connected peer in SLOTTING, the peer received rules
set before joining (`1792698102607273400`: revision 1, locked, balance enabled).
Its unlock attempt was rejected and audited; rules remained unchanged.

With balance off and faction lock on, cross-faction and leave-slot attempts kept
slot 12. Same-faction movement to slot 13 succeeded. Unlocking allowed movement
to USSR slot 1. Enabling proportional balance allowed return to US slot 12 and
rejected return to USSR under the two-player quota. Every observed assignment
retained one claim with matching native group/faction and voice group. The final
client report agreed with server state (`1792698161699315400`: revision 4, US,
slot 12, unlocked, balance enabled). Invalid actor/revision/mode and quota tests
also passed. Full receipt mapping and filtered audit are in the MCP repository's
`.local/admin-implementation/faction-connected-progress.json` and
`faction-connected-audit.txt`. These are two-player/two-faction checks, not scale
or all-phase coverage. Test processes were stopped and settings restored.

### Player staging countdown HUD (2026-10-22)

The all-player countdown is now implemented in the owning freeze component using
replicated remaining seconds and native widgets. Active concept:
`Art/References/freeze-hud/concept-v2.png`; its brief records dimensions and traits.
An initial stretched-root failure is preserved (`1792698546606920500.png`); a
nested card corrected it. Final embedded render: `1792698651136237100.png` (full)
and `1792698629834540700.png` (reduced Workbench window).

`GC_TestFreezeHud` observes actual widget text and dimensions. At a 2181-pixel-wide
viewport the card measured 408.937 by 102.234, matching 360 by 90 at 1920 width.
Text changed from 02:30 to 02:10 with authoritative remaining seconds. The widget
was absent after admin end (`1792698638324988300`), three-second expiry
(`1792698672514054400`) and GAME phase exit (`1792698674876436500`). Play stopped.
Remote/JIP rendering and unrelated-mode cleanup still require checks. The saved
concept is not runtime evidence or final independent acceptance. Raw checkpoint:
MCP repository `.local/admin-implementation/freeze-hud-progress.json`.

### Remote staging HUD and late join (2026-10-22)

A real ordinary peer joined after GAME staging began. Its report showed configured
300 seconds but remaining 277 and HUD 04:37 (`1792698816723881000`), proving it did
not restart from the configured duration. MCP captured the spectator HUD at
`1792698822348227400.png`. Admin end removed it (`1792698832744027500`). After
production assignment to slot 11 and restart, authoritative possession/group/
faction/voice matched (`1792698852149827200`) and the client rendered the HUD at
360 by 90 in its 1920-wide viewport (`1792698845418205200.png`). Its timer text
tracked replicated remaining time and disappeared on expiry (`1792698879830406200`).
The concept author directly compared both remote captures to concept-v2 and judged
the HUD visually close and legible, with the previously documented lighter panel
and thinner timer. This is iterative direction, not independent final acceptance.

Separate unresolved finding: the late-joining spectator camera intersects its body
in the first capture; skin/foot obscure the foreground. Do not count overall
spectator presentation as passed. Remote HUD validation does not validate that
camera. Peer processes and Play stopped, original settings restored. Full receipts
are mapped in MCP repository `.local/admin-implementation/freeze-hud-remote-progress.json`.

### Spectator body obstruction correction (2026-10-22)

The earlier camera obstruction was another spectator body sharing the same spawn
position, not the controlled body inspected initially. The remote snapshot
`1792700585377729700` found two nearby spectators: the controlled body was hidden,
but the other body retained VISIBLE. A frame-based hide missed that passive proxy.
`GC_LobbySpectatorBodyComponent` now schedules one deferred hide from initialization
and removes the pending callback on deletion. It uses no persistent frame polling.

After compile `1792700631420718500`, the remote late-join snapshot
`1792700694527384000` reported both bodies hidden. MCP screenshot
`1792700699130987000.png` shows the unobstructed view. Production admin assignment
to slot 11 then retained one claim and matching native possession/group/faction/
voice (`1792700718696055000`); the remote ordinary character remained visible and
the spectator camera was removed (`1792700707635034300`). These results cover one
host/one remote GAME late join and assignment, not final whole-task acceptance.
Raw mapping: MCP repo `.local/admin-implementation/spectator-deferred-progress.json`.
Test sessions stopped and original peer settings restored.

### Connected-player respawn and ordinary-client rejection (2026-10-22)
The optional peer fixture now permits sending all nine production admin-slot requests
as the ordinary connected client. All nine were centrally logged as
rejected-not-logged-in; server slot, claim and character remained unchanged.
With the host logged in as admin, current-position respawn preserved the independently
observed moved location (145, 1, 140); original-position respawn returned the peer to
(123.896, 1, 119.864). Both replaced the character, removed the prior entity, and
preserved one claim plus matching group, faction, possession and voice association.
The connected client independently reported the replacement entity and position.
Freeing this occupied slot preserved its character, cleared claim/group/faction,
and returned the remote player to the spectator camera with hidden spectator bodies.
Raw MCP receipts and filtered audit are indexed in the MCP workspace at
.local/admin-implementation/slot-respawn-connected-progress.json and
slot-respawn-connected-audit.log. Forced spawn failures, disconnect races and
broader load are not covered. These checks do not establish whole-task acceptance;
the admin menu remains rejected and deferred to its own redesign slice.

### Connected slot creation, pin/lock, removal and conversion (2026-10-22)
Through production admin requests, a new slot at (150, 1, 145) was created and
assigned to ordinary peer 2. The remote client reported its new slot and entity.
Pinning blocked a remote ordinary transfer; unpinning allowed the same transfer.
Locking the now-vacant slot blocked entry; unlocking allowed the same request.
Occupied-slot removal cleared claim/group/faction and returned the client to its
spectator camera, while preserving the character. Making that character playable
again produced a new slot using the same entity; duplicate conversion was rejected.
Assignment of this converted slot agreed on server and client.
Evidence: MCP workspace .local/admin-implementation/slot-lifecycle-connected-progress.json
and slot-lifecycle-connected-audit.log. This run covers one peer and one template;
group/all locking, created-slot JIP and exhaustive client-roster removal remain
outside these observations. The menu remains unaccepted and deferred.

The connected session was stopped through MCP and its original peer parameters,
profile and FPS limit restored (cleanup receipts are in both connected-slot
checkpoints). Optional GC_TestAdminGroupLock and compact GC_TestRosterLocks helpers
were then added for subsequent group/all-lock and client-roster observations.
The group helper calls the production controller request and grants no permissions.
The roster helper is read-only and available on host and peer. Workbench reload
1792701412206577400 compiled these helpers; their runtime checks remain pending.

### Group/all locks and complete connected-client roster (2026-10-22)
The group-lock request locked exactly slots 11–19; all other groups stayed unlocked.
All-lock and all-unlock affected all 42 slots. Complete host and peer roster rows
matched for these three snapshots, including occupied assignment preservation.
In SLOTTING, ordinary transfer into the locked group/all-locked roster was blocked;
the same transfer succeeded after unlock. Initial PREGAME attempts are retained
but are not lock-enforcement evidence because that phase independently blocks
ordinary slot switching. Controlled=0 is expected during SLOTTING.
The same complete-roster comparison verified that creation added slot 43 on both
peers, and removal deleted its roster row on both peers without disturbing the
other 42 rows. Raw receipt comparison and assertions are saved in the MCP workspace
at .local/admin-implementation/verify_roster_evidence.py, with results in
group-lock-connected-progress.json and filtered group-lock-connected-audit.log.
This covers one connected peer, not JIP, logged-out-admin group RPC or large rosters.

### Connected messaging checks and unresolved long-token display (2026-10-22)
Admin-only delivery reached the admin host and left the ordinary peer history empty.
Two all-player messages reached the peer in order. Host popup observations showed
the first message followed by the second after invoking the production dismiss
button; a native MCP peer screenshot showed the first message. A selected-host
message reached the host and was excluded from peer history. Invalid requester,
stale recipient, ordinary-client admin-send, 513-character content and markup were
rejected without added deliveries, with central audit outcomes.
An exact 512-character message reached both clients, but its 500-character unbroken
token was visibly clipped horizontally in the peer popup. This is an unresolved
display defect, not a passing maximum-length rendering check.
Evidence: MCP workspace .local/admin-implementation/messaging-connected-progress.json,
messaging-connected-audit.log, and screenshots 1792701680534915300.png (short message)
and 1792701797425222700.png (long-token clipping). Native admin logout, history cleanup
on real logout, queue overflow and sustained-rate behavior remain unverified.

### Popup long-word correction: host validation (2026-10-22)
The popup now measures unbroken text runs against its scroll viewport after native
layout update and inserts display-only line breaks. UTF-8 continuation bytes stay
with their leading character; transmitted/history text remains untouched.
Initial attempts without forced native text layout did not resolve clipping and
are retained in the evidence. The final version compiled in MCP receipt
1792702101755378500. Native host screenshot 1792702122603837700.png shows the entire
512-character test message wrapped inside the existing popup. A Cyrillic run also
wrapped; exact receipt assertions confirmed all characters preserved and original
history unchanged for both messages. No menu redesign or concept change was made.
Evidence: MCP workspace .local/admin-implementation/message-wrap-progress.json and
verify_message_wrap.py. Final-fix remote rendering, normal prose, viewport resizing,
scroll overflow, performance and whole-task independent review remain pending.
Play stopped and original peer settings restored after these checks.

### Connected-client popup wrapping regression (2026-10-22)
With final wrapping code from compile 1792702101755378500, the ordinary connected
client displayed the exact 512-character long-token test over multiple lines
(native screenshot 1792702248655751000.png). Peer history receipt
1792702253799671000 retained the original unbroken message. Normal prose with an
explicit newline retained its text in host widget observation 1792702275423477100
and displayed readably on the peer (1792702275908026900.png).
The popup keeps the existing dimensions, typography, charcoal panel, amber outline
and dismiss control. This fixes text presentation without redesigning the deferred
admin menu. Evidence index: MCP workspace
.local/admin-implementation/message-wrap-peer-progress.json.
Viewport resizing, scroll overflow, performance and independent review remain open.

### Connected priority/mute authorization and gameplay boundary (2026-10-22)
New optional helpers call production admin voice requests for a specified player,
observe server capture authorization, and send peer-owned production grant/release
RPCs without holding input or opening a microphone. The speech-observation helper
explicitly simulates the server native-event notification; it is not audio evidence.
Compile: 1792702390603074200.
Host and ordinary peer were designated priority through the admin host. Both gained
channel authorization, and simultaneous simulated speech notifications left both
transmitting and eligible in one server observation. Admin mute replicated to the
peer, cleared active-priority state and made transmission ineligible; unmute restored
eligibility. Channel authorization remained separate from the VON eligibility gate.
Peer release cleared channel authorization.
After assignment and production phase advances to GAME, the remote playable
character's Lobby VON grant was denied, and simulated speech observation could not
activate a lobby priority lease. Assignment/group/faction/voice invariants remained
consistent. This does not verify audible gameplay isolation or native speech timing.
Evidence: MCP workspace .local/admin-implementation/voice-connected-lease-progress.json
and voice-connected-lease-audit.log. No third ordinary connected speaker participated.

### Native administrator-role transition limitation (2026-10-22)
The explicit optional role fixture compiled (1792702609131287800), but native
GivePlayerRole with ADMINISTRATOR returned false (1792702691289491000). The
connected peer remained ordinary (1792702707894859500). No role was granted;
this does not validate revocation, password login/logout, or private-history
cleanup after losing privileges. SESSION_ADMINISTRATOR was not exercised.
The peer stopped with no surviving processes, embedded Play stopped, and original
peer parameters, profile and frame limit were restored. Evidence index: MCP
workspace .local/admin-implementation/admin-role-transition-progress.json.

### Native-chat validation (2026-10-23, in progress)

`GC_TestNativeChat(action, channelName, text)` exercises the stock chat panel and
native transport. Actions include input-down/input-up (ChatToggle), open/close,
cycle, draft/submit, prefix, send and snapshot. The whisper action uses native
SendPrivateMessage with the recipient ID in channelName; history reports its
receiver ID. Send uses native open/select/submit
and clears the draft. `GC_TestNativeChatPeer(playerId, action, channelName, text)`
dispatches the same operation to that real connected owner's client; use report
afterward to read its observation. Dispatch alone is not delivery evidence.

`GC_TestAdminChatRecipients()` reads actual server recipient status and deadlines;
it never changes time or eligibility. `GC_TestNativeChatAuthority` invokes the
production channel callback with a real connected sender but is a handler check,
not transport validation. The acceptance contract is public Admin sending,
admin-always receiving, and a renewable three-minute receive window for a sender.
Exercise actual expiry, non-renewal by receipt, and no backfill in the peer history.
Spectator membership and ordinary routing must use real spawn/leave transitions.

`GC_TestChatVehicleSpawn()` creates at most two stock M1025 test vehicles near the
host. `GC_TestChatVehicleEnter(playerId, vehicleIndex, seat)` invokes the native
compartment API on the real owner's client. A negative seat exits; observe exit
before requesting another entry. Chat snapshots report actual vehicle replication
IDs and native group frequencies. Use two ordinary occupants to verify both
same-vehicle receipt and different-vehicle exclusion; spectator routing is separate.

Raw evidence and pre-slice copies: MCP workspace `.local/native-chat/`. Visual
reference: `Art/References/native-chat/native-chat-v1.png`. Earlier sidebar and
admin-only-channel evidence does not validate the current routing contract.

### Connected session-admin grant/revoke and private history (2026-10-22)

Historical sidebar-chat evidence below is superseded for text chat by the native
channel slice. It still records the original role-event failure and its fix.
Native SESSION_ADMINISTRATOR grant exposed a recursive base role-event invocation:
the native editor-mode listener grants GAME_MASTER inside the admin-role event.
The failure and completed native grant are retained in admin-role-recursion-failure.log;
the role was explicitly revoked before restarting the session. The optional role
fixture now uses SESSION_ADMINISTRATOR, matching the installed voting implementation.
Lobby2's own game mode queues role-event dispatch outside the native callback stack
and removes queued callbacks on destruction. Native role authority remains unchanged.
Compile 1792703114146706600 passed. A fresh connected client successfully granted
session-admin access and sent a private message to both admins. Revocation cleared
private history; a subsequent send was server-rejected despite remaining GM access.
The revoked peer received no subsequent private host message. Regrant did not restore
old private history. A second grant/message/revoke cycle cleared the new private
message while retaining the all-player announcement. No recursive-call exception
occurred during this bounded retest. Peer and Play stopped; original peer settings
were restored. Evidence: MCP workspace .local/admin-implementation/admin-role-connected-progress.json
and admin-role-connected-audit.log. Password login/logout, audible voice revocation,
performance and independent whole-task review remain unverified.

### Independent-review corrections R1/R2 (2026-10-22)
Admin chat and popups now share display-only long-word wrapping. It measures the
text child as well as its scroll viewport, excluding scrollbar space, and keeps
UTF-8 sequences intact. Stored/transmitted messages remain unchanged. Normal words
are measured as complete runs; only oversized runs require per-character splitting.
The 256-event session history now evicts its oldest entry with ordered removal.
GC_TestAdminHistoryRollover invokes the production recorder 258 times against
explicitly isolated history/label containers, exercises the production report,
then restores the original containers. It verifies all retained entries 3..258,
newest-first 64-entry reporting, filtering and unauthorized rejection. It does not
simulate network joins or establish disconnect-race coverage.
Final compile: 1792704493257164700; final history check: 1792704515647260500.
Text source was unchanged after compile1792704382494578100. Raw text comparisons
and captures in MCP workspace review-fix-text-checks.json cover long Cyrillic runs,
normal prose/newlines in chat and popup, and a 512-character popup. The reviewer’s
429-character ASCII case is wrapped in1792704411099776100 and rendered in
1792704414823486300.png. Initial scrollbar overlap attempt is preserved in
1792704346593961700.png; final Unicode chat1792704442211271700.png and popup
1792704448744085500.png were inspected. Full/reduced-window recheck, maximum-history
performance and all prior whole-task verification gaps remain open.
Evidence index: .local/admin-implementation/review-fixes-progress.json in MCP workspace.

### Text capacity and formatter timing follow-up (in progress)

Historical sidebar measurements: the sidebar and its widget-specific timing/draft
helpers were removed by native chat. These numbers are not native-chat validation.

The pre-optimization production chat passed102 sequential512-byte sends: newest100
entries remained ordered and intact, with the oldest retained entry reachable by
scrolling. MCP evidence: chat-capacity-progress.json in the MCP workspace.
GC_TestAdminTextTiming measures the production formatter on the visible chat widget
and restores its original display; it does not inject messages into controller history.
GC_TestTextMeasurement compares immediate native text dimensions with an explicit
widget update. Neither helper establishes multiplayer performance or frame pacing.

The100-entry/50480-byte timing case measured2466ms before optimization and261–262ms
after line-local scanning and whole-prefix measurement at UTF-8 boundaries.
The latter still represents a meaningful hitch, not a performance acceptance pass.
Current compilation:1792705833026524500. Chat/popup ASCII512, Cyrillic and explicit
newline regression evidence: text-performance-regressions.json. This supersedes the
earlier per-character measurement implementation description, not its preserved evidence.
Final capacity rerun and consolidated independent recheck remain pending.
Evidence and intermediate failures: text-performance-progress.json in the MCP workspace.

The subsequent native whitespace-splitting and byte-boundary search revision
measured58ms twice for the same100-entry case (1792706070730783200,
1792706071867022400), with non-newline text preserved. Compilation:
1792706054465760000. This removes full per-character scans from the formatter;
it does not add cached private content or change controller history ownership.
These are formatter microbenchmarks, not a canonical whole-game performance pass.

The optimized production path passed the102-send capacity/ordering/text check
again, with real top/bottom scroll captures1792706336617000500.png and
1792706338554447000.png. See chat-capacity-optimized-observations.json.
Final text checks cover512-byte ASCII, Cyrillic, mixed ASCII/CJK, repeated spaces,
tabs and blank lines (text-performance-final-regressions.json). Requester/stale
recipient rejection still passes at1792706494683797800. Independent recheck is pending.

Four-byte emoji readback remains a separate native-widget limitation: U+1F600
is stored intact by the controller, but native TextWidget.SetText/GetText returns
U+F600 even without the formatter. GC_TestNativeTextRoundtrip isolates this at
1792706430100266600; native-unicode-roundtrip.json preserves the input/output.
Do not count that failing case as general Unicode support or silently change storage.
Final fixture compile:1792706410214986600; production formatter unchanged since
1792706054465760000. Test Play stopped at1792706495143898200.

### R3 visible tab separation correction (bounded recheck passed)

The shared display formatter expands each tab to four spaces; received/stored
messages retain their original tabs. Compilation1792706992071530000 and production
chat/popup checks in tab-fix-observations.json preserve repeated spaces, blank lines
and trailing newlines while displaying the expanded separator. Actual opened
render1792707047477612400.png shows the word boundary in both surfaces.
ASCII512/Cyrillic/newline regressions: tab-fix-text-regressions.json;100-entry
formatter timing67ms at1792707048634934300. The prior102-send capacity evidence
still covers unchanged controller history and non-tab formatting; it was not
repeated for this display-separator-only change. Requester/stale-recipient denial
passed1792707075634475600. Play stopped1792707076296228600.
Native supplementary Unicode and the broader review exclusions remain unresolved.

Independent recheck 3 confirmed visible tab separation in both surfaces, exact
stored originals, existing text regressions and authorization denials. Its report
is `.local/admin-implementation/independent-review/recheck-3.md` in the MCP repo;
fresh render:1792707165079315800.png. Whole-task readiness remains incomplete.

### Created-slot late join and reconnect

`created-slot-jip-gameplay-progress.json` records a slot created and locked before
the diagnostic peer joined. All43 roster rows matched authority. After admin
assignment and the normal phase transitions, server and peer confirmed possession
of the created entity at its configured position. Disconnect cleanup preserved
the entity; reconnecting with the same identity recovered the slot and character.
This covers one peer/template, not account-ID reuse races or concurrent admins.
Factionless admins can reopen Slotting from pause to use the Game header button;
the faction-specific briefing view intentionally remains unavailable to them.

### Native melee staging probe

The optional `GC_TestFreezeCombat` helper accepts `CharacterMelee`,
`CharacterThrowGrenade`, `CharacterFire`, `raise`, or `snapshot`. The `raise` action
prepares the weapon through the stock controller. Combat actions supply a bounded native input pulse
before the production control filter and observes native command state for three
seconds. It never invokes attack/throw methods or implements the freeze policy.
Wait for `observing=0` before judging the result or starting another probe.

Compile1792707885052454500 passed. On the same locally controlled slot1 character,
melee attack-tag frames were0 during staging (1792707934822637700), and315 after
the admin ended staging (1792707945834300700). This establishes suppression and
release for the tested native melee action, not damage against a target or remote
melee behavior. Grenade input produced no throwing frames even without staging
(1792707964627403400); the current grenade accessor returned null. Grenade
suppression is therefore unverified, with loadout/input setup still to resolve.
Turret and vehicle firing remain unverified. Play stopped1792707974726396900.

The follow-up grenade fixture (compile1792708053766939800) distinguishes inventory
from current selection. `GC_TestEquipGrenade` found three grenades and equipped an
existing RGD5 through the stock API (1792708081709672100); it never throws directly.
Both throw and fire input probes still yielded no native throwing frames outside
freeze, with inventory unchanged. Grenade suppression remains unverified; see
`freeze-grenade-setup-progress.json`. The earlier null selected-grenade result did
not mean the character lacked grenades.

The subsequent raised-grenade comparison passed locally. Compile1792708317324389000
adds read-only native throw-input observation without overriding the throw handler.
With an equipped, raised RGD5, active staging produced no native throw input or
throwing state and preserved all3 grenades (1792708363433786100). After the admin
ended staging, the same pulse produced native throw input and throwing state,
and inventory decreased to2 (1792708372875806700). These are completed three-second
observations, not a visual or remote-damage claim. See
`freeze-grenade-behavior-progress.json`; earlier inconclusive runs remain preserved.
Play stopped1792708393884870700. Remote grenade, turret and vehicle checks remain open.

Connected grenade comparison subsequently passed with an ordinary diagnostic
peer. `combat-equip-grenade`, `combat-raise`, `combat-throw` and `combat-melee`
extend the optional peer command fixture; snapshots include completed native
combat observations. `GC_TestFreezeServerCombat(playerId)` independently reads
the authoritative inventory. Frozen remote grenade input produced no throw and
inventory stayed3 on both sides (1792708581643867100/1792708582708802500).
After the authenticated admin ended staging, the same raised-grenade input
produced native throwing state and inventory2 on both sides
(1792708594548697800/1792708596118383300). The remote melee probe reported0 attack
frames frozen and20 released, but selected equipment changed from rifle to
grenade; this is qualified evidence, not the same-equipment local comparison.
See `freeze-combat-connected-progress.json`. The initial whitelist compile
failure1792708464042832100 is preserved; replacing the oversized expression with
an explicit action list compiled1792708475036239800. Production was unchanged.
Peer/Play stopped, original peer settings restored and localhost disabled.
Turret/vehicle firing and broader acceptance checks remain open.

### Mounted-gun staging regression (2026-10-22)

The optional `GC_AdminTurretTests.c` fixture enters a stock M3/M2HB turret and
pulses its normal `TurretFire` action before the production callback. It does not
call native fire setters. With the original character-only hook, occupied-turret
ammunition fell from99 to90 during active staging (1792708996098193400), and
from90 to81 after authenticated end (1792709006895857900). This reproduces a
separate turret-input gap rather than inferring one from source.

`GC_LobbyTurretController.c` now applies the existing active-mode freeze filter
at the turret callback and always preserves the inherited callback. Compilation
passed1792709028530829800. The same local scenario then retained99 rounds during
staging (1792709063144726400), and consumed ten after authenticated release
(1792709069773111700). Audit1792709070465114000 records configure/end. The one-second
input frame counters are neither shot counts nor performance measurements.

Play stopped1792709071127722200; peer settings were unchanged. Remote turret,
addon-free/unrelated-mode comparison for this new global override, vehicle pilot
weapons, and consolidated independent review remain required. This is a bounded
local behavioral pass, not whole-task acceptance. Raw evidence index in MCP repo:
`.local/admin-implementation/freeze-turret-behavior-progress.json`.

### Connected mounted-gun validation (2026-10-22)

Optional fixture compilation passed1792709290659171300. The stock turret was
spawned on the server and entered through its native compartment API on the
owning client. An earlier server-only entry attempt left the client unoccupied
(1792709222265716000); no firing result was inferred from that failed setup.

Connected ordinary player2 completed a one-second/30-frame input pulse during
staging: client1792709366201643700 and server1792709363682913600 retained99 rounds.
After native-admin end, the same occupied turret and input pulse consumed nine
rounds: client1792709380829027500 and server1792709382184156400 both reported90.
Audit1792709382877949600 records the host configure/end actions. This supports
remote suppression/release for this mounted gun, not pilot weapons or damage.

Peers stopped with no survivors1792709399246393700, Play stopped1792709408448583800,
original peer settings restored and localhost disabled. The new shared override
still requires unrelated-mode/addon-free and mode-exit checks, plus consolidated
independent review. Raw index in MCP repo:
`.local/admin-implementation/freeze-turret-connected-progress.json`.

### Mounted-gun unrelated-mode and mode-exit checks (2026-10-22)

With current scripts compiled1792709502753430700, active300-second Lobby2 staging
was observed1792709528456509800 and Play stopped before expiry. Without script
reload or process restart, the stock isolation world reported SCR_BaseGameMode
and SCR_PlayerController1792709556454779300; Lobby2 manager and freeze component
were absent1792709557702787700. The outer test-addon input hook reached the
production turret callback; occupied stock M2 ammunition changed99->89
(1792709565897948500,1792709572234328700). Loaded addons were confirmed at
1792709584766307100.

Fresh addon-free process10000 loaded only core, ArmaReforger and IsolationProbe
(1792709599534894600). The same fixture/stock turret changed99->90
(1792709621846034900,1792709631069093700). Both retain ordinary firing; the
nine-versus-ten round outcomes do not establish identical timing or performance.
This is a bounded test of firing availability after mode exit and with the addon
loaded, not a claim about every vehicle, audio or damage path.

Both processes closed normally after stopped Play. The Lobby2 MCP test project
was restored as process32020 and AdminMultiplayer.ent reopened1792709663772859700;
Play and peers remain stopped. Raw index:
`.local/admin-implementation/freeze-turret-isolation-progress.json` in MCP repo.
Consolidated independent review remains pending; no menu acceptance is implied.

### Remote admin follow and recovery (2026-10-22)

The optional peer transport now invokes production follow requests and reads the
owner camera. Its first client compile failed because the snapshot helper was
WORKBENCH-only (actual1792709864552914200.png); the helper now compiles in the
optional test client. A subsequent source-checksum mismatch is preserved at
1792710077955577000. Host and peer restarted on matching sources before these
behavior observations. MCP configuration now includes the isolated peer log source.

Native SESSION_ADMINISTRATOR grant1792710163538951000 enabled remote follow;
1792710173548161200 reports mode3/targetPlayer1. Target movement was reflected by
camera displacement1792710205638695500. Production current-position respawn
replaced the target entity1792710216465915900 and released follow to mode0/target-1
1792710218580652400. After native role revoke1792710219637237700, a new follow
request left the camera free and was audited rejected-not-logged-in at
1792710229196565600. Existing GM role was insufficient.

The actual peer screenshot1792710186860194700.png was opened: only floor/sky,
with the followed character absent. Initial framing therefore remains a usability
failure despite successful attachment/tracking. No framing acceptance is claimed.
Distinct target death and receipt expiry/invalid/replay remain unverified. The
ThermalProfile reload assertion recurred1792709757180657700; acknowledging it is
not a stability fix. All failures are retained in MCP repo evidence index
`.local/admin-implementation/follow-remote-progress.json`.

Test role was revoked, peers stopped with no survivors1792710247416958000, Play
stopped1792710254251385000 and original peer settings restored. Workbench38268
has AdminMultiplayer.ent open; relay15562 is current. Menu redesign stays deferred.

### Admin follow initial framing correction (2026-10-22)

`GC_SpectatorManualCamera.TryFollowPlayer` now frames the selected character before
using the existing Follow attachment. Only the authorized admin request calls it;
ordinary manual follow retains its existing offset/orientation behavior. The
starting position traces for camera-layer obstructions and refuses insufficient
clearance. Obstructed-space behavior remains unverified.

Compilation passed1792710454177720700 after the recurring native ThermalProfile
assertion1792710448264279100 was acknowledged; no stability fix is claimed.
Remote native-admin request1792710516690157800 attached to target1. The actual
1924x1127 peer frame1792710527875477300.png was opened: full soldier visible near
horizontal center, with stock label and attachment hint. This corrects the prior
empty floor/sky framing in1792710186860194700.png in the tested open-terrain case.
Movement tracking1792710543645792000, respawn detachment1792710552165245300 and
revoked-role rejection1792710563561750100 still pass.

Test role revoked, peer stopped with no survivors1792710571309418500, Play stopped
1792710577835359900, and original peer settings restored. Obstruction, stance,
manual-follow regression and consolidated independent review remain open.
Raw index: MCP repo `.local/admin-implementation/follow-framing-progress.json`.

### Independent recheck and pilot-weapon fixture (2026-10-22)

Consolidated independent recheck4 remains verification incomplete. It reproduced
mounted-gun suppression/release and checked newer JIP, combat, isolation and
follow evidence against originals; no new production defect was established.
The rejected menu remains deferred. Report and raw checks are in MCP repo
`.local/admin-implementation/independent-review/recheck-4.md`.

The optional pilot fixture compiles1792711889830918400 and enters the stock Mi8MT
named PilotCompartment. Native GetPilotCompartmentSlot did not resolve the initial
setup, and casting its attached turret to SCR_TurretControllerComponent failed.
The actual native TurretControllerComponent cannot be modded (compile failure
1792711861691113700). The current fixture injects HelicopterFire only at the
character input callback and reads loaded projectile entities without modifying them.

Pilot snapshot1792711936806594900 reports an overridden turret, six rocket pods
and96 loaded rockets. Frozen pulse1792711946795748600 and released positive
control1792711960745514700 both completed but retained96 rockets. This is
INCONCLUSIVE: no pilot suppression pass or bypass is inferred. Actual cockpit
screenshot1792711970671748000.png was opened; native firing/arming and input
timing still need investigation. Production code is unchanged by this fixture.

The recurring reload resource assertion remains unresolved; ThermalProfile log
1792712023056107300 retains all three new occurrences. Play stopped
1792712012743917000, no peers were started, settings unchanged. Raw receipts,
compile/setup failures and fixture hash: MCP repo
`.local/admin-implementation/pilot-weapon-fixture-progress.json`.

### Pilot input diagnostics and ordinary Follow regression (2026-10-22)

Released-state pilot input was active, triggered and preserved across inherited
callbacks on all437 character frames and429 helicopter frames, yet96 rockets
remained. The optional pilot fixture now uses SCR_HelicopterControllerComponent's
native preparation callback and reads every rocket pod. This remains inconclusive;
no firing/suppression pass is claimed. Owning MCP Ghidra analysis is preparing as
job ef200b7928594eecb4de53ee094c645e; last observation reports running. Details and
full UUID-suffixed receipts are in MCP repo
`.local/admin-implementation/pilot-input-diagnostics-progress.json`.

Ordinary manual Follow was exercised through GC_SpectatorCameraContextAction,
using a live character preserved after a production admin free-slot action.
The existing context action attached with mode3 and matching target, preserving
world position and basis with measured errors0 and0. A later snapshot retained
mode3 and the same camera position. This is a bounded local regression pass for
ordinary offset/orientation preservation after admin auto-framing was added; it
does not establish obstructed or stance behavior. Full receipts and fixture hash:
`.local/admin-implementation/follow-manual-regression-progress.json` in MCP repo.

Latest compilation succeeded. Play stopped, no peers were started and settings
were unchanged. Production scripts are unchanged in this validation batch.
Independent consolidated recheck remains required after remaining work.
## Pilot native-input checkpoint (2026-10-22)

Pilot native-input follow-up: Ghidra analysis of Workbench 1.8.0.13 completed
with 101 warning and 22 error lines retained. A native action initialization/read
path uses `TurretFire`; no defined `HelicopterFire` string hit was found, which
does not establish absence. Changing only the optional pilot pulse to `TurretFire`
still left 96 rockets loaded after 379 active/triggered input frames with freeze
inactive. This remains a failed positive control, not a suppression result.
Production is unchanged and Play stopped. Native provenance, inferred-code
limits and exact receipts are in MCP repository
`.local/admin-implementation/pilot-native-input-progress.json`.

## Follow obstruction checkpoint (2026-10-22)

Follow obstruction checkpoint: the optional follow fixture invoked the production
framing primitive on the same standing target before/after spawning a stock
concrete wall behind it. Camera z moved from 119.231 to 121.51, on the target side
of the wall front at 121.358653; Follow remained attached to the same target.
The opened MCP screenshot showed the target unobscured. This is one static-wall
geometry check, not additional admin RPC authorization or all-obstacle coverage.
Raw evidence: MCP repository `.local/admin-implementation/follow-obstruction-progress.json`.
No production change; Play stopped. Fixture additions await consolidated review.

## Message queue saturation checkpoint (2026-10-22)

Optional `GC_AdminMessageQueueTests.c` calls the production controller send path
and reads history, pending messages, and rendered text without changing them.
The immediate 102-request burst delivered one message; server rate limiting
rejected the remainder. This is not a queue-capacity test.

A fresh embedded-host run sent 120 requests at 550 ms intervals. The central
audit contained exactly message IDs 1 through 120. At completion, history held
100 messages (21 through 120); the popup queue held 100 entries, preserving the
visible message 11 with message 22 next and 120 last. A subsequent automatic
dismissal produced 99 pending messages and displayed message 22, confirmed by
the opened MCP screenshot. Overflow and timed dismissal both ran normally.

Evidence index: MCP repository `.local/admin-implementation/message-queue-progress.json`.
This closes bounded embedded-host saturation coverage. Play was stopped.

The subsequent connected ordinary-peer run delivered 120 production messages
to both recipients. Host and peer each reported history 100 (21 through 120)
and pending 100 (visible 11, next 22, last 120); the opened native peer screenshot
showed message 11. All 120 central audit entries recorded two recipients. See
`.local/admin-implementation/message-queue-multiplayer-progress.json` in the MCP
repository for raw receipts and exact original/restored setting comparison.
The initial original-profile peer process exited before connecting; its cause
is unresolved and that attempt is not counted. The successful run used the
existing isolated test profile and native-generated addon roots. Peers and Play
were stopped, original peer settings restored, and localhost returned to Off.
Consolidated independent review of these optional fixture additions is pending.


### Pilot weapon control boundary validation

Optional character-control pulse fixture now establishes a working stock Mi8 pilot positive control: 96 to 48 rockets before freeze, 48 unchanged during freeze, then 48 to 24 after production admin end. Frozen input was active in 351 sampled frames and cleared by the inherited production filter in all; post-release input survived in 281 frames. Production start/end audits report applied. Evidence: MCP workspace `.local/admin-implementation/pilot-character-input-progress.json`, including raw receipts, earlier failures and native-analysis chain. This is embedded-host behavior only; remote pilot behavior and unrelated-mode pilot baseline remain unverified. No production change was needed. Consolidated independent review remains outstanding. The rejected admin menu is deferred to a future slice and is not accepted.

### Admin voice-channel movement — implementation draft, 2026-10-23

Added production server handler/RPC, authority-only assignment provenance in the
existing voice state, automatic reconciliation preservation, disconnected-owner
cleanup, and shared player-menu picker. Admin destinations include Global, every
registered faction/group and connected personal owner, regardless target faction.
Ordinary self-selection retains existing permission checks and replaces an admin
assignment only when valid. Recipient channel cards include their actual assigned
room. The optional `GC_AdminVoiceChannelTests` invokes production and snapshots
assignment alongside slot/faction/group/moderation; it does not grant permissions.

Active visual reference is `Art/References/admin-voice-channel/concept-v1.png`.
Source work is not accepted: Workbench startup PID 26216 reported "Could not
initialize Steam platform." via MCP native dialog inspection. No compilation,
new multiplayer behavior or actual rendered comparison is claimed. The previous
pilot-fixture compile predates this feature. Next: restore Steam/Workbench startup,
compile, exercise all destination types and ordinary/logged-out/stale denials,
check retuning and unchanged slot/faction/group/moderation on peers, phase/topology
and personal-owner departure, then render/reference checks and consolidated review.

Selection-context extension: shared client-local selected player, named slot/channel destination actions, direct row selection and amber/SELECTED styling added in source. Snapshot selected identity at menu creation; changed or stale selection disables execution. At that checkpoint compile/render/multiplayer checks were blocked by Steam startup. Selection reference is admin-voice-channel/concept-v2.png; no visual acceptance claimed.

### Full-screen admin menu rework checkpoint (2026-10-23)

The earlier menu deferral is superseded. The runtime menu now contains Heal
(selected players, everyone, within 15m of the admin) and Teleport (From/To).
All three player fields share a browsable, live-search roster. Concepts are
`Art/References/admin-tools-redesign/full-screen-v3.png` and `teleport-v3.png`.

Optional `GC_TestAdminPicker` opens fields, edits through the real change handler,
and clicks production rows. `GC_TestAdminToolsSnapshot` observes selected stable
identities, visible rosters and action state. `GC_TestAdminBack` calls the production
MenuBack handler; it does not simulate a physical key. `GC_TestAdminPopupCount`
observes visible workspace popups after closure. `GC_TestAdminSearchRanking` calls
the production ranking function with six fixed cases. `GC_AdminHealTests` supplies
explicit damage arrangement and hit-zone observations; healing and teleport
requests invoke production controller/server paths, with no permission grants.

Workbench PID 16600 loaded this test addon and compiled the integrated code.
Embedded-host tests restored damage for all three heal scopes (one living player,
nearby distance zero), rejected a stale selected identity without healing, skipped
an unspawned spectator, and rejected unauthorized/self teleport. Search and scope
UI checks are recorded in MCP repository `.local/admin-implementation/admin-menu-*`.
These observations do not prove multi-player scope boundaries or replication.

The native host rejected the separate peer for sharing its all-zero identity;
account readiness remains unresolved. Positive teleport, remote roles/logout,
15m inclusion/exclusion, multiple targets, disconnect/recycle and representative
roster performance still require verification. Native reload also reproduces the
ThermalProfile resource-leak assertion and BlockListApi leak. Final independent
review is now recorded in the MCP repository at
`.local/admin-implementation/admin-menu-independent-review.md`: verification
incomplete, not ready. Captured-viewport prototype closeness and bounded host
checks pass; multiplayer, wider UI/context/input/lifecycle and performance/stability
remain required. Reviewer-authored checks were executed mechanically on the primary
MCP connection because direct reviewer tools were unavailable; the reviewer assessed
the raw receipts/logs/screenshots. No ready/complete claim is made. Play/peers are
stopped and normal Lobby2 has been reopened without this optional test addon.


### Multiplayer continuation (2026-10-23, 08:25 UTC)

The earlier all-zero-identity connection blocker cleared: a native Peer Tool client
joined with a distinct real identity. Workbench PID 17764 exercised production UI
and controller/server paths with two living players. Selected healing left the
unselected player injured; multi-selection and Everyone healed both. Nearby healing
included the peer at the observed 15m separation and excluded it at 15.1m. The
ordinary peer's own Heal and Teleport requests were rejected as not logged in,
with unchanged health/position and central audits. Teleport succeeded in both
directions; remote-owner and server positions agreed, and the menu displayed the
observed success. These are bounded two-player tests, not a full-load benchmark.

Two context regressions were fixed: Open Admin Tools now checks the identity
captured by the player/slot context, and Make playable passes the character's
replication-component ID. The latter first failed with rejected-stale-character;
the retest created slot 43, preserved position, disabled AI and rejected duplicate
conversion. Native MenuBack action-value delivery closed the menu and all popups.
Explicit lobby shutdown released the menu, popups and owned input/voice/camera
state. In an unrelated mode after Lobby2, opening Admin Tools left the menu
unchanged, with no admin menu or admin input context.

Raw evidence: MCP repository `.local/admin-implementation/` files
`admin-menu-continuation-input-context.json`, `admin-menu-multiplayer-session.json`,
`admin-menu-continuation-final.json`, `admin-menu-continuation-log.json`,
`admin-menu-two-player-heal.png`, `admin-menu-two-player-teleport.png`, and
`admin-menu-recheck-manifest.json`. The screenshots show a different viewport
aspect ratio from the earlier full-screen captures. Earlier failures are retained.

Verification remains incomplete. Native reload still asserts a ThermalProfile
resource leak; compilation after acknowledging it is not a clean stability pass.
Large rosters/long names, canonical performance, occupied/dead-character edge
cases and role-loss/in-flight transitions still lack full evidence. Stopping the
peer process did not immediately disconnect its server connection; the selected
peer was still valid when Apply ran. That attempt is not a passing stale-selection
test. Subsequent observation confirmed disconnection. Consolidated independent
review remains required; no completion, commit or publication is claimed.


Consolidated recheck: `admin-menu-independent-recheck.md` confirms the current
layout, ordinary-peer authorization denial, two-player tools, context repair,
native Back and bounded isolation evidence. Whole-task verification remains
incomplete; roster load/performance and consequential lifecycle/eligibility cases
remain open. Fresh probe-only evidence in
`admin-menu-leak-probe-baseline-complete.json` reproduces the same ThermalProfile
path and 160-byte resource entry with Lobby2 and Lobby2McpTests absent. This
establishes occurrence without Lobby2, not the native cause or a clean reload.
Cleanup is recorded in `admin-menu-recheck-cleanup.json`: normal Lobby2 PID14964
is loaded with only core/base/Lobby2; test clients and Play are stopped, original
peer settings restored, and optional test addons unloaded.


### Lifecycle and measurement continuation — 2026-10-23 08:54 UTC

The next MCP session verifies real kick/disconnect selection removal, disabled
Apply, stale-identity rejection and same-player reconnect without reselection.
Native seated-player and unavailable-landing Teleports reject without moving the
source. Normal Workbench Stop during a pending Teleport logs cancellation, not
success. Native death followed immediately by production Heal/Teleport requests
keeps the original character dead and stationary; audit records skip/rejection.
Subsequent spectator-target requests also leave observed positions unchanged.

Optional `GC_TestAdminDeadTarget(targetId, destinationId)` arranges native death
and invokes the real controller requests. `GC_TestAdminProfileStart(samples)` and
`GC_TestAdminProfileReport()` record bounded native ProfileData observations,
without changing menu actions, identities or permissions. Four 100-sample runs
at 100ms intervals compare briefing/Heal/Teleport/briefing in one two-player
session. Mean native frame times were 2.394/2.391/2.375/2.411ms. This is bounded
small-roster evidence, not individual-frame pacing, full resource/memory coverage
or large-roster acceptance. GPU timing returned zero and remains unavailable.

Raw evidence and limitations are in MCP workspace `.local/admin-implementation/`
`admin-menu-edge-final.json`, `admin-menu-edge-notes.md`, the four `profile*.txt`
files and `admin-menu-profile-summary.json`. The initial spectator-view sample
is preserved but excluded from the briefing comparison. Large-roster/long-name
workflow coverage and the consolidated independent recheck remain open. The
known probe-only ThermalProfile assertion remains an environment limitation;
no menu-specific leak or clean stability is established. No completion or commit
is claimed by this checkpoint.


## Admin menu workload continuation — 2026-10-23 09:13 UTC

The shared player picker snapshots native roster names once per refresh; Teleport now uses a fixed action label while From/To and the summary retain names. Optional WORKBENCH-only UI fixtures exercised 128 display rows in Heal, From and To, including ranked queries, no-match selection persistence, rejected fixture IDs, final-row scrolling, long field/summary text and repeated popup cleanup. MCP captures cover the smaller viewport and full-screen layout against the unchanged v3 concepts. These are display fixtures, not 128 connected players or renamed native players.

Raw native frame intervals, search/rebuild timings, source manifest and failures are recorded under MCP repo .local/admin-implementation/admin-roster-final.json and admin-roster-validation.md; preceding lifecycle evidence is admin-menu-edge-final.json. The second client in this latest session was rejected by the engine for duplicate identity, so prior distinct-native-identity multiplayer runs remain separately identified. The known ThermalProfile reload assertion and current log warnings remain recorded. No total process/GPU-memory or GPU-present timing claim is made. Consolidated independent recheck remains pending; this continuation alone does not establish acceptance.

## Admin menu resource comparison — 2026-10-23 09:36 UTC

The final menu recheck passes the v3 layout, searchable/browsable pickers, native two-player Heal/Teleport, authorization/audit, recovery and bounded UI workload checks. Its remaining resource finding is under recheck against `admin-blocklist-comparison-final.json` and `admin-blocklist-results.md` in the MCP evidence directory. Two menu-free Lobby2 runs with normal native identities did not emit BlockListApi. A separate process with only core/base/IsolationProbe reproduced the exact BlockListApi leak after initial compilation failure, repair/reload and stock Play with a zero native identity. Lobby2 was absent from its pre-Play native addon inventory; the temporary invalid test file was removed before Play. This is a comparable baseline, not a repaired engine leak or proof of its internal cause. No production changes were made for this diagnostic.

The retained ThermalProfile and BlockListApi warnings must not be described as clean reloads. Full process/GPU resources, GPU presentation timing, 128 connected clients and live microphone transport remain outside this menu slice's passing measurements. Historical checkpoint gaps above are superseded only by their specifically cited later evidence and the final independent verdict.

## Initial AO map framing — 2026-10-25

Opt in with **Enabled** and **Auto Frame Map** on an AO's `GC_MapShapeComponent`. Its existing faction visibility controls eligibility. The first visible snapshot with eligible polygons fits their combined XZ outer bounds, allowing 8% margins and visible briefing side panels. Subsequent updates preserve manual navigation; reopening outside GAME frames again. The first GAME opening frames once independently of briefing; later openings preserve the view. Native map zoom and terrain clamps still apply.

Optional `GC_TestMapFrameSpawn`, `GC_TestMapFrameConfigure`, and `GC_TestMapFrameRead` fixtures exercise this through the production snapshot/overlay. Embedded Arland checks passed for disabled framing, wide/tall concave polygons, two combined polygons, faction-hidden geometry, and preserving manual zoom/center through close/reopen. At a 2613 × 1408 viewport the tall 200 × 900 polygon retained 113-pixel top/bottom margins. The combined 1050 × 300 bounds projected to x=524..1978 between the side panels. Final script reload succeeded after correcting fixture arguments, vector assignment syntax, and the eight-argument RPC limit (labels/bounds now share one JSON metadata argument).

Local MCP screenshots and receipts: `Art/References/ao-map/initial-frame/`. Embedded host coverage exercises serialization/decoding but does not verify remote/dedicated transport or JIP. No resolution matrix, unrelated-mode runtime baseline, or performance benchmark was run for this slice. Normal Lobby2 Arland was restored with Play stopped after testing.

Final clarified lifecycle verified in fresh Play with an authored opt-in: initial PREGAME fit, a new fit after Lobby → Briefing, an independent first GAME map fit, then preservation of manual zoom 2 / center (1499.1,2900.5) after native map close/reopen. No test reset was used in this final run. Screenshots startup.png, briefing.png, game-first.png and receipt r92e75037e7254122bf62e9d52a825095 supplement the local evidence. The earlier once-per-lifetime reopen result predates this clarified behavior.

## Map overlay opening — 2026-10-25

The shape canvas can initially be zero-sized even when the native map reports open. Projection now waits for usable dimensions and rebuilds once on size change through the existing update loop, also refreshing at native map-open completion. The current snapshot bounds are retained until readiness so framing does not wait for another poll. Existing network polling and closed-map cleanup are unchanged.

Optional GC_MapOpeningTests observes opening/projection without changing it. Embedded Arland checks passed through preview, briefing, first GAME opening and stationary GAME reopen with manual zoom preserved. The stationary reopen produced a complete fill 21.1 ms after attachment, before the next ~500 ms snapshot. This is simulation projection timing, not GPU presentation; the full reported 1–2 seconds was not reproduced. Remote transport/latency and alternate resolutions remain untested. Local logs, screenshot and limitations are in Art/References/ao-map/opening-delay/.

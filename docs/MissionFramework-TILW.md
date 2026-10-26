# TILW feature mapping

Reference: Til-Weimann/tilw-mission-framework main at
0f2e423a8fc6d5aeb7faf9ec772cd13d31cf4f9d. Behavior was independently implemented
against installed Reforger 1.8 sources. TILW resources/scripts are not a dependency.

## Instruction catalog

| TILW instruction | Lobby2 action |
|---|---|
| EndGame | End Mission |
| SendMessage | Show Message |
| DisplayChatInfo | Show Chat Message |
| SpawnPrefab | Spawn Prefab |
| SpawnGroup | Spawn AI Group |
| SpawnVehicle | Spawn Crewed Vehicle (or Spawn Prefab for empty vehicles) |
| AssignWaypoints | Assign Waypoints |
| DeleteEntities | Delete Entities |
| SetFlag / ClearFlag | Set Flag, with Set on/off |
| DeleteOpenSlots | Delete Empty Slots |
| ReactivateEvents / DeactivateEvents / RunEvents | Control Events: Rearm / Disable / Run |
| EditRespawnTickets | Change Respawn Tickets |
| EditMapItem | Set Briefing Visibility / Set Marker Visibility |
| RunScript | Run Custom Script |
| ServiceVehicles | Service Vehicles |
| ChangeTime / ChangeWeather | Set Time / Set Weather |
| RandomFlag / FlagSampling | Randomize Flag (%) / Choose Random Flags |

## Other framework features

Nested logical terms, inversion and count matching become Condition Group.
Player/AI casualty monitors become explicit percentage conditions.
Damage and AI control-mode monitors become named-entity conditions.
Extraction/control/presence triggers become Faction In Area, Area Control and
Entities In Area; automatic flags provide continuous or latched results.
Capture Progress provides capture/loss delay, reversing interrupted progress,
initial discovery choice and six named status messages.
Initial, random and derived flags share Mission Flags.
Debug end suppression becomes Preview Mission End.
Flag and teleport actions use native interaction prefabs.

## Deliberate adaptations

- Ticket operations no longer call external COA/PS lobby mods. They use Lobby2's
  canonical slot respawn path and an explicit respawn-wave option.
- Native catalog visibility and faction audiences replace TILW map-item state.
  Existing Lobby2 phase gates remain authoritative.
- Custom script components replace five hard-coded script slots.
- Disabling an event cancels pending actions. Running an event schedules the next
  evaluation; there is no synchronous recursive event execution.
- Randomize Flag clears a failed roll; Choose Random Flags can retain old values.
- Empty percentage populations do not match, even for At Most.
- Flag sources create names on first write, as in TILW. Central entries are optional initial/random/automatic settings. Unwritten names read false; empty names reject.
- Separate timing/area/flag composition replaces an independently polling trigger
  entity system. The low-level trigger query scheduling API is not reproduced.

## Scope and verification

This is an adapted mission-authoring implementation, not a copy of every TILW repository asset or low-level API.
Area spawning, deletion protection, native cleanup exclusion and navigation
rebuilds are now actions. Area conditions can draw debug shapes during local Play;
the runtime exposes a flag-change ScriptInvoker for custom scripts. Native movement controls provide Set AI Hold Position, and crew groups provide Keep
Gunners Seated. These replace the intent of custom static-AI assets; they do not
copy TILW's behavior-tree internals. Their remaining verification is recorded in
the validation document. Existing Lobby2 AO and marker systems remain their
canonical owners. The upstream vehicle-seat lock experiment (explicitly marked
nonfunctional in multiplayer) has not been imported.

See the validation record for tested behavior and remaining multiplayer coverage.

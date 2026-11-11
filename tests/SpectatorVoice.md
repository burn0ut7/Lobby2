# Spectator voice — 2026-10-29

The dedicated spectator prefab has two native VoN components. The transmitter
uses the existing `von_spectator.acp` with its 0.001 m direct range. The receiver
uses `von_spectator_receive.acp`, with a 68 m direct playback range and a direct
bus volume of 1 instead of the old zero-valued constant. Both retain the clear
radio processing path. Normal character audio resources remain unchanged.

Keep the receiver **after** the transmitter in the prefab. In Reforger 1.8.0.13,
live callbacks selected the last registered VoN component for reception. The
first prototype received on the transmitter and still muted direct voice;
reversing registration fixed that. Lobby capture explicitly selects the
transmitter. The stock capture entry point rejects the dedicated spectator body
while Lobby2 is active, including unassigned faction PTT; normal bodies and
unrelated modes retain `super`. Capture and receive activity observers bind to
their respective components and detach on possession/teardown.

This is separate capture/playback ownership, not two alternative voice systems.
Native radio encryption, transmission and delivery remain authoritative. The
listening receivers do not become transmit entries, and spectator PTT continues
to select only the separate lobby radio. No global audio variable is needed.

Body following retains the existing server validation and owner movement, but
uses `BaseGameEntity.Teleport(transform)` directly instead of the player helper
that fades the camera. The dedicated body has simulation NONE, inactive physics,
no gravity or collision layers, no stamina/AI/character sound/editor component,
and disabled damage. Native inventory storage and replication remain for radio
ownership. Equipment-inventory access is disabled through `AllowInventoryAccess`;
the dedicated body's movement, weapon and view controls remain disabled across
native phase and menu transitions. The manual camera owns spectator movement.

## Game navigation checks — 2026-11-11

Workbench 1.9.0.37, embedded Arland, optional `GC_SpectatorGameViewTests`:

- Actual header Game button activation returns to `GC_Lobby_SpectatorMenu` and
  the current manual camera from active-match lobby, postgame voting, and debrief.
  Postgame still opens voting automatically and initially removes the camera;
  only explicit Game navigation restores it. Repeated debrief/Game navigation passes.
- Native `Action_OpenInventory` leaves inventory closed. Radio discovery and
  power remain available; both spectator VoN components remain active.
- Across preview, GAME, POSTGAME and ordinary-body/spectator possession changes:
  spectator movement/weapon/view restrictions remain set, physics remains inactive
  with zero collision layers, damage remains disabled, and character sound,
  stamina, AI and editable-character components are absent.
- A temporary ordinary `Character_Base` retains enabled controls, sound, stamina,
  physics and damage; Game navigation does not create a spectator camera for it.
  The original spectator is restored and the temporary body deleted afterward.
- Reload operation `8e8595829cf4477c8e84a4979117a9e3` succeeded; existing stale
  Arland resource warnings remain. Local MCP captures/receipts are retained at
  `Art/References/spectator-game-view/`. This pass does not repeat microphone
  transmission, dedicated-server/remote-client, controller or other-mode testing.

## Evidence

Workbench 1.8.0.13, embedded Arland plus standalone peer, console session
`logs_2026-10-29_19-03-12`:

- 21:09:43–47: direct and radio 38000 receive callbacks ran on
  `GC_SpectatorReceiveVoNComponent`; 21:09:49–52 capture ran on
  `GC_SpectatorTransmitVoNComponent`.
- The user confirmed microphone audio: spectator hears the living peer locally
  and over radio; the living peer cannot hear spectator chat; two spectators
  hear each other. 21:10:27–31 records the spectator-to-spectator callbacks after
  switching the peer's role.
- The range-only experiment leaked spectator speech to the living player and
  did not restore incoming direct audio. That experiment was reverted.
- Host body matched camera position with simulation 0, active 0, collision layer
  0 and no colliding geometries. After remote camera movement to
  `(1204.46,45.7111,2723.13)`, the server observed body position
  `(1204.46,45.72,2723.13)`. Both peer and server still reported disabled physics,
  stamina/AI/sound/editor components absent, and damage disabled.
- Final observer cleanup compiled at 21:12:38 with no script errors. After
  leaving Lobby2 in the same process, the stock Isolation world reported
  `SCR_BaseGameMode`, `SCR_PlayerController`, voice enabled and two entries.
  Its callback probe returned `received=1 captured=1 transmitting=1
  radioTransmitting=0`, matching the previously recorded addon-free callback
  baseline in `GameMasterPlayable.md`. This is scripted callback and lifecycle
  coverage, not a fresh addon-free microphone test.

`GC_TestVoiceBody` and the peer `spectator-focus` commands `voice-move|0` and
`systems|0` inspect movement and body systems. `GC | Voice path` logs capture,
server use and receive events at most once per two seconds per component;
`GC | Spectator voice` bounds body-follow logs to once per five seconds.

The anti-leak boundary retains the existing near-zero range; an exactly
co-located receiver within 0.001 m is not proven inaudible. Simultaneous speech,
dedicated-server audio and a fresh addon-free microphone comparison remain
unmeasured. Native callbacks prove routing, not audible output by themselves.
Existing Arland resource/plugin/loadout warnings are separate from this change.

## Radio menu ownership

Radio monitoring defaults off. The prior microphone setup explicitly enabled it
through a test command; that was not startup behavior. A separate UI defect made
the menu's dynamically created radio actions expire when `Build` returned: the
button kept only a weak reference. The button now owns its action until detach,
using the existing cleanup. No radio defaults or network protocol changed.

Reload at 21:18:13 passed script compilation. A fresh session reported target 0,
zero receivers and zero inventory receivers. Opening the marker menu and invoking
its real All radios row through `GC_TestContextAction` created two receivers;
reopening and invoking Stop listening cleared target, selection and both inventory
receivers. Selecting and deselecting the individual radio did the same. Menu builds
and clicks were separate runtime calls, exercising the action's retained lifetime.
These are production button-handler checks, not physical mouse input.

MCP captures `Art/References/spectator-radio/listening-selected.png` and
`listening-stopped.png` show checked choices plus Stop, then unchecked choices
without Stop. Local speech remains audible while near the player, including the
local portion of radio speech; stopping radio monitoring does not mute proximity
voice. The test session was left with monitoring off.

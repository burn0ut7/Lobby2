# Weather Action

Add **Weather Action** to a mission event's Actions list.

- **Weather Preset:** the exact name of a preset available in the map's weather configuration. For example, `Clear`, `Cloudy` or `Overcast` where supported. Default: `Clear`.
- **Transition Seconds:** how long the weather transition takes on the game's weather timeline. Default `0` changes immediately. The engine uses hours internally; this action converts seconds to hours. Time acceleration may change the real elapsed duration.
- **Allow Natural Changes:** unchecked by default, keeping the requested weather until another system changes it. Checked allows normal weather progression afterward. The native default state duration is retained (0.001 hours); this is not a guaranteed long hold at the selected preset.

The event's conditions and Delay decide when the action starts. Transition Seconds controls the weather change itself. This action does not change the time of day, choose a random preset or add a second weather simulation.

Example: choose `Overcast`, Transition Seconds `120`, and leave Allow Natural Changes unchecked to request a two-minute weather-timeline transition to persistent overcast.

The manager validates the exact preset against the loaded weather-state list before calling the engine's server-only `ForceWeatherTo`. Missing managers, unknown names and invalid durations are rejected. Valid durations are 0 through 86400 seconds. Native weather owns the transition and networking; the action adds no polling, callbacks, global override or custom replication. Other weather controllers or mission-start resets can still supersede it. The native call has no success return; dispatch is not proof that a visible transition completed.

Reference: TilW `TILW_ChangeWeatherInstruction`, commit `4a5300f94fb9323ce4942ef296bcffcdda922626`. Installed catalogue `c52648a62fcb4229ad089610d060ea44`: `scripts/GameCode/World/TimeAndWeatherManagerEntity.c:257-327` for server authority, transition creation, looping and notifications; `scripts/Game/Editor/Containers/Attributes/SCR_AutomatedWeatherEditorAttribute.c:21-24,79-85` for state enumeration and seconds-to-hours conversion.

Verification: source/logic review and Workbench script compilation/reload. No in-game tests were run, as requested. Actual transition timing, appearance, multiplayer delivery and picker presentation remain untested.

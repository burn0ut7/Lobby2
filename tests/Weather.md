# Default weather handler and fog

`GameMode_Lobby.et` includes the stock `SCR_TimeAndWeatherHandlerComponent`.
Derived game modes inherit it. Stock defaults apply, including an 08:00 starting
time and disabled fog override. Configure mission-specific values on the game-mode
instance and save the world. No custom weather implementation is introduced.

For visible fog, enable **Fog Override** and adjust both **Fog Amount** and
**Fog Height Density** in this component. Both sliders range from 0 to 1.
The toolbar Weather Editor preview is separate from these saved startup values.

## Validation — 2026-11-11

Workbench 1.9.0.37, embedded Arland_Lobby, existing Lobby2McpTests project with
TilWMissionFramework loaded. Tests exercised the production stock handler:

- Default prefab: live component inspection confirmed the inherited handler and
  fog override disabled. `GC_TestWeatherSnapshot()` reported server=1, handler=1,
  fogOverride=0, fog=0, heightOverride=0, height=0.
- Runtime `SetupFog(0.6, 0.1)` read back both overrides and exact values, but the
  airfield view looked nearly clear. `SetupFog(1, 1)` visibly obscured the distant
  hangars and tree line. Low height density can therefore look ineffective in this
  view; this test does not establish a universal visibility distance.
- Temporarily saved 1/1 fog overrides in the base prefab, stopped Play, reloaded,
  and started a fresh session. Without a runtime fog setter, readback returned
  server=1, handler=1, fogOverride=1, fog=1, heightOverride=1, height=1. A fresh MCP
  screenshot confirmed dense fog. The temporary fog defaults were then removed.
- Final reload and another fresh session confirmed the shipped defaults again:
  server=1, handler=1, fogOverride=0, fog=0, heightOverride=0, height=0 (receipt
  `r986ff721977e43db8d1f70a009eae0eb`). Play was stopped after testing.

Startup receipt: `r5b0307f12f134ec4947d40cce32baabc`; subsequent readback:
`r0e6b10e426b64539bee0fe527f4bd444`. Screenshot timestamp: 2026-11-11 07:30 UTC.
Reload `ffdb50c5c192409f9a8aa3571a5bfd12` compiled without script errors; existing
ArlandLobby resource-reference errors persisted. Log session:
`logs_2026-11-11_02-27-33`, reload lines 22042–22202.

The optional `GC_TestWeatherSnapshot()` helper is read-only and runs only when
explicitly invoked through MCP on `GC_LobbyManager`. No automatic test behavior
or shared stock-class override was added. Dedicated clients/JIP and other terrains
were not tested.

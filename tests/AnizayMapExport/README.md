# Anizay map export fixture

Optional Workbench project for Anizay workshop addon `6266DCA9193C705E`.
Production Lobby2 and the regular MCP test project do not depend on Anizay.

Make the installed Anizay folder and `tests/McpTestAddon` discoverable in
Workbench's addons directory (directory junctions are sufficient for Workbench).
Open this `addon.gproj`, then `worlds/playback/Anizay_Playback.ent`.
The fixture inherits Anizay's terrain and uses its own AI-world prefab.
It does not modify the installed mod archives.

Start Play and run the website export with `scripts/maps/anizay.json`.
Local export settings must include the actual installed Anizay directory in
`pakRoots`, rather than its junction path. See the website's
`docs/manual-map-import.md` for the package/upload workflow.

The fixture has a US Ammo Team slot. Use normal Lobby2 phase/slot handlers:
Pregame -> Slotting -> claim slot -> Briefing -> Game. Existing explicit
`GC_TestPlaybackTerrain` helpers place the player at terrain height and exercise
movement and stance. Production telemetry owns recording and output files.

Validated with Anizay 1.0.12 and Reforger 1.8.0.13. Live X/Z map bounds are
0..10240 metres. Play is left stopped after capture.

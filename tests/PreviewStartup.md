# Preview startup restoration — 2026-11-10

Pressing Play in `worlds/examples/arland/Arland_Lobby.ent` must open
`GC_Lobby_PreviewMenu` in `PREGAME` without invoking a test or advancing phases.

Commit `42425c7` deleted the menu/input/map configuration resources. Subsequent
changes recreated only the admin and debrief registrations. The existing startup
handler still requested preview correctly, but the engine rejected its missing
menu preset (ID 60).

Restore the six missing menu registrations and their existing input contexts,
map configurations and player-table tooltip resource from `aa3525b`. Preserve
the current admin/debrief registrations. No gameplay or test startup code changed.

Verification on Workbench 1.9.0.37, embedded Play, existing Arland lobby world:

- Reload completed without script compilation errors.
- Fresh Play opened preview automatically in PREGAME; runtime inspection reported
  `top=GC_Lobby_PreviewMenu focused=1`, with map, navigation and player list visible.
- The existing test helper sent the production MenuBack action down/up; the
  resulting top menu was `PauseMenuUI focused=1`.
- MCP screenshot: local `tests/evidence/preview-startup-20261110/preview.png`.
- A second Reload/Play with the tooltip resource restored again opened preview
  automatically (lines 25616–25617), then `MapPreview.conf` (line 25664), without
  missing menu/tooltip errors. Final capture: `preview-final.png` in that folder.
  Reload operation: `dcbcb17ed0f44d8cb639d54efa944cf3`; Play operation:
  `ddcb2b86053349a9b750b1419aedbd19`.
- Evidence: Workbench session `logs_2026-11-10_22-02-05`, failing startup at
  lines 24479–24485; restored startup at lines 24923–24924. Runtime operation
  `b157081683a84010b060486691bdc5ec` confirmed the pause interaction.

Existing authored map markers and missing mission-header information are visible
in the screenshot. Existing invalid mission-flag conditions and unrelated resource
diagnostics remain outside this fix. Multiplayer, physical input and full journeys
through the other restored menu registrations were not exercised in this pass.

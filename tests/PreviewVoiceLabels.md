# Preview voice-channel labels

2026-10-23, Workbench 1.8.0.13, Arland_Lobby, embedded authenticated host.

Reproduction: enter Preview, open Voice before entering Lobby. Seven group
headings were empty although querying their native callsigns returned names.
Entering Lobby created new cards with the correct names. Preview cards were
created before initial callsign assignment and did not observe its event.

Each group card now observes the native callsign-change event and refreshes its
presentation. Reconfiguration removes the old subscription; detach removes the
current one. No voice routing, channel visibility or authorization policy changed.

MCP checks with the optional test addon:

- Before fix: `GC_TestVoiceListOrder` reported seven empty labels with nonempty
  current callsigns in Preview; all seven matched in Slotting.
- After native script reload and fresh Play: all seven Preview labels matched
  native callsigns; Global, both faction HQs and personal channel remained named.
- Advanced through the production Lobby button and opened Voice: all seven
  Slotting labels still matched. MCP screenshots captured both views.
- Reload and subsequent runtime log contained no SCRIPT errors. The previously
  observed engine resource-leak assertion occurred during reload teardown and
  was acknowledged; this task does not resolve that warning.
- `git diff --check` passed. Remote clients, JIP and audio were not tested.

Local evidence in the tooling workspace's `.local/admin-moderation/`:
`preview-voice-before.png`, `preview-voice-after.png`,
`slotting-voice-after.png`, `preview-voice-receipts.json`.

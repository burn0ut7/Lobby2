# Single voice indicator — 2026-10-24

Reuses the existing player-row indicator location and 26-unit icons. Removed
separate priority/admin-mute badges. Admin mute displays lockedCircle, taking
precedence over priority/activity/deafen. Otherwise priority displays microphone,
RGB 170 gray while idle and white during activity. Without these flags, existing
speech/deafen images remain; normal speech now has full opacity while idle retains
0.38 opacity. No voice routing, moderation authority or shared overrides changed.

Native reload passed in Workbench 1.8.0.13. Optional AdminMultiplayer embedded
host test used production admin requests and deafen/tab button handlers. Local
test activity pulses invoked the existing activity event, exercising the production
row listener and 500ms idle timeout; this does not establish microphone capture
or audible transmission. Widget snapshots confirmed gray -> white -> gray,
admin mute replacing priority, one indicator while muted+deafened in Players,
deafen restored after clearing flags, and ordinary idle restored after undeafening.
The four unified-*.png captures show the actual UI (3840x2160 source, 1920x1080
MCP output). Prior screenshots in this folder are historical references.

Raw MCP evidence: reforger-script-tools/.temp/local/evidence/voice-unified.
Log session logs_2026-10-24_07-44-17 correlates production requests and receipts.
Priority idle/active/idle receipts: rc22dbaea30a04e26891e6ff293e2bd86,
rd2b3da73135c4c32857f4aca9ded97ae, rb582149b7f3d4df7886153dbbd2321bd.
Muted Players snapshot: rca8a4399138b4d108e22ad86a99d4851.
Restored deafen/idle: rb7264f2a075e422194e4a5773429884d,
ra0185aa0f7cf45949a5125729c7107f4.

Existing missing-map/resource diagnostics remain in the fixture. Remote-client
voice capture, physical input, constrained viewports and long names were not
retested. This is presentation validation, not a new audio-routing validation.

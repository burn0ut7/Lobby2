# Chat channel prototype

Implemented 2026-11-11. The user deferred further runtime testing to a later
manual slice after implementation and script compilation.

## Intended channel access

| Authoritative state | Channels |
| --- | --- |
| Preview, Slotting, Briefing | Global, Faction, Admin |
| GAME, playing | Admin |
| GAME, spectator | Global, Admin |
| POSTGAME, any player state | Global, Admin |

Faction additionally requires native faction membership and delivers only to the
same faction. Global replaces the separate spectator conversation. Selection,
send processing and delivery use current eligibility. Old received history stays;
ineligible players receive no new messages. Admin remains publicly writable,
with permanent reception for current admins and a renewable three-minute reception
window after a non-admin sends. Receiving does not extend that window.

The four unused numbered routes (Group, Vehicle, Local, retired Spectator) remain
serialized to preserve numeric channel identities but deny availability, send
processing and delivery. Whisper is absent from the chat prefab. A live probe
showed that native private messages bypass PrivateMessageChannel callbacks, so
callback denial was insufficient. No global native channel override was added.

Debrief uses the same native ChatHud and GC_LobbyMenuChat lifecycle as existing
lobby menus. Shared panel/input changes remain guarded by the active Lobby2 mode.

## Completed checks

- Saved scripts compiled successfully through MCP Reload in the existing Lobby2
  MCP Tests project and Arland world. Final own Reload operation:
  `a62dae4d618e41b88eeff707ec47a911`; console session
  `logs_2026-11-11_02-27-33`, lines 25151–25311. No script compile errors.
  The existing ArlandLobby world GUID/name diagnostics remain unrelated.
- `git diff --check` passed for all changed chat files.
- Before the user deferred further testing, embedded Play exercised production
  phase and slot assignment handlers: Preview/Slotting/Briefing allowed Global;
  native faction assignment made Faction available and a Faction message arrived.
- GAME playing exposed only Admin. The Global server processing callback rejected
  the sender; a raw Global send added no message. The native UI displayed Admin.
- After a real character death, GAME spectator exposed Global/Admin, delivered a
  Global message and rejected Faction in the server processing callback.
- Removing the Whisper configuration compiled and loaded with
  `whisperConfigured=0`. A raw self-whisper produced no immediate message or crash;
  delayed and remote-recipient results were not completed before testing stopped.
- Optional `GC_TestNativeChat` now reports phase, whisper configuration and
  server-side self-delivery eligibility for future validation. Existing native
  send, channel-cycle and authority-processing probes are retained.

These are partial single-host observations, not multiplayer verification. The
source review found no further defects; deferred coverage must not be inferred
from the compile or callback results.

## Deferred manual slice

- Verify the final prefab rejects native Whisper to a remote recipient, including
  delayed reception and direct requests rather than only the UI path.
- Open debrief while alive and dead; cycle Global/Admin, send and receive, cover
  focus loss/gain, Escape, reopening and duplicate input/listener prevention.
  Inspect the rendered ChatHud against the existing lobby chat placement.
- Check Global isolation between GAME players and spectators, same/cross-faction
  delivery in briefing, raw unauthorized sends, concurrent phase changes and JIP
  on dedicated server, listen host and ordinary/admin clients.
- Recheck non-admin Admin-channel expiry, renewal, no earlier-message backfill,
  authenticated admin reception and logout behavior.
- Verify the native selector updates on phase/player-state change, preserves
  usable input and cannot select the retired routes through prefixes.
- Run unrelated-mode and mode-transition checks against an addon-free baseline.
  These were not run for this prototype.

Raw local MCP receipts are in the tooling checkout at
`work/chat-prototype/receipts.json`. An interrupted screenshot captured the editor,
not the intended game state, and is not visual evidence for this prototype.

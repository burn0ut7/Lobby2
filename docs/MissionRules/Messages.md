# Message Action

Use one **Message Action** (`GC_MessageAction`) in an event's Actions list.

| Delivery | Result |
| --- | --- |
| Players | Popup for all connected players, or the selected Factions. Empty Factions includes spectators. |
| Admins only | Popup only for players currently logged in as administrators. Factions is ignored. |
| Server log | Writes the message body to the existing mission-framework server/Workbench log, including its event name. No popup or message-history entry. Title, Factions and Warning are ignored. |

**Warning** selects the existing warning icon, color and sound for either popup delivery. This provides ordinary messages, player warnings, admin messages, admin warnings and log output without separate action types. Add multiple Message Actions when an event should both notify players and write a log.

**Title** is optional, up to 80 characters. Default headings are Mission, Mission Warning, Admin or Admin Warning. The body allows up to 512 characters for popups; markup is rejected. Log output keeps the existing log command's text limits. Blank/whitespace-only messages and invalid delivery values are rejected. Existing faction validation and duplicate-recipient suppression are unchanged for Players.

Admin delivery checks the native current admin roles on the server and again on the receiving controller. It is trusted server-authored mission logic, not an admin request or a client-send permission. Logging in later does not replay old messages. Logout removes admin-only history, visible notices and waiting notices through the existing role-change lifecycle; queued delivery also rechecks login. Ordinary player notices remain visible. Existing admin audit/history messages retain their no-popup behavior.

All popups use the existing bounded notification stack and lifetime. No new layout, transport, timer or global override was added. Old saved Message Actions default to Players and preserve their Warning setting. Old `GC_LogMessageAction` resources still load and execute, but that type is hidden from new authoring; use Message Action / Server log instead.

## Verification

Workbench 1.9.0.37, optional Lobby2 MCP Tests, Arland world, embedded local server:

- Reload passed without script errors; pre-existing Arland GUID resource diagnostics remain.
- `GC_TestUnifiedMessageValidation`: **9 checks, 0 failures**, receipt `rc9f640cfc07b48fc98445fc6b163a6fe`.
- Actual event execution produced Mission, Mission Warning, Admin and Admin Warning popups. Each event ran once; invalid source dispatch was rejected. Warning icon/accent state was verified. Sound handles were created; audible playback was not assessed.
- Server log delivery produced `GC | Mission Framework | unified-message-test: Unified message log verification 2026-11-11` at console line 25082, with popup counts unchanged.
- Actual native role revocation prevented admin delivery and left history unchanged. After the existing asynchronous role notification settled, console line 25997 reported `admin=false visible=1 queued=0 admins=0`: ordinary player notice retained, both visible admin notices and the queued admin warning removed.
- Original MCP captures are in the tooling workspace at `work/unified-messages/messages.png`, `admin-warning.png` and `logout.png`.

The native disposable test refused role restoration (`restored=0`), so Play was stopped afterward; no persistent permissions or authored-world changes were made. Remote dedicated peers, multiple screen sizes, and editor save/reopen of the new delivery field were not separately exercised. Test helpers live only in `tests/McpTestAddon/scripts/Game/GC_UnifiedMessageTests.c` and run explicitly.

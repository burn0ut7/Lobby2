# Mission endings

End Mission keeps the serialized result values 0–3 (complete, faction victory,
faction defeat, faction draw) and adds 4 (mission failed). Select the primary
faction for faction results. Victory automatically includes that faction, plus
up to 16 additional winning-faction entries; duplicates count once. Unknown
factions reject the entire action before ending the match. Additional winners
on non-victory results are invalid.

The optional description is trimmed and limited to 512 characters, without
markup. At actual completion, a nonblank description sends one mission notification headed by the
primary faction and result. Blank/whitespace descriptions send no notification.
The debrief opens on Mission result, retaining the description and result factions;
statistics and activity remain available through the existing sidebar.

Native SCR_GameModeEndData remains the result/faction authority. History stores
endReason, endTitle, endMessage, resultFactions and winningFactions. Existing
mission_results joins winners to the last GAME commander by faction;
defeat/draw/failure do not invent winners. Mission-history outcome additionally
supports complete, failed and defeat. Deploy the updated StatsIngest contract
with the addon; schema.sql appends end_title/end_message to mission_history.
Old records without those optional strings remain valid.

## Pending ending countdown

`EnterPostGame` now requests a 120-second countdown through the existing
announcement timer. It keeps GAME, player activity and statistics running.
No victory popup, final history, or debrief is published until expiry or force.
The first accepted request retains its native result; repeated ending requests
cannot overwrite it or restart its deadline.

In Admin Tools > Boundaries > Announcement timer, pending endings show:
Set time (remaining minutes), Extend (additional minutes), Cancel ending and
End now. Time changes accept 1-3600 seconds and never exceed 60 minutes.
The countdown is always shown to all players, including spectators/JIP, using
the existing timestamp-adjusted HUD transport. It replaces an existing ordinary
announcement. Ordinary announcements remain display-only and cannot overwrite
a pending ending. Stop all boundaries also cancels a pending ending.

Cancellation clears the timer and its retained result; it does not call native
EndGameMode. A later new mission end request can start another countdown. The
existing admin phase override can still force completion, preserving a pending
result if present. Authority checks active login on every timer action; revision
keys reject requests for a previous pending result.

Countdown checks, Workbench 1.9.0.37, same Arland fixture:
- Default countdown retains GAME, collecting=true, finalized=false and sends no
  result popup. Repeated requests leave its result/deadline unchanged.
- Set time then Extend adjusted remaining time through production handlers and
  actual admin buttons. Unknown requester, stale revision and all four controls
  after explicit host logout were rejected without mutation.
- Cancellation at two seconds remained GAME with no pending result after the old
  deadline passed. The production Cancel ending button also preserved GAME.
- End now produced POSTGAME, one final message, both original US/USSR winners,
  and the original description. Final statistics completed normally.
- Natural 120-second expiry produced POSTGAME, finalized statistics, the retained
  US/USSR result and description, one final message, and a hidden countdown HUD.
  Live captures verified the slim gameplay countdown and all four admin controls.
- Audit output recorded accepted controls and unauthorized/stale rejections.
- Reload compilation passed. Admin menu registration, also removed by 42425c7,
  was restored with native MenuContext so the existing timer UI can open.
- No dedicated-peer or alternate UI-scale testing in this pass. No shared
  base-game overrides were changed; the phase sequence stays unchanged.

## Verification — 2026-11-10

Workbench 1.9.0.37; Lobby2 MCP Tests, existing Arland lobby world. Optional helper
GC_TestEndingPrepare uses production phase/action owners and checks pre-match,
wrong-engine and unknown-faction rejection, failure-with-winners rejection, and
whitespace normalization. GC_TestEndingRun(result, message) checks acceptance,
notification sequence changes and repeated-ending rejection. GC_TestEndingReport
reads the production debrief report.

- Reload compiled the changed game scripts. StatsIngest built with zero warnings
  and errors; captured victory and failure histories passed its --validate route.
- US primary + USSR/US/USSR additional entries produced US/USSR once each in both
  native-derived debrief and history, one notification, and no repeated ending.
- Failure with whitespace produced no notification, no winners and an empty
  description. The actual debrief showed Mission failed without an empty message.
- Production sidebar clicks switched between All and Mission result. Returning
  to the cached result did not add a duplicate request (two pages/two deliveries).
- A 512-character description exercised wrapping and result sizing. UI captures
  are retained with the task evidence.

The pre-existing deletion in commit 42425c7 removed menu registration, preventing
the debrief from opening. Restored only its preset using native MenuContext;
Workbench required a restart to discover the restored resource. Other removed
menus and the fixture's obsolete Arland resource reference remain unrelated
diagnostics. No shared base-mode overrides changed. Dedicated peers, alternate
UI scales/gamepad, a populated CO roster and a live PostgreSQL migration were
not exercised in this pass; the existing commander join was inspected.

Installed source evidence: catalogue c52648a62fcb4229ad089610d060ea44,
game/data/data007.pak/scripts/Game/GameMode/SCR_GameModeEndData.c lines 14–53
(native multi-faction construction) and GameOverScreen/EGameOverTypes.c lines
9–43 (reason values). The restored preset follows the prior project definition
and installed Configs/System/chimeraMenus.conf native menu conventions.

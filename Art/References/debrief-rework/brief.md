# Debrief rework

Active concept: `concept.png`, a deterministic layout wireframe with illustrative
data and no world background. Keep the existing native BlurWidget and translucent
surface, sparse typography and horizontal Private / faction / Other players tabs.
Player selection opens Overview / Combat log; search applies to the current view.
Reuse the existing admin table row, edit field and native button handlers.

Installed reference: catalogue revision `721cd7b05eb54d3ca3591d9097af381d`,
`game/data/data010.pak/UI/layouts/Menus/DeployMenu/DebriefingScreenStatistic.layout`
lines 1–30 and `DebriefingScreenStatisticsTile.layout` lines 1–65. Native size
layouts, vertical row containers and Roboto Condensed remain the presentation basis.
The existing Lobby_Debrief.layout supplies the supported blur mechanism.

Data ownership: final counters remain GC_MissionStatsComponent projections of its
backend batch. Combat entries remain references owned by GC_SessionLogComponent;
the debrief never starts another damage/death collector. Client identifiers are
final-report ordinals, never persistent account identities.

User-directed revision: use the full viewport. Move Private/factions/Other players
into a left navigation column; show the overview counters across the content
width and the remaining stats in two columns. Hide overview heroes in player and
combat lists so those tables can use the full content height. No explanatory copy.
The original wireframe is superseded for geometry by this revision and final live
captures; retain its sparse monochrome style.

Final user-directed refinement: remove AFTER ACTION REPORT and generic Players
headings, move content up, put Back (Esc) at the bottom-right edge. Overview uses
six prominent metrics: shots, player kills, deaths, AI kills, grenades, bandages.
Rename Combat log to Activity. Remove the player-list faction column; use native
faction colors on player names. Keep the faction overview selections. Remaining
copy is labels, counts, loading/errors and retention status.

AI refinement: reuse the faction overview geometry and its two native top-right
buttons as Players / AI. The selected AI state scopes all visible totals and
ammunition to AI. Use six combat-only metrics and omit uncollected player-only
statistics. Active evidence: ai-final.png. No additional paragraphs or panels.

Navigation refinement: sidebar order is All, registered factions, Private, Other
players. All is the opening selection and retains the Players / AI switch.
Active live capture: all-final.png. Sidebar label/decorative widgets ignore
pointer input so the native button receives the whole row's hit area.

Current refinement: All and each faction now show combined player + AI totals
and ammunition. Remove the Players / AI toggle from those overviews. Rename
Private to Your stats; personal Overview / Activity remains available. The prior
all-final.png and ai-final.png show superseded toggle states. New live captures
and further testing are deferred at the user's explicit request.


Current refinement: three overview figures (Projectiles, Killed in action, Participants), scoped to the selection. Participants retains the existing human count. Removed search and the second hero row; all remaining summary fields form one vertical list, with ammunition alongside. Existing screenshots predate this refinement. Testing and new captures remain on hold at the user’s request.

Player drill-down refinement: every visible cell in Other players routes clicks to its row. A resolved player opens the shared personal Overview/Activity with their name and report ordinal; Back/Esc returns to the list. Player-row cells no longer own hover tooltips; the opened report exposes the details. Activity cells retain their tooltips. Source review only; testing remains deferred by request.


Mission activity refinement: Activity log is the last sidebar entry after Your stats and Other players. The top-right Overview/Activity controls are removed; overview figures and tables move upward. Personal reports offer View activity at the lower left; Back/Esc returns from personal activity to that report, then to the player directory. Mission activity projects the existing shared combat buffer, including AI/player damage and deaths, through the authorized POSTGAME report queue/cache (missionCombat, empty subject, filter 0). It retains the latest 4096 combat events and explicitly reports truncation; personal buffers remain 256. Each UI page holds up to 60 events, fetched in existing six-event replies; Older/Newer browse without unbounded row creation or eager whole-log downloads. Combat events now snapshot actor/target faction keys, with player assignment fallback for older events. These are additive session-log fields, not a second telemetry collector. All testing, compilation, live UI inspection and screenshots remain deferred at the user’s request.


## Live validation — 2026-11-02

Testing resumed at the user's request. Fixed stale removed-search references in GC_DebriefReworkTests and guarded GC_TestAIReport before mission initialization (the old helper raised a null batch exception in PREGAME). Final Reload 34a0ca5b1a12476896ee607f0f2dccde compiled successfully, cycle 25736–25873 in logs_2026-11-01_22-52-25. No script compilation errors; three existing ArlandLobby GUID/name errors remain. Startup also logs existing world Parent/SlidingTrackMaterial, mouse-mask and missing loadout-manager errors; no debrief runtime errors in subsequent observations through line 26301.

Embedded Play on GameMode_Lobby2: production phase requests PREGAME → SLOTTING → BRIEFING → GAME, slot 1; native USSR AI firing reduced magazine 29 → 19 and produced ten AI rounds. Native damage exercised player→AI, AI→player and AI→AI death; production end transitioned to POSTGAME. All and USSR summaries showed Projectiles=10, Killed in action=2, Participants=1, AI kills=2, with ten rounds of 545x39 ammunition. Personal summary showed 0 shots/0 deaths and remained separate. Mission activity contained seven combat events versus five personal events, including faction-colored AI labels and timestamps.

Native widget hit resolution plus the production button handler passed sidebar cells, player-name cell, View activity and Older/Newer. Personal activity → Back → personal overview → Back → directory passed. This is native widget dispatch through the optional helper, not physical mouse or gamepad input. A reversible, non-persisted mission-buffer fixture passed 130-event paging: 60 / 60 / 10 rows; first timestamps 02:09 / 01:09 / 00:09; next cursors 71 / 11 / -1. Newer returned to the middle page (60 rows, 01:09, cursor11) with serverRequests unchanged at56. Fixture restored and client cache cleared afterward. 128-player contract: valid=1, eight pages, maxPayloadCharacters2112, build10ms/total14ms (single listen-host observation, not 128-client load). Personal event retention/filter contract passed 256 total,64 per direction/type filter.

Current actual MCP captures: overview-live.png, personal-activity-live.png, mission-activity-live.png. activity-paging-live.png is explicitly the synthetic 130-event presentation fixture. No production behavior changes were needed in this validation slice. Physical input/gamepad, other UI scales, dedicated-server callbacks and 128-client network load remain unverified. Workbench project/test addons remain loaded; Play is left in actual POSTGAME with synthetic event buffer restored.


## Activity death context — 2026-11-02

AI actor captions now use only the localized role when available. Damage event captions are Damaged <amount>; the redundant damage column is removed. The bounded eight-field report wire shape stays compatible, while UI renders five columns and gives Weapon / cause more width.

At death, the stats owner consults the existing shared combat buffer for the victim life's latest positive external hit, excluding bleeding, TRUE terminal notifications and self damage. It fills a missing death weapon only when that hit matches the credited source, also retaining the hit sequence. The canonical session death keeps a reference to that immutable damage event and serializes additive lastDamageSequence/actor/name/kind/prefab/weapon/projectile/type fields. No new collector, per-hit network message or kill-credit policy is added. Lookup is bounded by the existing4096-event history; when the source is unavailable the UI says Unknown source / Last hit unavailable. Bleed-outs, respawns and suicides display their cause plus the last external hit separately from kill credit. Weapon/projectile resource labels remain bounded; details cap240characters, existing six-event replies remain under the3800-character envelope.

Reload c68cb58cdf494806bea1b518488743fc passed script compilation (logs_2026-11-01_22-52-25 lines26358–26495); the same three existing Arland resource references remain. Native live player→AI and AI→AI kills now show Rifle AK74N 1P29 / Rifle AK74 rather than blank weapon. Native AI→player damage followed by stock bleeding produced a real bleeding death and displayed Bled to death · Last hit: Light AT Rifleman / Rifle AK74. The canonical player-stat death also retained that weapon and damageEventSequence9. GC_TestDeathContext passed source selection past synthetic bleed/TRUE/self events and presentation for bleeding/respawn/suicide; this fixture restores its temporary buffer and does not persist events. Personal filtering/retention contract still passes256total/64eachfilter. Native respawn/suicide actions were not rerun in this slice; their cause presentation was fixture-tested. Current screenshot: death-context-live.png (actual native events). Test addons/project remain loaded in POSTGAME. Physical input, other UI scales, dedicated/multi-client load remain outside this pass.

Final wording refinement: column heading is now Cause, per user request. death-context-live.png was captured immediately before this text-only heading change (shows Weapon / cause); event content/layout are the same. The new GUI unknown-name log was traced to inherited UI/layouts/Map/ToolMenu.layout while opening the spectator menu after death, not the debrief (lines26823–26828).


## Overview spacing and column alignment — 2026-11-02

Increased the gap from overview labels to stats/ammunition from0.032 to0.067 of the content height (roughly double). Detail lists retain their lower edge. Table headers now take their horizontal anchors from the rendered first data row after native layout, so the scroll-content gutter cannot shift column positions relative to the header. This runs once per table refresh; pending alignment is removed on close. Player directory shares the same correction.

Reload9d082dbe2637449e99dec3a406f94437 compiled scripts successfully; only the existing three Arland GUID/name errors remain in that reload. Live embedded POSTGAME: all five header-to-row text X deltas measured0px for a short actual combat log and again for60rows with the scrollbar visible (130-event reversible fixture). Pointer selection still worked. Fixture restored and client cache cleared afterward; left in actual All overview. Screenshots: overview-spacing-live.png (actual mission), activity-alignment-live.png (explicit synthetic pagination fixture, generic AI lacks a role prefab). Independent screenshot/source review requested. No other viewport scales or resize-while-open behavior measured in this slice.

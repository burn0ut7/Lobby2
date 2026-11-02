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

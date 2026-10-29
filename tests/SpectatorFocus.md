# Spectator focus and health

Click a player marker or a player/voice row to select them. The fixed bottom-left health panel appears and the existing event panel opens on Player. Row selection retains the existing one-shot camera jump; marker selection leaves the camera in place. Everyone and You remain available. Health bars (H, rebindable) independently enables overhead health for AI and players.

Character markers (role icon and name) project entity origin + 2.1 metres, not animated head bones. Overhead bars sit above the name/icon in screen space. Green >66%, amber >33%, orange <=33%; dead is a full red track with an empty health fill. The fixed panel includes the player's name, percentage and only applicable life state. No selected-health tool or context action.

The server reads native default-hit-zone health and controller life state. Each spectator receives their selected player's changed whole-percent health and bounded personal combat history. The selected history is fetched in 20-row pages, cached up to 256 rows; opening/closing the panel or reselecting the same target does not request it again. Live heads retain loaded older rows. Changing targets resets the focused history; Everyone/You caches remain independent.

Overhead health is entirely client-local: H only changes a local visibility flag. Active markers read native character health immediately on enable, then at most every 100 ms while visible. No world-health subscription, request, response, server cache or server sampler exists. Values reflect the native character state available on that client; characters outside local streaming range have no world marker. The separately selected 2D health/history feed remains authoritative and works independently of local entity availability. This supersedes the world-health networking described in the original validation notes below.

Reference: CoalitionArma/COALITION-Lobby revision aa502e34b8e743cd2a86e999b3dc56aa80b89660, Scripts/Game/Systems/UI/Spectator/Elements/COA_EntityInfoDisplay.c and UI/layouts/Menus/Spectator/Elements/EntityInfoDisplay.layout. Its native frame/track/fill technique informed ours; its extensive card and blood-percentage metric were not copied. Installed native PAK revision 3e84dd3e4386408097aaf18a7a8b6372: DamageManagerComponent.c 47–58 documents default-hit-zone health; HitZone.c 44–61 does not guarantee client synchronization, hence authoritative values for the selected 2D feed. The overhead display deliberately uses available local values as requested. Project marker resources retain existing native editable-character hierarchy.

## Validation (2026-10-29, Reforger 1.8.0.13)

Current Arland Lobby world and optional test addon; host plus two real standalone clients. Runtime fixtures invoke production handlers and native damage; these are not physical mouse tests.

- Reloads passed with no script errors. Existing ArlandLobby GUID and SCR_DedicatedServerPlugin resource diagnostics remain.
- A playing owner attempted the health subscription RPC: server focus/world caches stayed empty. An unsubscribed spectator received no health packets.
- Separate spectator received 100 -> 68 health; authoritative healing 50 -> 85 reflected in selected display. Native damage produced 5 damage, then 299 damage (killed), and Dead state on the remote viewer. Unconscious and automatic regeneration were also observed through native damage.
- A 25-hit native fixture loaded 20 selected rows with a next cursor, then 25 with next=0. AI/player overhead bars used actual replicated entity IDs and live server values; fixed/world bars followed the same colors.
- Further final interaction/capture evidence is recorded below.

- Final native player-row click handler returned handled=true after opening Factions. Closing the event panel and reselecting the same player left healthPackets=7 and eventPackets=2 unchanged; the same cached row returned. The visible Health bars option button toggled successfully.
- Actual MCP standalone 1920 captures: Art/References/spectator-focus/runtime-healthy-v1.png, runtime-injured-v1.png (38% amber), runtime-dead-v1.png (red corpse/world and fixed Dead panel). Workbench viewport also inspected at 2445x1386. These images are local reference artifacts, not packaged resources.
- Independent review found a >20-event live-head gap. The server now backfills new head pages to overlap the previous newest sequence, bounded by the 256-record ring, and sends one combined catch-up reply. Regression result follows.

Limitations: production callbacks were exercised through the optional test addon; physical mouse/gamepad input and other UI scales were not tested. No comparative frame-time benchmark was collected. Subscription/packet counters establish filtering and idle behavior, not bandwidth measurements. Selection outside replication range uses server-resolved health/history and no client entity dependency, but actual replica eviction was not separately forced in this pass. Mode isolation is confined to existing Lobby2-owned controller/UI classes; no shared stock class override was added.

## Final burst regression

Fresh reload d1e6ca37280f42c9bfc939a5b2e06eeb passed. In the new match, host selected player 2, received one native damage row (sequence10), rows=1/next=0/eventPackets=2. Then GC_TestFocusBurst(2,25) invoked 25 native hits in 179 ms (operation2d61f821bd854aa39b42913301e00c14). Next server sample produced rows=26/next=0/eventPackets=3; rendered snapshot contained every sequence10..35. Only one combined catch-up reply was needed, preserving the old row with no middle gap. A separate spectator peer also subscribed before the burst; its result is recorded with final review evidence.

Remote burst result: CLIENT-REPORTED rows=26,next=0,eventPackets=2 (initial + one catch-up). A focused event click moved the camera from <2694.46,139.881,4223.13> to <1199.58,46.8021,2706>. Independent reviewer approved source, fixed gap, actual healthy/injured/dead visuals and regression evidence with no remaining actionable findings.

After native death, host remained at healthPackets=13/eventPackets=4 across idle observations. Formerly playing player2 entered spectator and selected self: CLIENT-REPORTED health=300,rows=20,next=17,first=Killed Burn0ut7 > You, demonstrating pre-spectator history availability.

## Immediate local overhead bars (2026-10-29 follow-up)

Removed GC_SpectatorWorldHealthController and removed the overhead toggle from focus-settings RPC. Moved character marker projection to origin +2.1m; its health bar sits above both name and icon, following the same projection. Reused the existing layout/color/native health accessor. Reload fed93335769845e8a45f22dfc990d4b5 passed, no script errors; existing resource diagnostics unchanged. Initial enable uses the next normal marker render instead of waiting through two one-second network sampling intervals.

Verification: named H action exercised in embedded spectator; real peer2 used the same production toggle. Both reported world=1/focus=0/healthPackets=0/eventPackets=0/serverFocus=0/serverHealth=0. Remote captures runtime-local-injured-v2.png and runtime-local-dead-v2.png show local injury fill, dead red track and bar/name/icon stacking above living characters and corpse. No overhead RPC symbols/references remain. Independent source/visual review approved. No physical keyboard or frame-time benchmark claim; the existing native action fixture dispatches the named action.

## Click targets and authoritative jumps (2026-10-29)

Character markers now use their native ButtonWidget for clicks: the role image ignores the cursor and left selection runs once through OnClick. Right-click context behavior remains on mouse-down. Shared player-row decoration also ignores the cursor, making the entire row one native button across Players, Voice and Factions. Existing row handling focuses the player and requests the server position; the owner response moves the camera using the received vector without resolving a local target entity. No new network traffic or subscriptions were added.

Native ReportMouse probes over faction-row name, role icon and voice icon hit FactionPlayer_1 (ButtonWidget); all production handlers returned handled=true. Players and Voice name probes hit PlayerRow with handled=true. Each activation incremented the one-shot jump request once. Focus/history remained cached (healthPackets=1/eventPackets=1 across repeated selections).

Remote peer2 moved from <4194.46,140.315,5723.13> to <1198.02,41.2402,2733.24>, exactly matching the server resolver (receipts ra64eae1ca2aa4edd8448d75fa72bea83, ra793e56ea1554c049dec882268444130 and re0d3fe465d88436ca7b260033d9505b8). This proves a remote coordinate response and camera arrival; it does not prove that the target replica was evicted. The owner receive path has no target/entity lookup. Native pointer routing and direct production-handler execution are tested separately; physical mouse press/release remains unautomated.

Final reload 422a6fba0da143ffb33bae41e9a0aeda passed with no script errors; existing resource diagnostics unchanged. Target-specific marker probe hit LobbySpectatorMarkerButton, player=1, handled=true (ra0af173b2d894c419d197fcc220d3be2). Focus changed 0 -> 1, selected health became Dead and one personal death row appeared. Camera remained <1198.5,42.2229,2723.69>, jump request remained 0 (rbe6a39455811402ea4e99215a70aa9e8). Actual MCP capture: Art/References/spectator-focus/runtime-icon-focus-v3.png. AI marker probes correctly returned handled=false for player focus. The changes are confined to Lobby2 layouts/components; no shared stock override or server contract changed.

Fresh independent source/visual review approved with no actionable findings. Additional row capture: Art/References/spectator-focus/runtime-row-focus-v3.png; final workspace-root faction-name hit probe returned FactionPlayer_1/button=1/handled=true (rbe6f2b573a3648278ba029f63a53a6f8).

## Empty-world deselection (2026-10-29)

Left mouse-down in empty world space clears focus. The spectator-only GC_SpectatorWorldClick action uses mouse:button0/InputFilterDown, so closing a UI control on release cannot turn its click into a world click. Native widget hits, drawer/log bounds, the cursor-transparent selected-health display, open maps and other top menus preserve focus. Clearing hides selected health, removes Player and returns its active log to Everyone; You/Everyone tabs are otherwise unchanged. It sends the existing zero-focus unsubscribe once and cancels any outstanding player-jump response. Repeated empty clicks do not send requests.

Final reload 1e1786ee67b14573b77f6aa17313d4b8 passed without script errors (existing resource diagnostics remain). Optional GC_TestWorldFocusClick positions the native cursor and drives the named input action, with a separate release; it does not directly invoke the clear handler. Health hit=none and log hit=EventScroll both preserved focus=1; world hit=none cleared focus=0. Server focus/health cache became 0/0, camera unchanged (r86ec3af14f294eb9b89ad914d6eeb40b). Options and Voice buttons preserved focus=1 (r916b3b9db0034e529d6df1aeeba05332, r9f6edf0cd7284841bf9195a61836f37a). Marker hit preserved focus (r6f1e56a1948843c295e24d90b2b4cea7); selecting the player marker still returned handled=true. Actual MCP before/after captures: Art/References/spectator-focus/runtime-world-preserved-v4.png and runtime-world-cleared-v4.png. Cleared capture shows health and Player tab hidden, Everyone selected. Physical OS mouse press/release and gamepad input were not automated; the new action is mouse-only.

Open-map regression retained focus=1 (rde7dbc938b3240d9ae6d9a5a5b2075dc). Repeated empty clicks left focus/serverFocus/serverHealth at zero and health/event packet counts unchanged. Fresh independent source/visual review approved with no actionable findings.

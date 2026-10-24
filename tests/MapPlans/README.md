# Map plan validation

Workbench 1.8.0.13; 2026-10-24; embedded host on Arland Lobby. Core tests use this folder's addon.gproj. Drawing tests use ../MapDrawingPlans/addon.gproj, loading the optional companion and installed Map Drawing 1.1.1. Test addons are never production dependencies. All engine operations used Reforger MCP. Raw receipt-correlated requests/results and captures are retained under the MCP workspace `.local/map-plans/`.

Invoke `SCR_MapMarkerManagerComponent.GC_TestMapPlans(action)` on `GameMode_Lobby2` through generic runtime prepare/invoke. Actions include snapshot, add, save, load, clear, invalid, unauthorized, advance, ui, close-ui, ui-status and ui-PlanSave/PlanLoad/PlanClear/PlanCancel. UI actions call native button invokers and production request handlers; they do not prove physical input. Drawing tests invoke `TILW_DrawingSyncComponent.GC_TestDrawing(add/snapshot/start-local)`. Helpers do not grant permissions; tests use the embedded host's existing authenticated admin.

## Observations

| Behavior | Result / raw receipt suffix locator |
| --- | --- |
| Core and optional project compilation | ScriptsCompiled true; final core readiness 1792832404223110000-a24d8999396844a59ee1d85e1c4600a8 |
| Save / Clear / Load | One marker round-trip; UI actual local-file save 1792831295573160400-3048f46445604c1da6e79350d0b4a2cc |
| Confirmation / Cancel | Clear confirmation and cancel retained marker; confirmed Clear emptied, Load restored; 1792831334..1339 |
| Unauthorized / malformed | Requester -1 and invalid JSON rejected, unchanged state; 1792831096..1101 |
| GAME Load / Clear | Both rejected, marker retained; 1792831340..1342 |
| GAME Save overwrite | First click asks overwrite; second saved actual file, status confirmed; 1792832272033511100-6048a87c004d4197b0a152d673513b6b |
| Same mission recovery | Marker restored with final independent eligibility check; 1792832285073910100-b940bf9038f844dba3777e3a40b5eb8c |
| Expired recovery | Offline timestamp fixture now-7201, restart empty; 1792831421854404100-0ca8b6f6eb00462fb037aea8495fc67b |
| Clear survives restart | Empty; 1792831480105026700-d231f0f3a6694efd965820e36854e9af |
| Combined marker / drawing | Save, Clear, Load exact data; 1792831644346785800-2cff4951d8dd4775b439756b34f03165 |
| Actual process restart | Combined snapshot restored; 1792831832551169200-e2817cdb1d2947548656ef65c21754ba |
| Old manual file | Timestamp aged 10800 seconds; manual Load successful; 1792831833034802600-d443f0af39a6482286bb03c48f793c36 |
| Drawing phase gate | PREGAME local preview true; GAME server add unchanged and local preview false; 1792831906..1908 |
| Interrupted journal write | Offline truncated newest slot; previous complete snapshot restored; 1792831941287998600-c277edc072d74131babca5adc5250fb6 |
| Different newest mission | Older matching slot not selected; empty, 1792831947231025500-c1c7989b55d44f25a62239b684b1efbe |
| Actual A -> unrelated B -> A | A marker present before switch, B recorded independently, return A empty; 1792832318703367700-720020d9e99b48a4b3b4af992980512b |
| Unrelated mode after exit | Private/shared/server RPC/mission routes 0->1->2->3->4, owners 1/1/1/-1; 1792832301..2303. Matches addon-free baseline in ../MapMarkerPhases.md |
| Missing drawing support | Core-only restart reports recovery paused and preserves saved drawing plan; 1792832423232767300-c91b3da9e4c04500a5d23f2024964c48 |
| Repeated panel close/reopen | No SCRIPT errors in final optional session; log receipt 1792832318981395500-2a1bb40eeefe4da796ba5393382b0b70 |

The screenshot `1792831849154685600-194390034ac545b792e066ae913b1319.png` shows the initial panel, before the user requested removal of static explanations and labels. `1792832430899414400-32fd7e7fdd3948fbb2a63dd902779b47.png` shows preserved-recovery failure feedback. Source capture 2560x1440, exported 1920x1080. Compare against Art/References/map-plans/concept-v1.png and its brief. The user revision keeps Name inside the input, buttons, navigation and exit; explanatory text is removed. Action confirmation/results and actual recovery failures remain contextual.

Initial failures remain in raw evidence: an unsupported EditBox property prevented button creation; typographic punctuation truncated labels; teardown looked up destroyed widgets; GAME refresh discarded Save confirmation; recovery eligibility initially ignored intervening non-Lobby2 missions. Each was corrected and relevant behavior rerun. Existing engine/resource errors and the known reload teardown assertion are not claimed fixed.

Independent review requested corrections to GAME overwrite and last-mission eligibility; both were corrected and behavioral evidence returned. Final independent disposition: approved for delivery after corrected-source review, runtime evidence and the final simplified screenshot; documented validation limits remain.

## Limits

No dedicated-server/remote-client transfer, JIP delivery, cross-faction client rendering, maximum-size network payload, force-kill crash, physical mouse/keyboard/gamepad navigation or constrained viewport was validated. Restores use native marker and drawing replication/cache, but embedded-handler success is not proof of remote delivery. The truncated-write test is a file fixture, not a forced process crash. The global initialization hook intentionally records only Lobby2 recovery eligibility when another mode starts; no map/gameplay mutations or retained callbacks occur there. Native callback/RPC and serialized field names are compatibility exceptions to tagged naming; new entry points use GC names.

Missing-support file preservation was checked across the periodic-save interval: both SHA-256 values unchanged (`missing-support-verified.json`).

Final user-revised panel: `1792832777628558300-b50f6894c20541888e32ed24fbd11d0e.png` in MCP `.local/map-plans/`. Name placeholder and aligned buttons only, with blank initial status. Final compilation receipt `1792832762260070700-99e5c67d769f41988495c9d8a6e45abc`. Save overwrite, confirmation/cancel and reopen rerun after text reduction; no SCRIPT errors (`1792832714227398400-f7fa52b1e21343f794abaf0e0aae2e82`). Temporary button-anchor misalignment in earlier revision captures was corrected and recaptured.

# Validation

2026-10-25, embedded Workbench Play, optional McpTestAddon, Arland; authenticated local admin through normal lobby/slot/briefing/game handlers. Native script reload8dc37e1b9e404b7bad5c4883b2fdf6eb completed. Actual screenshots show row-local time fields beside Add time; AO rows have no time controls. Default5:00 is initialized only when creating a row; entered values remain through repeated poll results (11:00 screenshot after multiple updates).

Native Add time button accepted30,2:00,5:00,10:00,11:00 and added30,120,300,600,660 seconds respectively. Before/after server catalog deltas were requested amount minus0.70-0.78sec elapsed; other setup only counted down. Lower/upper bounds1sec and60:00 accepted. All outcomes logged through GC_AdminActionLogger with target and requested seconds; audit.json records them.

UI rejected empty, abc, -2,0,1:99,1:2,:30,2:,3601,999999999. Direct production requests rejected0,-1,3601; unauthenticated player-1 request rejected. Catalog before/after invalid batch decreased only by5.9sec elapsed. Ending Joint staging disabled its row action; direct extension after end rejected and remaining stayed0. Full receipt sequence retained.

Saved source follows Enforce conventions, existing method/serialized contracts preserved, git diff whitespace check passed. Existing world/resource/plugin/loadout diagnostics remain; no changed-code exception observed. No new shared-mode override or transport contract. Existing entity1-3600sec-per-addition limit retained.

Limitations: embedded host and current viewport only, native widget handler tests plus EditBox.SetText fixture, not hardware keyboard/focus, gamepad, dedicated/remote or constrained viewport tests. Reopening/switching tabs creates new rows and resets unsent input to5:00; ordinary status polling preserves it.
Fresh independent reviewer approved with no blocking findings; retain the input/remote/viewport limitations above.

# Kick / Ban validation

2026-10-23. Optional McpTestAddon, AdminMultiplayer world, embedded server plus a real diagnostic peer. Workbench operations and production button handlers invoked through Reforger MCP. Local screenshots and runtime receipts: `C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-moderation/`.

- Searchable session table retains disconnected players. Joining peer changed visible rows 1 to 2 automatically, preserving selected UID (receipt ra15b7c39b4d84fd69ca477da2d3ee78e).
- Production confirmation kicked peer 2. Resulting connected/controller/slot/voice all zero (r9e5b6c732f404efc80d3d05a0382f493). Automatic update retained disconnected row, reason and UID; Kick disabled (ra2b76f0dfb2d4c3094b859c3d46d0392). Central audit in logs_2026-10-23_15-36-25 line 665 includes actor, identity, target, reason, requestId and outcome. See automatic-disconnect.png.
- Native dropdown opens above the full-screen menu, with native option clicks verified for Permanent, Hours, Days and UTC date. Minutes, invalid date and parser overflow checked. See dropdown-fixed.png and date-review-final.png. Seven parser assertions pass (r24961145f4ba475ca479ab3133ff3146).
- Blank reason, past expiry, unknown requester rejected. Manual offline UID reaches dedicated-only boundary (rc5c0d87e00f94eff8f85e8d7917447ea).
- Logged-out connected host receives rejected-not-logged-in for both Kick and Ban without disconnecting (r02a2350dce5c43bc9dee3151efc22fe0). Menu teardown null-child defects were fixed in moderation and the concurrently added reservations component; repeated logout/stop/reload then completed without those exceptions.
- A synthetic session presentation record with a long note and three-second expiry verifies full note selection and local expiry updates (r49317679c0eb45ef86eb4144eafb43c3, long-note-expired.png). This fixture does not create a backend ban.

The row selection fills the existing editable reason and UID fields. No selected-detail panel, profile button or Refresh button. Connect/disconnect/moderation events refresh the roster; a local presentation timer updates expiry only. Pending requests and unknown outcomes are explicitly distinguished from success.

Boundaries: native persistent bans require a dedicated server and were not executed here. No global Steam-ID-to-Arma-UID lookup is available; known session Steam IDs are searchable, manual offline targets require Arma UID. The native Steam accessor is obsolete but verified in this installed build and used only for Steam accounts. Session history is not a replacement persistent ban database. Gamepad, long-session table volume and constrained viewport coverage remain unverified. Shared earlier admin changes remain uncommitted dependencies.

Sources: installed source mirror revision 9b2fea665c0943b9bcf87ec2891bc011, native BanServiceApi/PlayerManager/BackendApi and installed ServerAdminTools. Wiki research was unavailable because its index was not prepared. Native declarations establish signatures; remote kick and screenshots establish observed behavior.

Independent review approved the slice after fixes for long-note access, stale relative expiry, and result wording. Final whole-minute display verified by re6347032b3c149ed9040991afaa8d203 (`expiry=1 min`). Final reload completed and Play stopped cleanly. Original peer parameters and auto-reconnect restored.

## Immediate actions and row context, 2026-10-23

User explicitly removed confirmation. Kick/Ban now validate and submit on first click; pending disables the form/actions. Confirm/Back widgets and review state removed. Right-click Remove ban uses the clicked row's identity and the existing native RemoveBans path; server authorization, request correlation and central audit remain authoritative.

- Reload 45176131f3e94064aeab78e909329575 succeeded. One Kick click reached server host-kick rejection (r02c881d3aedf495fa01d11ddba177e8a); one Ban click reached dedicated-only boundary (r7e7995d019604ecab420240f52246241).
- Context Remove ban reached the same boundary (rf0f62afc84004012907e99ec5bdf47c1). After entering a different manual UID, right-clicking the row still submitted that row's UID (r090259b23ade42319ddc5bdaf3e055b0; logs_2026-10-23_15-51-18 line1609).
- Logged-out requester rejected for Kick/Ban/Copy/Unban; stayed connected and clipboard unchanged (rd34ba5bab2884a61a751ae20f6a98b51). Screenshot remove-ban-context.png.
- User confirmed external clipboard paste works. Scripted clipboard readback was empty; preserve that contrary result. A hot-edited layout initially referenced a not-yet-loaded row class, producing an Unknown class/null error; clean reload loaded both and clicking passed. The earlier engine resource-leak reload assertion remains an observed boundary, not a fixed engine defect.

Actual persistent ban/unban completion still requires dedicated-server validation. This follow-up tested immediate dispatch and authority through production handlers, not successful persistent mutation. Prior real remote kick evidence above remains applicable to the unchanged server kick handler.

Fresh independent review approved the immediate-action/context changes after correcting clipboard mismatch feedback to uncertainty wording. No remaining review findings in this scope. Test session stopped and production Lobby2 editor restored; shared uncommitted dependencies still prevent packaging this slice as a standalone valid commit.

## Kick / Ban filter polish — 2026-10-28

`GC_TestModerationFilters(field, query, choice)` exercises the production editable
selectors. Use `players` or `status`, free text with choice `-1`, or native option
indices (players: 0 clears; status: 0 any, 1 banned, 2 not banned). Both free-text
fields search all table columns; selected choices combine as exact filters.
`fixture` installs 18 presentation-only rows, including permanent and four-second
ban expiries. It creates no backend bans. Close/reopen the admin menu to discard it.

Verified embedded Workbench 1.8.0.13: typing and single-click player selection,
reason/UID/Steam searches in both filters, combined filters, empty results and
reset, expiry moving a row from Banned to Not banned, reopening, date entry and
invalid-UID feedback. The table spans .15–.70 instead of .13–.49; form fields are
grouped below it. MCP reload e19475b4084249948b7e4d4a485a301b passed with no script
errors; four pre-existing resource/plugin diagnostics remain. Current screenshots
and receipts are in the MCP workspace's local evidence folder
`admin-scope-20261024/moderation-polish-results.json`; independent visual review
approved. These filters cover the existing server-observed session roster, not a
complete backend ban inventory. Dedicated-client coverage was not performed.

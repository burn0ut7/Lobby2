# Admin missions

Admin Tools now has **Missions** and **End match** under Mission. Both require a current native admin login; the server checks again for every request and logs accepted/rejected actions through `GC_AdminActionLogger`.

## End match

Choose Mission complete, Faction victory, Faction defeat, Faction draw or Mission failed. Faction results require a current faction. Add an optional debrief message (512 characters maximum), then start/replace the countdown (1–3600 whole seconds), cancel it, or end immediately. These controls use `GC_BoundaryAdmin`, the existing pending-ending owner. Statistics continue during the countdown. The retained native result drives the notification, debrief heading and mission history. Revisions reject stale changes from another admin; cancellation invalidates the old revision. Authored end-mission actions share the native result builder.

## Mission browser

The server fetches the website catalogue through the private `$profile:Lobby2Voting.json` configuration already used by voting. Players only send owner RPCs. There are no client HTTP requests or embedded credentials.

Search matches names, tags and player counts. The table pages through all catalogue entries, including archived/unavailable entries, without the population filter used for voting. Select a row to see its thumbnail, full scrollable description, tags and declared required add-on IDs. Missing thumbnails and unknown player ranges remain explicit. Unavailable missions cannot be selected for loading; the website independently checks availability and scenario identity when accepting the request.

**Load mission** queues an idempotent request on the website. It does not restart or switch the running mission in this slice. A newer selection replaces the server's previous queued request, but not one already claimed by a future worker. The UI reports queuing explicitly. A repeated click for the same selected request uses the same request ID. After a connection error, the outcome may be unknown; reusing that request ID is safe.

The queue snapshots the exact scenario resource, declared `requiredAddonIds` and source metadata. Declared IDs are not a complete transitive dependency graph or proof of installed versions. The future PowerShell restart worker must resolve/validate the owning add-on and its dependency closure before execution. It must not substitute all currently loaded add-ons. This slice does not execute shell commands or resolve dependency versions.

## Verification

Workbench 1.9.0.37 embedded Play and an isolated six-mission website fixture exercised the server REST → owner RPC → browser path, full descriptions, thumbnails, numeric search and a persisted queued launch. The website catalogue integration suite covers auth, bounded full-text transport, idempotency, server scoping, replacement and unavailable-mission rejection. Restart execution and dedicated/separate-client behavior are outside this verification.

Native entry/button handlers selected US Army victory and scheduled 120 seconds (`c1e58b4e7b88405ab4a42477f80f0ae8`), cancelled while keeping statistics active (`d8558f53e690411eafdfd1d850dcce37`), rejected a stale revision (`901d0111a2464b47a07fee7b90c43dc3`), and entered POSTGAME with finalized statistics (`e322ce60b2d74047b5c761b7b3c6ef04`) and the retained message (`c14445af61a8428b959c9133cb116131`). After native host logout, launch transport rejected access (`568763ac271241d4b563e47f11647409`) and ending requests left the match unchanged (`c102d1fc50164a4e990598e2064e8dd8`); both generated central audit entries. Screenshots are `Art/References/admin-missions/browser.png` and `ending.png`.

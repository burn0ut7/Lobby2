# Game Master name badge

Follow-up: name labels use native Auto sizing so the star follows the text with
a six-unit gap instead of sitting at the far edge of a filled name cell. Stars
are now 22 × 22 (previously 18 × 18). Reload and live slot/voice-row inspection
confirmed the corrected position: [adjacent star](evidence/gm-badge-20261101/gm-adjacent.png).

Lobby player/voice rows and occupied slot rows show a small white star when the
connected player has the native `GAME_MASTER` role. Admin/new-player name colors
remain independent. Empty, disconnected and KIA slot presentations hide it.

The optional `GC_TestGameMasterBadge("check"|"clear"|"restore")` observes the
rendered badges. The explicit local-admin fixture clears/restores an existing
native GM role and lets production role events refresh the UI. It never sets
badge visibility. Allow the deferred role event to run before calling `check`.

## Verification — 2026-11-01

Reload compiled successfully. Live Arland checks confirmed visible GM badges,
removal with admin red retained, and restoration after the deferred role event.
Empty slots remained unbadged. Captures: [GM](evidence/gm-badge-20261101/gm-on.png),
[admin without GM](evidence/gm-badge-20261101/gm-off.png).

Native resource evidence: installed game `data/data010.pak`,
`UI/Textures/Icons/icons_wrapperUI-32.imageset` lines 346–350 (`favourite`).
Native GM ownership: `data/data007.pak`,
`scripts/Game/Editor/Entities/SCR_EditorManagerEntity.c` lines 583–589.
PAK catalogue revision: `721cd7b05eb54d3ca3591d9097af381d`.

The existing reliable role-change notification now carries native role flags
instead of only admin status, allowing the existing bounded convergence check
to wait for GM changes too. These pending values never authorize actions or
determine the badge; presentation re-queries the native player manager. Server
and clients must use the same addon build because that RPC argument changed.

Separate-client replication, disconnect/KIA transitions and other UI scales were
not exercised in this pass. Existing Arland resource, dedicated-plugin and
missing-loadout-manager diagnostics were also present; no badge script errors
occurred. Workbench was reopened with the optional test addon and the same Arland
world because the preceding production session did not load test methods.

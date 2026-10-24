# Voice and lottery icon validation — 2026-10-24

Native script reload succeeded through Reforger MCP. Tested in embedded Workbench
1.8.0.13, optional AdminMultiplayer.ent fixture. All status changes used the
production admin request/controller and existing deafen button handlers.

Observed states: neither flag, priority only, both flags, both plus personal
deafen, admin mute only, neither flag while deafened, and fully restored. The
widget snapshots in results.json report each indicator independently. Both Voice
and Players displayed the same flags; switching back after clearing them showed
neither badge. No new subscriptions, moderation transport or voice policy.

Original MCP screenshots show both indicators alongside personal deafen in each
list. The lottery capture shows gameMechanics and Lottery open, with voice badges
cleared. Capture source 3840x2160, resized by MCP to 1920x1080. The icons occupy the
existing row height without clipping or replacing the existing voice indicator.
The preceding slot-status/runtime-presence.png is the baseline composition.

Initial fixture placement in GC_AdminVoiceTests caused compile-order errors when
calling later modded helpers. Moving the observation method into GC_VoiceStatusTests
fixed compilation. A separate SteamAPI initialization failure persisted: the
installed app-ID file existed one directory above Workbench's launch directory.
The installed ID 1874910 was copied to Workbench/steam_appid.txt, matching Steam's
[documented direct-launch setup](https://partner.steamgames.com/doc/api/steam_api#SteamAPI_Init); subsequent startup and native reload passed.
No game executable, archive, Steam account or server launch code was changed.

Full raw calls/logs are local in reforger-script-tools/.temp/local/evidence/voice-icons.
This verifies embedded presentation and production request refresh. Remote clients,
long-name/constrained-viewport layouts, physical input and audible transmission
were not retested. Existing missing-map diagnostics in the fixture are unrelated.

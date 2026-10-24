# Native chat integration

Active reference: `native-chat-v1.png`, captured through Reforger MCP on 2026-10-23 in Lobby2's optional multiplayer test world, before this change. This reuses the game's existing chat design; no generated concept was needed.

Intent: use the normal chat action, input line, channel cycling, history, icons and message styling for Admin and Spectator. Remove the right-hand Admin Chat tab and panel. Keep the existing separate admin announcements.

Defining traits: compact native chat over the game view, standard channel badge/icon and input, native typography and history behavior; no new chat window or sidebar. Channel names distinguish Admin and Spectator. The baseline screenshot includes the old sidebar and editor chrome only as context: these are not defining traits. Any placement adjustment to prevent lobby-header overlap must retain the stock chat composition and be recorded here.

Source: live `SCR_ChatPanel`/ChatHud in installed Reforger. Reference capture: 2026-10-23T14:26:28.2991481Z, Workbench process 38656. Runtime paths will be recorded after implementation and rendered validation.

Intentional placement: Preview, Briefing and Slotting place the stock chat frame
80 layout units below the top edge, clearing the phase/navigation header. Spectator
keeps the stock top-left overlay. Native history fades and can temporarily overlay
menu content while chatting, as the existing stock chat does. The duplicate gameplay
HUD panel is hidden through the native panel manager while a lobby menu owns chat.

Implementation: `Prefabs/MP/GC_LobbyChat.et`, `scripts/Game/Chat/GC_LobbyChatChannels.c`,
`scripts/Game/UI/GC_LobbyChatPanel.c`, `scripts/Game/UI/GC_LobbyMenuChat.c`, the three
lobby layouts and spectator HUD. Actual MCP captures are retained in the MCP
workspace `.local/native-chat/chatPreviewFinal.png`, `chatSlottingFinal.png` and
`chatSpectatorFinal.png`; filenames describe captures, not acceptance verdicts.

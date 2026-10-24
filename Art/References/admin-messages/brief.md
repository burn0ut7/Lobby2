# Admin messages — concept v1
Built-in image generation created concept-v1.png before implementation. Prompt: full-screen Lobby2 admin workspace, existing dark/amber style, left Heal/Teleport/Send message, right audience choices Selected players/Everyone/Faction, searchable player selection, multiline 512-character message and Send message; separate recipient notification illustration. Original generated asset retained in the Codex generated_images directory.

Journey: Admin Tools or player context -> choose audience -> write message -> send -> server result. Context entry preselects only the clicked player. Faction choices come from current slot groups; recipients are connected players occupying those factions' slots, including dead slots. Everyone includes spectators. No change to native admin chat.

| Element | Purpose | Decision |
| --- | --- | --- |
| Tool navigation | Choose action | Extend existing navigation |
| Audience controls | Choose recipients | Three explicit choices |
| Searchable player picker | Select/browse players | Reuse existing component |
| Faction list | Choose current slotted faction | Show only in Faction scope |
| Message entry | Compose announcement | Multiline; up to 512 characters |
| Send and result | Submit once and observe server outcome | Reuse tool footer |
| Recipient popup | Read sender and announcement | Existing popup, sound once, fade at 14s, close at 15s |

Concept critique: preserves simple two-column workspace. Recipient inset is illustrative, not part of the composer. Production retains existing centered recipient popup and existing button/selection styling rather than adopting generated borders/shadows. No new UI framework. Baseline inspected: MCP .local/admin-implementation/admin-menu-two-player-heal.png. Verify actual composer, context preselection, faction scope and popup lifecycle through MCP; source checks cannot establish sound audibility or physical input.

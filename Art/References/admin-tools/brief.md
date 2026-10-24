# Admin Tools concept

Active visual reference: **concept-v1.png** (2026-10-22).
Use case: ui-mockup. This is design reference media, not a runtime texture.

## Source and tool

- Source screenshot: `source-lobby-v1.png`, copied from `C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792683423556661100.png`.
- Source depicts Lobby2 running in the Reforger Workbench viewport.
- Generated using the built-in `image_gen.imagegen` tool with the source screenshot as its image reference.
- Original generated output: `C:/Users/Gray/.codex/generated_images/01a16442-c215-7d63-8545-2b569a53e948/exec-ad72b65e-aaa2-48af-949a-9949d2b6a2f8.png`.
- Creative direction and initial visual inspection: requested Astra Extra High setup. Generation tool chooses its own media model; no media model override was exposed.
- Resulting reference asset: `Art/References/admin-tools/concept-v1.png`.
- Production messaging assets: `UI/layouts/Admin/Lobby_AdminChat.layout`, `UI/layouts/Admin/Lobby_AdminMessage.layout`, and the existing `UI/layouts/Players/Lobby_Player_List_Panel.layout`. The full tools overlay is still pending.
- Primary rendered messaging evidence: `C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792686893757233500.png` (admin chat) and `1792686901451614800.png` in the same directory (512-character notice). These are embedded-host renders, not independent acceptance or multiplayer proof.
- Initial tools implementation: `UI/layouts/Admin/Lobby_AdminTools.layout` and `Lobby_AdminOption.layout` in the same folder, with header access in `UI/layouts/Lobby_MenuBar.layout`. The current tools render is `C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792687944661539700.png`. Players/Kick/Ban and Create Slot are still missing; this render is not whole-concept acceptance.

## Defining visual acceptance traits

- Retain the existing subdued charcoal/green topographic lobby background, original phase header, factions and squad context.
- Flat, square, translucent dark panels; condensed white typography; thin gray separators; sparse muted amber/gold highlights and selected controls.
- One compact outlined Admin Tools overlay. In this image its bounds are approximately 47% of frame width and 60% of frame height, leaving the header and lobby margins visible.
- A narrow target list forms roughly one third of the overlay, with a search field, selected player indicated by muted amber fill and left amber rule, secondary faction/role text, and quiet online count.
- The remaining two thirds contain the selected-player heading followed by Players, Slots, Voice and Messages categories. Horizontal rules separate categories; controls form clear aligned grids.
- Players exposes Kick/Ban, with restrained red outline reserved for Ban. Slots exposes Assign/Free/Lock/Pin/Create/Respawn. Voice exposes distinct Priority speaker and Muted states.
- Messages has Admins/All/Selected recipient selectors, a multiline-capable input implementation may use the one-line appearance shown here, and an amber Send action.
- A visible Admin Chat tab uses the same selected-tab treatment in the lobby sidebar. A small player context menu offers priority/mute status and an Admin Tools shortcut.
- Keep dense utilitarian game layout and legible text, without decorative science-fiction elements, bright neon, oversized icons or a new visual theme.

## Intentional differences and limits

- The concept removes Workbench chrome and extends the game viewport to approximately 16:9 so the intended in-game composition is clear.
- Existing faction cards, topology and phase icons are generatively reconstructed; implementation should retain the project's actual resources instead of reproducing illustrative artifacts.
- Player names, player count, channel messages and selected states are illustrative state, not assertions about live runtime state.
- The context menu is shown open while the admin overlay is also open to document its visual vocabulary in one reference; actual interaction may present these separately.
- The generated concept shows the complete primary action set; assignment targets, ban duration/reason, creation details and confirmation flows are interaction details to be integrated using the same controls and spacing.
- Compare actual MCP-rendered output to this exact saved version. Initial concept inspection establishes only design direction; it is not independent acceptance of implemented runtime UI.
- Messaging implementation uses the existing sidebar width, with shortened Voice/Players labels to fit Admin Chat. The admin channel has a persistent input and newest-first history; the full tools overlay remains pending. Public/selected notices use a separate centered 720 by 360 logical-pixel dark panel with amber border, plain wrapped text, and Dismiss, applying the reference's visual vocabulary rather than adding a second tools menu. Initial notification sizing failed and was corrected before the successful render; retain both evidence sets.
- The initial tools panel is 980 by 800 logical pixels, with a 290-pixel target column and a scrollable action column. Extra slot variants, group/all locks, explicit recipient-list controls and an inline confirmation area require more vertical room than the illustration. It retains square charcoal panels, muted amber selection, condensed type and outlined controls. Final comparison must include the still-missing categories and representative screen sizes; the existing screenshot alone does not establish visual completion.

## Prompt

```text
Use case: ui-mockup.
Create one polished, realistic 16:9 game UI design concept for an ADMIN TOOLS overlay in an Arma Reforger mod called Lobby2. Use the attached actual Workbench screenshot as the visual reference, but present the clean game viewport at full-frame 1600x900, WITHOUT the Workbench editor chrome, toolbars, hierarchy, properties, or console. This is a design mockup in the existing game's visual direction, not a marketing illustration.

Preserve and extend the existing very dark desaturated charcoal/green topographic map backdrop. Preserve the existing phase navigation header across the top with the BANGALORIOUS title at left and PREVIEW, LOBBY (amber active), BRIEFING, GAME, DEBRIEF. Behind the new overlay show the existing lobby: small Soviet Army and US Army faction cards along the left, a few compact squad/role rows beside them, and the voice/player side panel on the right. Match the reference's compact condensed white sans-serif typography, flat translucent charcoal rectangles, hairline gray separators, square corners, muted olive-amber/gold selected fills and thin gold selection lines. No glow, neon, gradients, bevels, science-fiction decor, oversized icons or oversized text. Retain believable subdued game-UI hierarchy.

The admin overlay should be centered, occupying approximately 60 percent of screen width and 65 percent of screen height, with the phase header and recognizable lobby margins still visible. Single thin muted gold outline. Top title bar: small shield icon, 'ADMIN TOOLS', a tiny gold 'ADMIN' tag, and a simple X close button. Everything aligned to a practical rectangular grid that can be implemented in Enfusion layouts.

Overlay content: a narrow left column, about 27 percent width, separated by one vertical hairline. Heading 'TARGET PLAYER', compact search input labelled 'Search players…'. Four readable example list rows: 'Burn0ut7' with small 'ADMIN' status, 'Harris', 'Petrov', 'Miller'. 'Harris' selected with a flat muted amber row and thin amber left rule. Small secondary text under selected name: 'US Army • Rifleman'. At bottom of left pane, a quiet one-line '4 players online'.

Right main column: header 'Harris' plus quiet subline 'US Army • Rifleman'. Four compact categorized rows with label column and simple rectangular actionable controls:
'PLAYERS' — buttons 'Kick' and 'Ban'; keep destructive styling restrained, thin desaturated red outline on Ban, no alarming red fill.
'SLOTS' — six buttons arranged cleanly in two rows: 'Assign', 'Free', 'Lock', 'Pin', 'Create', 'Respawn'. Tiny status text 'Current slot: Rifleman'.
'VOICE' — two checkbox-like flat toggles labelled 'Priority speaker' and 'Muted', with Priority speaker ON in amber and Muted OFF gray.
'MESSAGES' — three adjacent recipient selectors 'Admins', 'All', 'Selected'; Admins is amber active. Below, a simple wide text input containing 'Briefing starts in two minutes.' and right-aligned small amber 'Send' button.
Use generous enough padding to read, but compact utilitarian game controls. Fit all categories fully in frame.

In the existing right sidebar, make a visible top tab row 'VOICE', 'PLAYERS', 'ADMIN CHAT', with ADMIN CHAT selected in the same amber style; beneath it show a tiny admin-only chat channel with two short lines 'Burn0ut7: Ready for briefing.' and 'Harris: Checking slots.' This sidebar is secondary and visually quieter than the admin overlay.

Also show one compact contextual shortcut menu, below the chat messages in the right sidebar, not overlapping the overlay: title 'Harris'; options 'Priority speaker  ✓', 'Muted  —', 'Open Admin Tools'. This is the alternative player-row shortcut, not a second full admin system.

Render all requested labels legibly and exactly. Do not add extra functions, promises, captions, explanations, watermarks, or decorative imagery. The result must feel like a natural addition to the attached 2010s military simulation lobby, preserving the existing charcoal/map background, sparse amber selection treatment, thin separators, compact condensed typography, and crisp functional layout.
```


## Creation form iteration

The active concept remains `concept-v1.png`. Create expands a coordinate form
inside the existing Slots section; the confirmation names the source slot and
world position. It uses the same charcoal, amber outline, condensed typography
and square buttons. The action column scrolls when the form is expanded. Primary
MCP render evidence: `C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792688984732498800.png`.
This is an implementation checkpoint, not final independent visual acceptance.

## Player tools iteration

The Players section now includes required reason, Kick, permanent Ban, and an
expandable ban-list area. Server status/history is an on-demand text report in the
same scrolling action column. Primary render checkpoint:
`C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/1792689765661056400.png`.
The active reference remains `concept-v1.png`. Populated ban rows have not yet been
rendered or accepted; whole-panel density and report expansion still need final
visual review with all remaining tools integrated.

## User direction: menu redesign deferred

On 2026-10-22 the user rejected the implemented admin-menu design and asked us
to move on, with a separate future feature slice to correct it. The current menu
is not visually accepted. Stop further menu design/polish in the current pass.
Preserve concept-v1.png and all rendered evidence for the future redesign; do not
change the reference or claim visual acceptance from the behavior tests.

Freeze controls were integrated into the existing Mission section before that
direction. MCP captures 1792694893937127800.png and 1792694940877119500.png under
C:/Users/Gray/Documents/VS/reforger-script-tools/.local/admin-implementation/
show the inactive and active states. These are evidence of the rejected current
implementation, not an approved replacement concept.

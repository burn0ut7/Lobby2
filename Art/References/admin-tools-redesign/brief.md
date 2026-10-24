# Admin Tools redesign reference

## Nearby-heal radius refinement — 2026-10-23

The user superseded the fixed 15m scope with an adjustable distance up to 1,000m.
Reuse the saved/opened `full-screen-v3.png` as the composition and style reference;
this is a small control addition, not a new menu redesign. Preserve the full-screen
surface, left rail, quiet typography and single amber action. Intentional departure:
the third scope is now `Nearby players`. In that scope only, replace the player
field with a `DISTANCE FROM YOU` slider, a live metre value and 1m/1,000m endpoints.
Default 15m, whole-metre steps. The summary, action and completed result identify
the chosen radius. Selected players and Everyone keep their existing behavior.
Runtime paths remain the layout/controller listed below, plus the heal server action
and controller request. New rendered evidence and review live in the sibling MCP
repository under `.local/admin-heal-radius/`. Older fixed-distance statements below
describe the preserved v3 reference and are superseded only by this refinement.

Active authoritative visual references (2026-10-23):

- **full-screen-v3.png** — full-screen Heal, three scopes, searchable multiselect roster.
- **teleport-v3.png** — full-screen Teleport, single From/To fields and searchable roster.
- Earlier **concept-v1.png**, **teleport-v1.png** and **search-v2.png** are preserved historical versions, superseded wherever v3 differs.

Use case: ui-mockup. Reference media only, outside runtime import folders.

Current implementation/evidence checkpoint (2026-10-23; not accepted): runtime
assets are `UI/layouts/Admin/Lobby_AdminTools.layout`, `Lobby_AdminToolButton.layout`,
`Lobby_AdminPlayerPicker.layout`, `Lobby_AdminPlayerRoster.layout` and
`Lobby_AdminPlayerRow.layout`. Controllers are `scripts/Game/UI/GC_AdminToolsMenu.c`,
`GC_AdminToolsComponent.c` and `GC_AdminPlayerPicker.c`. Actual MCP-rendered screens
are in the sibling MCP repository `C:/Users/Gray/Documents/VS/reforger-script-tools/`
under `.local/admin-implementation/admin-menu-final-heal.png`,
`admin-menu-final-teleport.png` and `admin-menu-final-result.png`; metadata and source
hash manifest are saved alongside them. The saved v3 concepts remain authoritative.
Native-widget adaptations use a flat charcoal surface, a labelled Close control,
a left-side native search icon and X-marked checkbox. The actual roster contains
the connected host rather than invented reference players. These differences are
recorded for independent comparison, not pre-approved as equivalent. Multiplayer,
larger roster/viewports, performance and final acceptance remain unverified.
The user's new request explicitly supersedes the earlier full-menu redesign deferral. The rejected `../admin-tools/concept-v1.png` remains historical; it supplies only the existing visual vocabulary, not the new layout or acceptance target.

## Authoritative v3 direction

**This section and the two v3 images override all earlier sections/images wherever they conflict.** V1 and search-v2 remain preserved as history. Their framed window, visible game phase chrome, single-player Heal scope, and dropdown-era descriptions are superseded.

### Defining traits and behavior

- The Admin Tools surface fills the **entire viewport edge to edge**. There is no outer dialog frame, exposed map margin, or underlying game/phase navigation anywhere around the edges.
- A simple dark left tools rail occupies approximately 21% of the saved frame. It contains ADMIN TOOLS, a small ADMIN badge and only Heal/Teleport. A full-height gray rule separates it from the slightly lighter charcoal task panel.
- The right side contains only the selected tool. Square controls, condensed off-white typography, quiet gray rules, restrained amber selection and one amber primary action preserve the established visual language.
- Heal scopes are exactly **Selected players**, **Everyone**, and **Within 15m of you**. The radius is 15 metres and the origin is the requesting admin, as stated in the label.
- Selected players supports multiple simultaneous selections. The field summarizes count and names; its open searchable roster has clear checkboxes. The example checks Harris and Harper and uses the dynamic action label **Heal 2 players**.
- Everyone and Within 15m of you must clearly identify their actual scope in the action/preview and result. They do not retain a misleading selected-player label. Their active-state renders are not shown in the saved image.
- Teleport fields are labelled **From** and **To**, selecting exactly one player each. The field names and a source → destination preview make direction explicit. The example uses Harris → Burn0ut7 and the action **Teleport Harris**.
- **Every player field opens a browsable full roster immediately with an empty search query.** Typing updates closest-name matches live. No typing is required to see players, and no Search submit step is introduced.
- All player fields use the same searchable roster vocabulary and magnifier affordance. Heal rows toggle multiple checked selections without closing; those selections persist across searches. Teleport shows one selected result for the active field, without a multiselect checkbox column, and choosing a player sets that one field.
- Opening, searching and choosing targets must not execute Heal or Teleport. The primary action remains separate and visible.
- The selected contextual player supplies the initial relevant target. Names, context, count, action labels, directional previews and result text must follow actual state; examples are illustrative.
- Inline result feedback remains a required behavior after execution. These two open-roster references show selection state, without an unearned success claim.
- Previously optional To me / Go to player form presets are omitted from these focused open-roster frames. If retained within the authorized scope, keep them subordinate presets and preserve the simple From/To hierarchy.
- Do not add future tools, disabled placeholders, a dashboard, health percentages, numeric player IDs, coordinates or mass teleport.

### Media provenance and exact files

All v3 media uses built-in `image_gen.imagegen`; the tool does not expose a media-model override. Saved versions were reopened and dimensions independently read: **1672 × 941** for both.

- Heal source inputs: saved/opened `concept-v1.png` and `search-v2.png` from this folder; the v3 prompt explicitly overrides their old frame and selection scope.
- Heal original: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-ce6e4e98-8b59-4fd4-ab2d-5d9ab18c2cc3.png`.
- Active Heal asset: `Art/References/admin-tools-redesign/full-screen-v3.png`.
- Teleport input: saved/opened `full-screen-v3.png`.
- Teleport original: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-f3eb288b-3fb5-42aa-aa1d-223a3775817e.png`.
- Active Teleport asset: `Art/References/admin-tools-redesign/teleport-v3.png`.
- Production asset paths and actual MCP-rendered evidence are owned by the implementation task and must be recorded when available. This reference task changes no source/runtime files and uses no editor/game controls.

### V3 review criteria and limits

Compare actual MCP-rendered output directly with **full-screen-v3.png** for Heal and **teleport-v3.png** for Teleport. Judge full-screen coverage with no exposed game chrome, simple tools rail, only selected-tool content, exact scope labels, readable field summaries, open roster geometry, checkbox versus single-selection semantics, restrained amber treatment and visible primary action. Check representative viewport sizes and long rosters/names.

Static media shows an empty-query roster and example selections. It does not verify live matching, ranking, persistence across queries, scrolling, keyboard interaction, server authorization, 15m targeting, execution success or result propagation. These need actual runtime evidence. Names and faction/role labels are illustrative; use real production data. Compilation and source inspection alone do not establish visual or interaction acceptance.

### Full-screen Heal v3 prompt

```text
Use case: ui-mockup.
Create the next authoritative Lobby2 Admin Tools design reference as a TRUE FULL-SCREEN UI, landscape 1672 by 941 pixels.
Input image 1 establishes the prior tool controls and charcoal/off-white/amber vocabulary. Input image 2 establishes the search-list styling. NEW REQUIREMENTS BELOW OVERRIDE both inputs wherever they conflict.

The Admin Tools UI must fill the ENTIRE image edge to edge. NO outer dialog frame, no floating window, no margins revealing a game map, no BANGALORIOUS title, NO game phase navigation, no underlying game chrome, no editor chrome. Every pixel belongs to this full-screen admin surface. A simple dark charcoal tools rail occupies the left approximately 20 percent of the screen, from top edge to bottom edge. A slightly lighter charcoal right task panel fills ALL remaining space. One fine gray vertical divider separates them. Square controls, condensed off-white typography, thin gray rules, quiet muted-amber selected/primary states. No glow, rounded cards, decorative HUD or background illustration.

LEFT TOOLS RAIL:
At upper left, compact heading "ADMIN TOOLS" and a small quiet "ADMIN" badge beneath or beside it.
Below with generous top spacing, exactly two simple tool rows: "Heal" and "Teleport". Heal is selected using muted amber fill and a thin amber left rule. Teleport is plain off-white.
No other tools, icons grid, player list, dashboard, settings or future placeholders in the rail.

RIGHT TASK PANEL:
At top-left with comfortable padding, heading "Heal" and short description "Restore health for selected players, everyone, or nearby players."
A simple close X lives at the absolute top-right of the fullscreen surface.
A quiet divider below the task heading.
Label "SCOPE". Three adjacent compact rectangular scope controls, all fully readable:
"Selected players" (amber active)
"Everyone" (outlined inactive)
"Within 15m of you" (outlined inactive)
These replace the older one-player/all-player choices.

Below with comfortable vertical spacing, label "SELECTED PLAYERS".
One broad selected-players field shows "2 players selected" and smaller secondary "Harris, Harper". A small magnifier at its right indicates searchable selection. NO downward dropdown arrow.
The field's searchable MULTISELECT roster is OPEN and anchored directly below it, with the same width, a square dark surface and fine gray border.
At the roster's top, a focused editable search field with magnifier and placeholder "Search players..." (empty query). Beneath it a small quiet "ROSTER" label.
Show a browsable list of existing players immediately, WITHOUT requiring typing first. Every row has a clear square checkbox at left, a player name, and smaller useful faction/role context. List these rows:
Burn0ut7 — US Army
Harris — US Army · Rifleman
Harper — US Army · Medic
Harlow — Soviet Army · Rifleman
Miller — US Army
Petrov — Soviet Army
Harris and Harper are checked with amber checkboxes and restrained amber row treatment. The other four are unchecked. Make the checkbox distinction unmistakable. Use fine row separators and a narrow scrollbar. Keep row text legible at normal game-UI scale.
The open list must feel like a searchable selection menu, not a permanent second dashboard. It updates matches live as text is typed; the empty search state shown here visibly offers the full roster. Checked selections remain selected across searches. Multiple picks do not close the menu automatically.

Keep the expanded list fully INSIDE the fullscreen viewport, with no clipping. Use a generous but bounded height and a vertical scrollbar. The form/list can occupy about 800 pixels of the task panel's width, leaving room to its right; do not make gigantic stretched controls.
At the bottom-right of the task panel, clearly visible outside the open list, place exactly one amber primary action "Heal 2 players". Beneath or near the main form outside the list, a quiet status line reads "2 players selected." No success claim is shown while editing.
Preserve comfortable hierarchy and spacing; use the whole screen naturally. No duplicate picker, no traditional dropdown, no numeric player IDs, no invented health percentages, no multi-tool controls, no future tools, no extra confirmations, no decorative imagery, no border enclosing a smaller floating window, no marketing captions, no watermarks.
```

### Full-screen Teleport v3 prompt

```text
Use case: ui-mockup.
Create the companion TELEPORT state of the supplied authoritative full-screen Lobby2 Admin Tools concept. Preserve the image's exact edge-to-edge framing, approximately 21%-width charcoal left rail, full-height divider, lighter charcoal task panel, top-right close X, title styling, condensed off-white type, fine gray rules and restrained amber accents. NO floating window, outer border or revealed game background; every pixel belongs to the fullscreen admin surface.

Left rail still contains only ADMIN TOOLS, small ADMIN tag, Heal and Teleport. Select Teleport with the same amber row/left rule; Heal becomes unselected.

Right panel:
Heading "Teleport".
Short description "Move one player to another player."
A quiet divider below the heading.
Then two plainly labeled searchable player fields in the same aligned form column:
"FROM"
Field: "Harris" with smaller "US Army · Rifleman" and a small magnifier at right.
"TO"
Field: "Burn0ut7 (You)" with smaller "US Army" and a small magnifier at right.
Use the exact labels FROM and TO. Each field selects ONE player only. No downward arrows and no multiselect checkboxes in this tool.

The TO field's player-search menu is OPEN, attached directly beneath it, matching its width. It contains a focused editable search input showing placeholder "Search players..." with a magnifier, then quiet ROSTER label and a browsable full roster immediately, with names:
Burn0ut7 — US Army
Harris — US Army · Rifleman
Harper — US Army · Medic
Harlow — Soviet Army · Rifleman
Miller — US Army
Petrov — Soviet Army
Burn0ut7 is the selected destination, marked with restrained amber row fill/left rule and small quiet SELECTED text at the right. Every other row is unselected. NO checkbox column, no multiple selections, no numeric scores.
Search filters/ranks names live while typing; the empty-query state shown here must visibly offer the full roster. A narrow scrollbar keeps the list bounded. Keep all rows and controls legible and inside the viewport; fit vertical spacing to the screen.

At bottom-right outside the menu, one amber primary action reads "Teleport Harris". At bottom-left of the task panel, outside the menu, a quiet directional preview reads "Harris → Burn0ut7". Do not show a success claim during selection. Do not add a separate search submit button, full-roster modal, competing primary action, coordinate/map inputs, mass teleport, future categories or more tools.
Any optional To me/Go to player preset shortcuts are outside this focused open-roster state; do not add them to this image. Preserve the new simple full-screen layout and restrained practical visual hierarchy exactly.
```

## Historical v1/v2 direction and provenance

The following sections preserve earlier concepts and prompts. They are not authoritative where the v3 direction above differs.


## Source and tool (v1 history)

Both frames use the built-in `image_gen.imagegen` tool. Its media model is not selectable through this tool.

- Heal inputs: `../admin-voice-channel/concept-v2.png` for briefing/context style and `../admin-tools/concept-v1.png` for charcoal/off-white/amber vocabulary.
- Heal original: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-773a97de-51f8-4ac8-a653-ff731324c5e4.png`.
- Teleport input: the saved `concept-v1.png` in this folder, opened before generation.
- Teleport original: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-97e301b5-c1bc-49af-8e2b-607b6b8a8f96.png`.
- Resulting reference assets: `Art/References/admin-tools-redesign/concept-v1.png` and `Art/References/admin-tools-redesign/teleport-v1.png`.
- Production asset paths: to be recorded by implementation owner. This concept task changes no runtime/source files or game/editor controls.

## Required design and defining traits (v1 history)

- A large, calm Admin Tools window fills most of the viewport while retaining the existing phase header and a narrow topographic-background margin. Square charcoal surfaces, condensed off-white typography, gray hairline dividers and restrained amber selection/primary-action styling match Lobby2.
- One simple left tool list and one broad right task panel. The left list contains only **Heal** and **Teleport** for this first slice. The selected tool has amber row fill and a thin left amber rule.
- The right panel shows only the selected tool's controls, with generous margins and straightforward form hierarchy. No mixed-category dashboard, permanent player table, future tools, disabled placeholders, bans or gear-spawn entries.
- The selected player from the contextual player workflow is the initial relevant target. Names displayed in selectors, action labels, previews and results come from real current selection; Harris is only illustrative.
- Each task has exactly one visually primary action and an inline outcome region. Pending, success and failure outcomes reuse that region; success text must follow an actual server result.
- Heal uses One player / All players scope choices. One-player mode shows a player selector and a player-named action. All-player mode must clearly identify its wider scope before the action; do not leave a misleading single-player target or label.
- Teleport has separate Moving player and Destination player selectors. A compact source-to-destination preview makes direction explicit. The action names the moving player.
- Optional **To me** and **Go to player** shortcuts are small outlined form presets in this concept. To me sets selected player → current admin; Go to player sets current admin → selected player. They do not create competing primary actions or silently teleport on selection.
- Close remains a simple top-right X. Avoid adding a decorative illustration, tutorial copy, complex dashboard or unrelated options to occupy unused space.

## Actual geometry and intentional differences (v1 history)

Both saved frames are 1672 by 941 pixels. The main window spans approximately x104–1575 and y120–853: about 88% of viewport width and 78% of height. The left navigation occupies about 26% of the window.

The generation prompt requested roughly 89% width, 86% height and 20% navigation; the generated result uses a slightly shorter window and wider navigation. The user's defining direction remains a large majority-of-screen window, a simple narrow tool list, and a spacious selected-tool panel. Implementation may use the requested larger height and narrower navigation where that improves viewing-size fit, while preserving the same composition and control hierarchy. Record any material deviation with the final rendered evidence; do not treat arbitrary shape changes as automatically accepted.

The fields occupy a readable column rather than the entire panel width. The primary action is right-aligned in the broad task panel, beyond the selector's right edge. Heal and Teleport share the frame, typography and main-action size; Teleport uses tighter vertical gaps to fit two selectors and presets.

## Limits and review handoff (v1 history)

- Background, player names, faction/role text and example results are illustrative; retain actual production resources and runtime data. These concepts do not establish live health, player eligibility, teleport success or authority.
- The Heal frame shows One player selected. All players is a clearly visible scope option, but that state's changed target/action copy requires actual rendered verification.
- These references cover the first two tools only. They do not authorize deleting or changing unrelated existing capability behavior.
- Actual open selector states, long names, no available player, unavailable destination, disconnected target, error feedback and small viewport behavior require implementation validation; their behavior is not proven by a static concept.
- Compare actual MCP renders directly with **both exact saved versions** at representative viewing sizes. Assess window composition, navigation-to-panel proportions, selected-tool isolation, field hierarchy, typography/spacing, amber use, dynamic names and outcome readability.
- Reference creation is not independent acceptance or runtime evidence. The primary task owns implementation, MCP evidence and final whole-task review.

## Player-search refinement (v2 history; superseded by v3 where conflicting)

The user requires **every player selection field** to open a searchable menu/list with live closest-name matches as the user types. **search-v2.png** supersedes the traditional dropdown appearance and behavior shown in the two earlier task frames. Preserve both earlier images as references for window/tool layout and outcomes; do not implement their downward chevrons or static player dropdowns.

- Active shared interaction reference: `Art/References/admin-tools-redesign/search-v2.png`.
- Built-in `image_gen.imagegen`, using the saved/opened `concept-v1.png` as the reference.
- Original output: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-9ff3c216-ddb0-444a-b863-de72b8ea6605.png`.
- The selected player remains visible in the collapsed field with secondary context and a magnifier affordance, without a dropdown arrow.
- Opening the field reveals an anchored square charcoal menu: focused editable search input, a quiet Closest matches label, and readable ordered name results. Matching updates on typing without a Search submission step.
- A result row shows the name and useful existing context; the currently selected player has a quiet SELECTED marker. Amber fill/left rule identify the active result.
- The concept query is **Har** followed by a caret. Example results are Harris, Harper and Harlow. Names/order are illustrative; actual matching must use real current names and rank the closest matches instead of merely showing a static alphabetical roster.
- Selecting a result updates that field and closes the search menu. Opening/searching does not itself heal or teleport.
- Apply this shared behavior to **Heal → Player**, **Teleport → Moving player**, **Teleport → Destination player**, and any other player-selection fields introduced in this scope. Do not leave one of them as a traditional dropdown.
- The search menu may overlay the task's feedback region while open. In the saved frame it extends slightly below the Admin Tools frame but remains fully within the viewport; keep actual lists visible and usable through appropriate bounded height/scrolling and screen-edge placement.
- This refinement adds no tools, categories, new gameplay actions, extra confirmation controls or permanent roster panel.

The saved static image establishes appearance and one open state. Live match updates, ranking, selection, no-match handling, long names and both Teleport field instances require actual behavioral/rendered validation. Compare against this exact v2 for player fields, plus the prior relevant task frame for the surrounding tool and outcome presentation.

### Player-search generation prompt

```text
Use case: ui-mockup.
Create the updated searchable-player-field state of the supplied saved Lobby2 Admin Tools redesign. Preserve its exact large window framing, topographic game background, phase header, square charcoal/off-white/amber style, left Heal/Teleport navigation, selected Heal tool, task heading, scope controls, and overall spacious hierarchy. This is a focused interaction refinement, not a new redesign.

Critical new requirement: ALL player-selection fields open a SEARCH MENU with a search box and live closest-name matches. They are not traditional dropdowns. The shown example is Heal's Player field; the same search-menu interaction applies to Moving player and Destination player in Teleport.

In this frame, Heal remains selected with One player scope. Keep the PLAYER field's current selection "Harris" with secondary "US Army · Rifleman". REMOVE the downward dropdown chevron. Replace it with a small ordinary magnifying-glass icon at the right to indicate player search.
Show the field's SEARCH MENU OPEN and attached directly below this field, with the same field width, square charcoal surface, thin gray border. Do not use a browser-style dropdown, giant modal or full-screen roster.
At the top of this expanded menu, place an actual editable search input with a small magnifying glass at left and typed text "Har" followed by a subtle caret. Directly below, a quiet small label "Closest matches". The search text is the focus.
Below are three readable name results in ranked order:
"Harris" with secondary "US Army · Rifleman" and a small quiet "SELECTED" marker at the far right.
"Harper" with secondary "US Army · Medic".
"Harlow" with secondary "Soviet Army · Rifleman".
The top Harris result is highlighted with subdued amber fill and a thin amber left edge. Other results are plain off-white on charcoal separated by fine rules. Do not display numeric relevance scores, online counts, permission badges or extra filters.
Results should be visually a short search menu anchored to the existing field, not a permanent player list in the sidebar. The current selected Harris is still apparent in the field above and the selected result below. The query is editable and the shown results illustrate live closest name matching as typing changes.
Position and size the menu so the large main window, action and all text remain on screen. The open menu may temporarily cover the inline result region, as a normal anchored overlay, but it must not obscure the player field, scope choices or primary action. Keep the right-aligned amber "Heal Harris" primary action visible. Do not show a success result while the selection menu overlays that region. No extra action button inside the search menu; clicking a matching player selects it.
No big instructional captions, arrows, empty-state placeholders, dropdown arrows, A–Z-only static roster, pagination, separate Search submit button, decorative illustration, new tools or categories. Retain the same condensed typography and quiet, functional military-game layout.
```


## Heal generation prompt (v1; player fields superseded by search-v2)

```text
Use case: ui-mockup.
Create one polished landscape 1672-by-941 game UI concept for a LARGE redesigned ADMIN TOOLS window in the existing Lobby2 Arma Reforger interface.
Input image 1 provides the current Lobby2 briefing/header/selected-player context. Input image 2 provides ONLY the established charcoal/off-white/amber material and typography vocabulary. The old multi-category dashboard in image 2 is rejected: do not preserve its layout, target list, button grids or multiple visible tools.

The new Admin Tools window occupies approximately 89 percent of viewport width and 86 percent of height, centered below the persistent slim game phase header. Only a narrow surrounding strip of the existing subdued topographic map and briefing context remains visible. The original full-width header remains recognizable: BANGALORIOUS, PREVIEW, LOBBY, BRIEFING (amber active), GAME, DEBRIEF. No Workbench chrome.
Window is a simple large flat charcoal rectangle with square corners and one thin muted-amber outline, no glow. Condensed off-white game typography, thin gray rules, subdued amber selected states. Clear spacious margins, calm utilitarian composition.

Window header: "ADMIN TOOLS" at left, small "ADMIN" tag, simple close X at far right. Thin divider below.
Below the header the window has exactly two columns: simple left navigation about 20 percent width, and a broad right task panel. One thin vertical rule separates them.
Left navigation contains ONLY two generously spaced selectable rows: "Heal" and "Teleport". "Heal" is selected with muted amber fill and thin amber left edge. "Teleport" is plain off-white. No search, players list, dashboard, counts, settings, future tools, disabled placeholders, ban or gear categories.

Right task panel shows ONLY Heal controls:
At upper left, restrained heading "Heal" with a quiet short sentence "Restore health for one player or everyone."
Below with comfortable vertical spacing, label "Scope", then two adjacent straightforward segmented choices "One player" (amber selected) and "All players" (unselected charcoal outlined). These are not large cards.
Below, label "Player", followed by a broad but not screen-wide selector field. It shows "Harris" in primary text and "US Army · Rifleman" in smaller secondary text, plus a small down chevron at far right. Harris is the player carried from the existing contextual selection. No avatar, portrait, health percentage, inferred medical state, giant player table, or extra options.
Arrange the form in a readable column approximately 650 pixels wide, aligned to the right panel's left padding. Do not stretch controls across the entire broad panel. Use deliberate open space to make this one task easy to understand.
Near the form's bottom, after a quiet horizontal separator, align one main amber button "Heal Harris" at the right edge of that form column. No competing action buttons and no extra confirmation modal.
Directly below the action area, a compact unobtrusive result line with a small simple checkmark reads "Harris healed." in off-white. This is an illustrative successful result state, not a toast overlapping other controls. Result area should remain visually subordinate to the task and reusable for pending/error feedback.

The entire panel must feel finished, spacious and functional. Preserve natural game-ui text sizes (not giant presentation type), balanced whitespace, square fields and outlined unselected controls. Do not add a decorative illustration to fill empty area. No dashboard, multi-tool sections, navigation breadcrumbs, instructional arrows, marketing captions, neon, rounded cards, gradients, bevels, heavy shadows or watermarks.
```

## Final label adjustment — 2026-10-23

The v3 composition remains active. The primary Teleport action now uses the fixed label `Teleport player`; the From/To fields and direction summary retain player names. This intentional departure from the concept's `Teleport Harris` avoids crowding the action with long player names. No layout or hierarchy change.

## Teleport generation prompt (v1; player fields superseded by search-v2)

```text
Use case: ui-mockup.
Create the TELEPORT state of the supplied saved Lobby2 Admin Tools redesign concept. This is a companion implementation reference. Keep the supplied image's framing, existing game phase header, topographic background, large charcoal outlined window, ADMIN TOOLS header, size, square corners, column divider, typography and restrained amber palette exactly consistent. Do not redesign the overall frame.

Make "Teleport" selected in the simple left navigation, using the same amber row fill/left rule formerly on Heal. "Heal" becomes an ordinary unselected row. Those are the only two tools.

Replace ONLY the right task content with a clear Teleport workflow:
Heading "Teleport".
Quiet explanatory sentence: "Move one player to another player."
A form aligned to the same left padding in the right panel, using the same spacious square selector fields:
Label "MOVING PLAYER".
Selector showing "Harris" and a small secondary "US Army · Rifleman", with a simple down chevron. Harris is the carried contextual player selection.
Then label "DESTINATION PLAYER".
Selector showing "Burn0ut7 (You)" and a small secondary "US Army", with a down chevron.
Under the fields, two small understated outlined shortcut controls on one row: "To me" and "Go to player". These are form presets, not additional primary actions. Keep them noticeably quieter and smaller than the main action. Their runtime meaning is to set Harris -> current admin for To me, or current admin -> Harris for Go to player.
Below a thin divider, a short clear preview reads "Harris → Burn0ut7". Put the one amber primary action "Teleport Harris" aligned right, matching the Heal frame's action position and button dimensions.
Below the action area, show a compact inline success line with a small simple checkmark and exact text "Harris teleported to Burn0ut7." Match the prior Heal result treatment.

Keep both selector fields and shortcuts fully legible, with comfortable vertical spacing and no overlap. This task form has more fields than Heal, so reduce unnecessary blank gaps while retaining the same frame and overall spaciousness. No map destination, coordinates, radius, vehicle options, complex dashboard, all-players teleport, multiple tool sections, future categories, disabled placeholders, decorative pictures, modal confirmation, or extra functions.
This is the same utilitarian military-simulation UI: condensed off-white text, charcoal flat surfaces, muted amber only for selection/primary action, fine gray separators, no rounded corners, neon, bevels, heavy shadows or watermarks.
```


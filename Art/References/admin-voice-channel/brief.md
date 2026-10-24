# Admin voice-channel move concept

Active picker reference: **concept-v1.png** (2026-10-23).
Active selection and destination-menu reference: **concept-v2.png** (2026-10-23).
Use case: ui-mockup. Reference media only, outside runtime import folders.

## Source and tool

- Built-in `image_gen.imagegen`; the tool does not expose a media-model override.
- Style inputs, both opened before generation:
  - `../admin-tools/source-lobby-v1.png`: actual Lobby2 Workbench viewport screenshot.
  - `../admin-tools/concept-v1.png`: square charcoal/off-white/amber visual vocabulary only. Its rejected whole-menu design is not reopened by this slice.
- Original generated output: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-93d861c9-999c-4e74-be20-e8040106e8fb.png`.
- Saved active asset: `Art/References/admin-voice-channel/concept-v1.png`.
- Production asset paths: to be recorded by implementation owner. This concept task changes no production files.

## Defining visual traits and scope

- Existing dark charcoal/green topographic background, condensed off-white type, original phase header, briefing/map context and right-side voice/player panel remain recognizable.
- A compact, single-column, square-corner charcoal picker uses a thin amber border. In the saved 1672-by-941 image it occupies approximately one third of frame width and three quarters of frame height, with the phase header and surrounding briefing visible.
- Title, small ADMIN badge and close button form a restrained header. Selected player and current channel appear above the search field.
- One scrollable destination list uses compact section labels: Global, Faction HQ, Groups, Personal Channels. Destination rows align consistently; the current channel is marked quietly. Selected destination uses amber fill and an amber left rule.
- Footer contains a small feedback region and right-aligned Cancel/Move controls. Move is amber, Cancel is outlined gray. Pending and final server-result text should reuse this feedback region without restructuring the dialog.
- The small player context-menu entry reads “Move to voice channel...”. The shared player/voice panel makes the same entry available in preview, lobby and briefing.
- The reference illustrates Harris moving from US Army HQ to Soviet Army HQ. All existing channel destinations are available to an authenticated admin regardless of target faction or whether a personal-channel owner is currently listening there.
- Moving voice membership preserves faction, squad and slot. The UI must not imply that those memberships are being changed.
- Retain utilitarian density, clear text and spacing; no rounded corners, decorative sci-fi chrome, neon, extra admin categories or redesign of the rejected full Admin Tools menu.

## Limits and intentional differences

- The generated briefing/map, content, player names and channels are illustrative. Keep existing real briefing resources and runtime data; do not implement the reconstructed background or invented mission text.
- The source is a lobby screenshot, so the generated briefing context conveys placement and visual vocabulary rather than proving the current briefing layout.
- The context menu and picker are shown together to document sequential interaction states. Runtime may close the context menu when opening the picker.
- The requested approximate 590-pixel picker height was expanded by generation to fit all groups visibly. Preserve the compact single-column hierarchy and on-screen fit, using scrolling for larger channel sets; the exact screenshot dimensions are not a pixel-perfect requirement.
- This image shows destination selection and the feedback area before submission. It is not a runtime result screenshot, proof of authorization, or proof of channel movement.
- No game/editor controls were used in this reference task. Parent task owns implementation, MCP render evidence and whole-task independent review.
- Compare final actual MCP-rendered UI directly with this exact saved version, including at representative viewing sizes. Reference inspection alone does not establish visual acceptance of implementation.

## Destination interaction refinement (v2)

Active selection and destination-menu reference: **concept-v2.png** (2026-10-23).
The existing **concept-v1.png remains active for the separate channel picker**. These two references cover different controls; v2 does not replace the picker design.

### Source and defining traits

- Built-in `image_gen.imagegen`, using `concept-v1.png` for briefing/presentation style and `../admin-tools/source-lobby-v1.png` for established slot rows. The media model is not selectable through the tool.
- Original generated output: `C:/Users/Gray/.codex/generated_images/01a16788-993c-7943-add8-3656815d3f1e/exec-f71a45f1-4a58-47f2-8295-62f28a106e95.png`.
- Saved reference: `Art/References/admin-voice-channel/concept-v2.png`, 1672 by 941 pixels.
- A selected player row remains visibly marked by restrained amber fill and a left amber rule; the concept also uses a compact SELECTED label.
- The destination owns the compact square context menu. A quiet destination title, thin separator and one actionable row provide clear hierarchy.
- Action names contain the currently selected player's actual name: **Move Harris here** on a channel; **Assign Harris here** on a slot. Replace Harris dynamically. Do not substitute a generic Move player/Assign player label.
- Selection persists visually while the destination menu is open. The main example selects Harris in US Army HQ and offers the Soviet Army HQ destination.
- A right-click directly on a player targets that clicked player for existing mute/priority and other player actions, regardless of the previously selected subject. It does not silently apply those actions to a different selected player.
- Existing charcoal/off-white/amber styling, condensed type, fine rules and square corners remain authoritative. No full Admin Tools redesign is introduced.

### Limits and comparisons

- The lower-left LOBBY · SLOT DESTINATION inset is a labeled second screen/state for design review; do not add that inset as a live briefing widget. Its Selected player: Harris caption is explanatory.
- The concept shows selected row and destination menus, not the separate picker. Use v1 for picker comparisons and v2 for selected-player markers, menu geometry and dynamic target labels.
- The generated map, briefing text, example faction image and role icons are illustrative; retain existing production assets and real runtime content.
- A channel-header context menu may appear near a player-row menu button in the composition. Its title and action explicitly target the channel, not Petrov. Final behavior must bind to the right-clicked destination itself.
- The inset additionally gives the destination slot a slim amber left marker. This is a contextual destination cue; do not confuse it with the selected-player subject or require a permanent selection on that slot.
- Reuse the existing menu system's real pointer placement and screen-edge clamping. Reference cropping of the far-right header globe is not an intended change.
- No production code, editor controls or runtime tests were changed by this concept task. Actual UI rendering and independent whole-task review remain required.

### V2 generation prompt

```text
Use case: ui-mockup.
Asset: Lobby2 Arma Reforger interaction concept for SELECT A PLAYER, THEN RIGHT-CLICK A DESTINATION.
Input image 1 is the saved voice-channel picker concept. Use only its exact established visual style and briefing context. Input image 2 is an actual Lobby2 screenshot establishing faction/slot rows. Do not show a voice picker or full Admin Tools menu. Create one crisp landscape 1672-by-941 design reference with a main briefing view and a modest clearly labeled inset illustrating the lobby-slot variation.

Maintain the charcoal/desaturated-green topographic map, condensed off-white text, slim full-width phase header reading BANGALORIOUS, PREVIEW, LOBBY, BRIEFING, GAME, DEBRIEF. BRIEFING is the active muted amber tab. Retain the left Mission Briefing text panel, quiet central map, and right-side VOICE / PLAYERS / ADMIN CHAT sidebar. Flat square rectangles, fine gray rules, muted amber selection fills and left rules; no round corners, glow, gradients, decorative HUD, giant typography or new design language.

MAIN BRIEFING INTERACTION:
Right sidebar should be comfortably wide enough for channel headings, names and the short contextual action, about 370 pixels. VOICE is active.
Show GLOBAL with Burn0ut7, US ARMY HQ with Harris, SOVIET ARMY HQ with Petrov, and MILLER'S CHANNEL as another available destination.
Harris is the SELECTED PLAYER. Mark ONLY the Harris player row using restrained amber fill and a thin amber left rule. A small unobtrusive label SELECTED at the row's far right reinforces the state; show normal off-white Harris text.
Right-clicking the SOVIET ARMY HQ channel heading opens a compact square charcoal context menu immediately to the left of that header, not a big dialog. The menu has a quiet title "Soviet Army HQ", thin separator, and one actionable row with exact dynamic text "Move Harris here". The action row has a faint amber hover fill. A small ordinary pointer may rest on that action.
The purpose is unmistakable: Harris remains selected in the US Army HQ list while the destination channel owns the context menu. Do not mark Petrov as selected. Do not show a generic "Move player", player search dialog, drag arrow, confirmation dialog or extra administrative controls. All existing channels are valid destinations, including opposite faction and personal channels.

SECONDARY LOBBY SLOT VARIATION:
At bottom-left of the main map, add a modest rectangular annotated detail inset, clearly separated from live UI and titled "LOBBY · SLOT DESTINATION". The inset represents another screen/state, not an actual briefing widget. Approximately 500 by 250 pixels, thin quiet gray border, dark charcoal background.
Within it show a compact lobby squad header "Kompas-43" with secondary "Soviet Army", then two existing-style slot rows with small role icons: "Senior Rifleman" and "Light AT Rifleman".
A compact destination context menu is attached to the "Senior Rifleman" slot row. Its title is "Senior Rifleman", its sole action is exact dynamic text "Assign Harris here". Use the same square charcoal menu, separator and restrained amber hovered action.
Above or below the inset content, outside its simulated rows, a tiny quiet reference caption "Selected player: Harris" makes the common selection state explicit. Do not add player selection controls inside this inset.
Keep the actual main briefing header, selected Harris row, destination channel menu and inset fully visible without overlap or clipping. Main view is dominant; inset is secondary. Use accurately legible exact action text.

Interpretation: selecting a player establishes the subject of destination actions; right-clicking a channel or slot targets that destination. A right-click directly on any player still targets that clicked player for the existing mute/priority actions, but do not draw that third menu in this image. No full admin menu redesign and no duplicate voice picker. Preserve concise practical military-simulation UI.
```


## Generation prompt (v1 picker)

```text
Use case: ui-mockup.
Asset: active implementation reference for a small admin "Move to voice channel" picker in the Lobby2 Arma Reforger mod.
Input image 1 is an actual running Lobby2 screenshot and establishes the existing game UI and materials. Input image 2 is a prior design concept used ONLY for the square charcoal/offwhite/amber visual vocabulary. DO NOT reproduce or redesign its large Admin Tools menu. Create one crisp 1600x900 landscape game UI concept, clean viewport without Workbench chrome.

Keep the existing dark charcoal/desaturated green topographic background and condensed off-white military-simulation typography. Preserve the slim full-width phase header with BANGALORIOUS at left and PREVIEW, LOBBY, BRIEFING, GAME, DEBRIEF. BRIEFING is active with a muted amber tab. Behind the overlay show a subdued briefing context: a narrow left panel with "MISSION BRIEFING", "Situation", "Mission", "Execution", a broad quiet topographic map with a few restrained map markers in the middle, and the existing narrow VOICE / PLAYERS / ADMIN CHAT sidebar on the right. VOICE is selected; the sidebar has readable compact channel headers and Harris as a player row. Keep background secondary.

Primary UI: one compact dialog, about 560 pixels wide and 590 pixels high, near center-right, leaving the header, briefing/map area, and right sidebar margins visible. Crisp square corners. Flat nearly opaque charcoal fill, single thin muted amber outline, no shadow tricks, no glow or decorative paneling. The top title bar reads "MOVE TO VOICE CHANNEL" with a small "ADMIN" tag and simple X close control.
Under the title show selected player "Harris" prominently but compactly, and secondary text "Current channel: US Army HQ". Thin gray divider. A full-width search field reads "Search channels..." with small magnifier icon.

A single vertically scrollable destination list below the search contains small quiet uppercase section headings and simple full-width rows:
GLOBAL
Global
FACTION HQ
US Army HQ     CURRENT
Soviet Army HQ
GROUPS
Kompas-43
Buran-44
PERSONAL CHANNELS
Burn0ut7's channel
Miller's channel

The "Soviet Army HQ" destination row is selected using a restrained muted amber fill and a thin amber left rule. All destinations are available; no locks, faction filtering, hidden personal channels, disabled rows, restriction text, or warnings. The current row's CURRENT label is small quiet secondary text. Section labels are smaller/desaturated than actionable row text, thin gray separators, uniform compact row heights, aligned baselines. Include a narrow functional scroll indicator inside the list at its right edge.
Below the list, a small single-line feedback area reads "Move Harris to Soviet Army HQ." A fine separator precedes footer buttons aligned right: outlined gray "Cancel" and compact muted amber "Move". Keep all dialog content fully legible and within the boundary.

To document the invocation vocabulary in the same concept, show a very small square charcoal context menu near the Harris player row on the far right, without obscuring the picker: "Harris", separator, "Move to voice channel..." highlighted faint amber. This context menu and dialog are illustrative successive interaction states; make the picker clearly the main focus. Do not add a full admin menu or extra actions.

Precise typography, balanced compact spacing, functional utilitarian layout, thin gray rules, off-white text, subdued amber only for selected or active controls. No rounded corners, neon, gradients, bevels, prominent illustrations, giant UI type, new faction art, watermarks, presentation captions, or extra functionality. The concept must look like a small natural extension of the supplied existing Lobby2 UI, in briefing context.
```


Implementation draft: destination categories currently appear as explicit row prefixes, rather than separate section headings. The picker uses the existing option-row resource. Native render comparison and representative viewport sizing remain pending; these differences are not yet visually accepted.

# Lobby2 Briefing Content and Action Design

## Status

Design and research document. This does not implement the briefing body or action dispatcher.

## Goal

Lobby2 briefings should support highly customizable, formatted mission information and interactive links. A briefing author should be able to select a supported action, pass parameters to it, and let Lobby2-owned script code perform navigation or dispatch further events.

The flexibility boundary is intentional:

- Briefing content may request a registered action and supply data.
- Briefing content must not select an arbitrary class, method, script file, or expression to execute.
- The registered handler remains responsible for validation, lifecycle, multiplayer authority, and the resulting behavior.

This produces nearly the same authoring flexibility as direct script invocation without turning mission content into executable code.

## Research findings

### Native rich-text support

Reforger's `RichTextWidget` is a multiline `TextWidget` that parses an HTML-like markup language. It is not a general HTML renderer or embedded browser.

The documented formatting includes:

- bold and italic text;
- foreground color by RGBA, hexadecimal value, or supported color name;
- SDF-font outline and text/image shadow;
- packaged font resources;
- left-, center-, and right-aligned paragraphs;
- headers with predefined or explicit scale;
- explicit line breaks;
- images from packaged image sets;
- Unicode codepoints;
- representations of configured input actions and individual keys.

Important constraints:

- Bold, italic, outline, and full Unicode behavior depend on SDF font support.
- A font named in markup must be a packaged font resource. Missing fonts fall back to the system font.
- Forcing a font disables language-specific substitution and can prevent localized glyphs from rendering. It should be exceptional rather than the default.
- Inline images come from packaged image sets. The rich-text format does not load remote images.
- Markup tags must be properly nested and must not overlap.
- Long, segmented, wrapped text can be expensive to refit. Runtime resizing and frequent text replacement increase that cost.

The base game also contains `SCR_RichTextTags.TagColor`, which emits the documented `<color rgba="...">...</color>` form. This confirms that the markup is used by game script, not only by the layout editor.

### Plain-text support

Plain text is a first-class briefing format. Authors must not be required to write rich-text markup for ordinary paragraphs, titles, labels, or action captions.

Lobby2 should use one document system and one text-block model rather than separate plain-document and rich-document systems. Each text block contains one `content` value and may optionally declare its format:

```text
- type: text
  content: Enemy forces occupy the radio station.

- type: text
  format: rich
  content: <b>Warning:</b> Enemy forces occupy the radio station.
```

If `format` is omitted, the block is plain text. Rich text is opt-in with `format: rich`. This default keeps simple authoring simple and prevents ordinary text containing markup-like characters from unexpectedly changing presentation.

This is still one rendering pipeline. The renderer may use a normal text widget for plain blocks and a rich-text widget for rich blocks, or safely encode plain content before passing it to a rich-text widget. That is an internal rendering choice, not a second content system.

Do not infer rich text merely because the content contains angle brackets or resembles a supported tag. Automatic tag detection makes typos, technical notation, localization, and runtime-supplied strings ambiguous. The declared format determines whether markup is interpreted.

### Rich-text actions are display-only

The rich-text `<action>` element inserts the text or icon for a configured input binding. The `<key>` element similarly inserts a key representation. Neither element is an interactive hyperlink and neither invokes script.

The documented rich-text format does not provide:

- an HTML-style `<a href>` element;
- an `onclick` attribute;
- a callback associated with an individual text span;
- JavaScript, CSS, or arbitrary HTML;
- embedded Enforce Script expressions;
- Markdown parsing.

`RichTextWidget` exposes content sizing, line width, line visibility, and elision behavior in addition to inherited text behavior. It does not expose a documented character-range hit-test or clicked-span callback. Deriving a clicked word from cursor coordinates would therefore be fragile and inaccessible to controller users.

### External browser links

The engine declaration `PlatformService.OpenBrowser(string url)` is available. The base game uses it from `SCR_BrowserLinkComponent`, which can resolve a backend link item or open a configured URL from a button component.

External links are therefore possible through explicit interactive widgets, subject to platform availability and policy. They are not native rich-text hyperlinks.

### Current Lobby2 briefing surface

Lobby2 currently authors four briefing-title rows in `UI/layouts/Briefing/Lobby_Briefing_Titles.layout`. `GC_BriefingTitlesComponent` registers those rows by fixed widget name, and `GC_BriefingTitleRowComponent` owns their selection and visual state.

That is sufficient for a first briefing-body slice. A later customizable-section slice should replace the fixed registration list with data-driven row creation rather than expanding the hard-coded pattern.

## Decision: formatted documents plus registered actions

The briefing is modeled as a document made of presentation blocks and action blocks.

Suggested block categories:

| Block | Purpose | Suggested rendering surface |
| --- | --- | --- |
| Text | Plain paragraphs by default; optionally rich headings, warnings, inline icons, and formatted values | Text or rich-text widget selected by the renderer |
| Image | Maps, diagrams, photographs, larger illustrations | Image widget in a bounded block |
| Divider or spacer | Document structure | Layout widget |
| Action link | Link-like interactive command | Button widget styled as text |
| Action button | Prominent interactive command | Standard button widget |

A simple conceptual document could contain:

```text
section: situation
title: SITUATION
blocks:
  - type: text
    content: Enemy radio traffic originates from Hill 62.
  - type: text
    format: rich
    content: <color name="orange"><b>Warning:</b></color> Expect resistance.
  - type: actionLink
    label: VIEW HILL 62 ON MAP
    action: focus_map_target
    parameters:
      targetId: hill_62
```

This is a data-model example, not a committed Reforger config syntax.

## Action dispatch model

### Request shape

An authored action request should contain:

- a stable action identifier;
- a collection of serializable parameters;
- optional presentation metadata such as label, icon, tooltip, and confirmation text.

At runtime, Lobby2 adds context that content is not allowed to forge, such as:

- the source briefing and section;
- the local player/controller identity;
- the active Lobby2 phase;
- the source widget;
- whether execution is occurring on a client or authority.

### Registered handlers

The dispatcher resolves the stable action identifier to a Lobby2-owned registered handler. A handler may:

- change local briefing selection;
- open or focus another Lobby2 panel;
- focus a known map target;
- request a permitted server-authoritative operation;
- invoke a Lobby2-owned event or `ScriptInvoker` for decoupled listeners;
- return a handled, denied, unavailable, or invalid result.

Handlers should register when the owning Lobby2 UI or service becomes active and unregister during teardown. The dispatcher and its subscriptions must not remain active when Lobby2 is closed, unloaded, or not the selected game mode.

### Parameters

Parameters provide flexibility, but each handler must define and validate its own schema. Validation should cover:

- required and optional keys;
- expected value types;
- ranges and maximum lengths;
- supported identifiers;
- whether a target currently exists;
- whether the action is legal in the active phase;
- whether the caller has authority or permission.

Unknown parameters should normally be rejected or ignored according to the handler's declared policy. Missing required parameters must fail without partially executing the action.

For common actions, prefer stable identifiers over raw state:

- use `targetId: hill_62` instead of trusting arbitrary world coordinates;
- use `sectionId: objectives` instead of a widget name;
- use `roleId` or another authoritative reference instead of copying faction or group ownership into briefing data.

### Why arbitrary script invocation is excluded

Briefing data must not contain values such as:

```text
class: SomeClass
method: RunAnything
script: scripts/Game/SomeFile.c
expression: GetGame().Something()
```

Direct invocation would make content tightly coupled to implementation names and would bypass validation, phase rules, authority checks, and lifecycle cleanup. It would also make malformed or server-supplied content capable of reaching unrelated game systems.

A registered action can still perform any behavior intentionally exposed by Lobby2. Adding new capabilities means adding a new handler or extending a handler's validated schema, not widening the interpreter.

## Interaction and presentation rules

### Link-like actions

Use real buttons styled as links. A link style should include:

- a distinct normal color;
- hover, focus, pressed, and disabled states;
- controller focus and activation;
- an optional icon indicating navigation, map focus, or external navigation;
- a tooltip or accessible label when the visible label is ambiguous.

For the first implementation, place action links as their own document rows beneath the related paragraph. True word-wrapped inline links would require a custom flow renderer made from multiple widgets and are not justified for the first slice.

Do not overlay invisible buttons on rich text. Such overlays become incorrect when text wraps, font scale changes, resolution changes, or localization changes.

### External URLs

External URL actions should:

- be explicit rather than disguised as ordinary text;
- accept only approved schemes, normally `https`;
- preferably resolve a known link identifier or validate against an allowlist;
- tell the player that an external browser will open;
- fail gracefully when the platform service is unavailable;
- never accept an unvalidated URL from replicated or server-supplied briefing data.

### Localization and fonts

- Author player-facing strings so they can move to localization resources.
- Keep markup structure separate from translated text when practical.
- Test text expansion, right-to-left presentation where applicable, and non-Latin glyph coverage.
- Do not force a custom font globally unless it contains every required glyph and the localization tradeoff is intentional.
- Package only fonts with suitable redistribution rights.

### Performance

- Put the document in a stable-width scrolling surface.
- Replace content when the selected section changes, not every frame.
- Avoid repeatedly resizing a long wrapped rich-text widget.
- Bound total text length, block count, tag count, and image count for runtime-supplied content.
- Prefer separate image blocks for large images rather than heavily segmenting one rich-text string.
- Cache parsed or resolved document data when profiling shows a need; do not introduce speculative caching in the first slice.

## Multiplayer and authority

Briefing rendering and navigation are normally client presentation. A client may freely change its selected section or local map view.

An action that changes authoritative game state must be treated as a request, not as permission. The authoritative system must independently validate:

- the requesting player;
- the active Lobby2 phase;
- the referenced base-game player, group, faction, character, or mission object;
- permissions and current state;
- replay or repeated-click behavior.

Briefing data must not become a competing source of faction membership, group membership, player ownership, or character affiliation. It should reference stable identifiers and resolve current relationships through the authoritative base-game systems.

Purely local actions should not be replicated. When all clients can derive a document from the same resource and authoritative mission state, replicate the compact state or identifiers rather than the fully rendered markup.

## Supported and unsupported capability policy

### Supported

- Plain text as the default text-block format.
- Documented rich-text markup.
- Packaged fonts and image-set images.
- Data-driven briefing sections and blocks.
- Link-styled and conventional buttons.
- Registered action IDs with validated parameters.
- Handlers that dispatch further Lobby2-owned events.
- Local navigation and presentation actions.
- Explicit, validated requests to authoritative systems.
- Approved external browser links through an explicit action.

### Not supported

- Arbitrary class or method names in briefing content.
- Script paths or executable expressions in briefing content.
- Reflection-like dispatch based on authored strings.
- Native clickable spans inside `RichTextWidget` unless a future verified engine API provides them.
- Mouse-coordinate guessing over formatted text.
- Invisible button overlays on words.
- JavaScript, CSS, general HTML, or embedded webpages.
- Remote images or arbitrary runtime resource paths.
- Unvalidated external URLs.
- Briefing-owned copies of authoritative faction, group, or player relationships.

## Suggested implementation route

Each step is intended to be a small, testable slice.

### Slice 1: static briefing body

- Add a stable-width scrolling briefing body.
- Render one plain-text paragraph and one authored rich-text sample with a heading, emphasis, color, and packaged image.
- Switch the body when an existing briefing-title row is selected.
- Validate layout, wrapping, scaling, and controller scrolling in Workbench.

### Slice 2: document data model

- Define add-on-owned section and block data.
- Render text blocks in their declared plain or rich format, plus image, spacer/divider, and action blocks.
- Preserve the existing four titles while moving their body content into the model.
- Add validation and graceful placeholders for malformed blocks.

### Slice 3: action dispatcher

- Define stable action identifiers and the request/context/result model.
- Add lifecycle-scoped handler registration and cleanup.
- Implement one local proof action, such as selecting another briefing section.
- Verify mouse, keyboard, and gamepad activation.

### Slice 4: parameterized map focus

- Add a map-target action with a stable target identifier.
- Resolve that identifier through the owning mission/map system.
- Open or select the map presentation and focus the target using verified engine APIs.
- Handle missing targets and unavailable map state without leaving the briefing inconsistent.

The exact map-opening and focusing APIs must be researched and verified against current Game Data before this slice emits Reforger-specific code.

### Slice 5: authoritative actions

- Add only an action that has a concrete gameplay requirement.
- Write the multiplayer data-flow ledger before implementation.
- Validate dedicated server, listen server, owning client, non-owning client, and join-in-progress behavior where applicable.
- Ensure the handler cannot affect unrelated game modes.

### Slice 6: external links, if required

- Add an explicit external-link action.
- Apply scheme and destination validation.
- Test platform availability and failure behavior.

## Validation checklist

For each implemented slice:

- inspect the layout structure and script bindings;
- run native script validation and exhaust all diagnostics;
- reload Workbench only after validation succeeds;
- exercise the affected briefing interaction live;
- inspect fresh logs;
- test keyboard/mouse and controller navigation;
- test plain text containing markup-like characters, long text, localized expansion, missing images/fonts, malformed parameters, and unavailable targets as relevant;
- confirm all subscriptions and action handlers are removed when the menu or Lobby2 lifecycle ends;
- confirm unrelated game modes retain their original behavior.

## Evidence

Research was performed against the packaged Official Wiki corpus revision `ow1:d40df5e4830cab07dbc5a9c1c06beadd521c4214315d43176954e54c27f682e7` and indexed Game Data catalogue revision `gd2:8a9858b5db837f96517e7e7ffb9973707122e5c0b14a3234934b6b6dde9cdc85`.

| Claim | Evidence |
| --- | --- |
| Rich text is HTML-like and supports the listed formatting, images, action/key representations, nesting rules, and performance constraints | Official Wiki, [RichText Format](https://community.bistudio.com/wiki/Arma_Reforger:RichText_Format), packaged path `Modding/Guidelines/RichText Format.md`, lines 1-212 |
| Layouts are composed of widgets, and buttons provide menu interaction | Official Wiki, [Layout Creation](https://community.bistudio.com/wiki/Arma_Reforger:Layout_Creation), packaged path `Modding/Tutorials/Layout Creation.md`, lines 3-200 |
| `RichTextWidget` derives from `TextWidget` and exposes content/line/elision operations but no span-click callback | Enfusion Core Game Data, `scripts/Core/generated/UI/RichTextWidget.c`, lines 12-39 |
| Base-game color helper emits documented color markup | Arma Reforger Game Data, `scripts/Game/Global/SCR_RichTextTags.c`, lines 5-23 |
| External browser opening exists and the base-game browser-link component calls it from a widget click | Enfusion Core `PlatformService.OpenBrowser(string url)`, `scripts/GameLib/platform/PlatformService.c`, line 34; Arma Reforger `scripts/Game/UI/Components/SCR_BrowserLinkComponent.c`, lines 1-63 |
| Lobby2 currently uses four fixed briefing title rows | `UI/layouts/Briefing/Lobby_Briefing_Titles.layout`, `scripts/Game/UI/GC_BriefingTitlesComponent.c`, and `scripts/Game/UI/GC_BriefingTitleRowComponent.c` |

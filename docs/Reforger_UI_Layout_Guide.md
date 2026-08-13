# Lobby2 UI Layout Guide

## Purpose

This is the working reference for authoring Lobby2 UI in Arma Reforger. It compares Lobby2 with the existing `ReforgerLobbyCommunity/PlayableSelector` add-on and records the layout hierarchy, sizing rules, dynamic-widget pattern, and keywords that are safe to use.

The goal is to keep Lobby2's visual language and resource naming while adopting the other add-on's more reliable layout structure.

## Short answer

The other add-on solves the smushed-slot problem by separating three responsibilities:

1. The screen owns the viewport and scroll area.
2. The group owns its fixed header/card and exposes a list as an insertion point.
3. The slot owns its own fixed row rectangle and all of its visual content.

The parent `VerticalLayoutWidget` is not responsible for inventing the height of a slot. It only stacks already-sized children.

For Lobby2, the recommended model is:

```text
Lobby screen / future scroll list
└─ Squad group
   ├─ Squad header: fixed card/header height
   └─ Slots: VerticalLayoutWidget, insertion target only
      ├─ Lobby_Slot: fixed row height, root button
      ├─ Lobby_Slot: fixed row height, root button
      └─ Lobby_Slot: fixed row height, root button
```

## What the other add-on does differently

| Concern | `ReforgerLobbyCommunity/PlayableSelector` | Lobby2 recommendation |
|---|---|---|
| Repeatable item root | `ButtonWidgetClass` is the root of `CharacterSelector.layout` and `PlayerSelector.layout`. | Make `Lobby_Slot.layout` a button-rooted item when the interaction slice is implemented. Keep the current role/status visual design inside it. |
| Item sizing | Character rows use an explicit 608 x 40 rectangle. Player rows use the same fixed-row approach. | Put the row-height contract on the slot layout itself. Do not put the slot's minimum height on the squad list. |
| Group sizing | `RolesGroup.layout` uses an explicit 84px header/card area, then a separate list below it. | Keep the tactical squad header separate from `Slots`; let the slots determine the group's content height. |
| Parent list | `CharactersList` and `VehiclesList` are empty layout insertion targets. | `Slots` should be an insertion target, not a sizing wrapper with forced child height. |
| Dynamic creation | Scripts call `CreateWidgets(layout, parent)` and initialize the returned handler. | Future Lobby2 scripts should create slots into `Slots`, then call one initialization method on each slot. |
| Visual hierarchy | Button → overlay/padding → frame → direct visual children. | Use the same shell while preserving Lobby2's horizontal role/status grouping. |
| Flow vs. absolute positioning | Lists flow items; item internals use explicit local coordinates. | Use flow layouts only between repeatable items. Use explicit local dimensions inside a slot. |
| Content sizing | `SizeToContent` is used locally for small content such as a counter overlay, not as the main row-height system. | Do not use root `SizeToContent` as a substitute for a minimum row height. |
| Interaction | The item root owns its button component and state visuals. | Keep slot interaction/state on the slot's root button component; do not add a separate size or data script just to solve layout. |

## Reference hierarchy from the other add-on

### Screen and scroll regions

`CoopLobby.layout` uses a fixed menu viewport, then separates the body into panels. The faction and role areas each have their own scroll widget and vertical list:

```text
PlayMenu
└─ Lobby overlay
   └─ VerticalLayout1
      └─ Body
         └─ HorizontalLayout body
            ├─ Faction panel
            │  └─ ScrollLayout FactionScroll
            │     └─ VerticalLayout FactionList
            └─ Roles panel
               └─ ScrollLayout RolesScroll
                  └─ VerticalLayout RolesList
```

The scroll viewport owns the available screen area. The list inside it is a content flow container. The list does not define the fixed height of every child.

Relevant file:

```text
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\CoopLobby.layout
```

Useful evidence is around the `Lobby` overlay, `VerticalLayout1`, `HLWBody`, `FactionScroll`, `FactionList`, `RolesScroll`, and `RolesList` declarations.

### Group/card

`RolesGroup.layout` uses a root `VerticalLayoutWidgetClass` named `RolesGroup`:

```text
RolesGroup: VerticalLayoutWidget
├─ RolesGroupButton: ButtonWidget
│  └─ RolesGroupNameFrame: FrameWidget
│     ├─ background
│     ├─ group name
│     ├─ custom description/count
│     ├─ faction accent
│     ├─ member icon layout
│     └─ border
└─ List: VerticalLayoutWidget
   ├─ CharactersList: VerticalLayoutWidget
   └─ VehiclesList: HorizontalLayoutWidget
```

The visible group card is explicitly 84px high. The list is a sibling below the card, not a child inside the card's fixed rectangle. This is the important hierarchy difference for the tactical squad layout.

Relevant evidence in `RolesGroup.layout`:

- `RolesGroup` root: lines 1–6.
- Root group button and padding: lines 8–23.
- Group frame: lines 26–30.
- Group name and custom text: lines 180–220.
- Faction accent: lines 222–235, `SizeY 84`.
- Member layout: lines 236–249.
- Border: lines 250–264, `SizeY 84`.
- Separate `List`, `CharactersList`, and `VehiclesList`: lines 269–286.

### Repeatable slot/row

`CharacterSelector.layout` is a fixed row component:

```text
CharacterSelector: ButtonWidget
└─ Overlay: ButtonWidgetSlot, local padding
   └─ FCharacterSelector: FrameWidget
      ├─ Background: 608 x 40
      ├─ CharacterFactionColor: 6 x 40
      ├─ ScaleWidget: 40 x 40 role icon area
      ├─ CharacterClassName: fixed text region, 40 high
      ├─ StateIcon: 40 x 40
      ├─ CharacterStatus: fixed text region, 40 high
      ├─ voice button: 40 x 40
      └─ Border: 608 x 40
```

The root button gives the row an interaction contract. The nested frame provides a local coordinate system. Every visual child is pinned to the same 40px row height, so a parent list cannot compress the text and icons into a smaller row.

Relevant file:

```text
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\CharacterSelector.layout
```

Useful evidence is the root/button hierarchy at lines 1–25, the 608 x 40 background at lines 27–36, and the fixed-height role, state, status, and border regions through line 257.

## How Lobby2 should be structured

Keep Lobby2's current visual content, terminology, accent, role icon, status icon, role name, and player name. Change the structural shell to this:

```text
Lobby_Slot: ButtonWidgetClass
├─ slot button component
└─ OverlayWidgetClass
   └─ SlotFrame: FrameWidgetClass
      ├─ Background: ImageWidgetClass, fills SlotFrame
      ├─ Accent: ImageWidgetClass, fixed narrow width and row height
      ├─ HorizontalLayout: preserves the role/status grouping
      │  ├─ RoleSection
      │  │  ├─ RoleIcon
      │  │  └─ RoleName
      │  └─ StatusSection
      │     ├─ StatusIcon
      │     └─ PlayerName/status text
      └─ Border: PanelWidgetClass or image, fills SlotFrame
```

The key is not to replace every Lobby2 horizontal layout with absolute positioning. The other add-on uses direct frame children because its row has a carefully designed fixed coordinate map. Lobby2 can retain the horizontal sections, but they must be inside a fixed-height slot frame whose parent list is allowed to stack it without resizing it vertically.

### Ownership rules

| Property | Owner |
|---|---|
| Screen position and available viewport | Screen/menu layout |
| Scrollable area | Scroll widget |
| Squad header height | Squad/group layout |
| Number and order of slots | Squad/group script and its `Slots` list |
| Slot row height | `Lobby_Slot.layout` |
| Role/status/player arrangement | `Lobby_Slot.layout` |
| Hover/selected/clicked behavior | Slot root button component |
| Player, UID, cache name, and other player fields | One `SetPlayer(player)` method in the slot script |
| Group membership authority | Base-game group APIs, not a Lobby2 shadow roster |

## Sizing rules that prevent smushing

### Rule 1: Give the item its own fixed row contract

The slot layout must have a stable height in its own root or inner frame. For example, if Lobby2's intended row is 80px high, the slot's local row rectangle and its visual children should all use that same 80px contract. If the intended row is 40px, use 40px consistently.

The exact number can change later. The important part is that the number lives in the slot resource, not in `Lobby_Group_Tactical.layout` and not in the list's slot settings.

### Rule 2: Let the list stretch horizontally, not vertically

The group list should be allowed to fill the available width. Its children should receive horizontal fill/alignment where the parent slot type supports it. The list should not force every child to fill its available height.

Recommended responsibility split:

```text
Slots list: horizontal fill, vertical content/auto flow
Slot item: fixed minimum/explicit row height
Slot internals: fixed-height icon/text regions, horizontal flow as needed
```

### Rule 3: Do not mix incompatible slot keywords

A slot declaration is determined by the parent widget type. `FrameWidgetSlot`, `ButtonWidgetSlot`, `OverlayWidgetSlot`, and layout slots are not interchangeable.

Examples:

- A child of a `FrameWidget` uses `FrameWidgetSlot` and can use explicit local position/size.
- A child of a `ButtonWidget` uses `ButtonWidgetSlot`.
- A child of an `OverlayWidget` uses `OverlayWidgetSlot`.
- A child of a `HorizontalLayoutWidget` or `VerticalLayoutWidget` uses a layout slot and may use alignment/padding/fill settings supported by that slot.

This is why putting `HorizontalAlign` on a frame child produced an error in the recent slot work: the keyword was being applied to the wrong slot type.

### Rule 4: `SizeToContent` is local, not a row-sizing strategy

The reference add-on uses `SizeToContent` for small content-driven areas, such as a local member-counter overlay. It does not use root `SizeToContent` to make a repeatable row negotiate its height.

Use it for a small label or counter only when that local widget should grow to its content. Do not use it on the slot root, the slot list, or the entire group as a replacement for a known row contract.

### Rule 5: Treat `SizeLayoutWidget` as a sizing tool, not a generic container

The engine declaration confirms that `SizeLayoutWidget` derives from `Widget`, not `LayoutWidgetBase`. Its API exposes width, height, minimum desired size, maximum desired size, and aspect-ratio controls. It is therefore a sizing wrapper, not a substitute for `FrameWidget`, `OverlayWidget`, `HorizontalLayoutWidget`, or `VerticalLayoutWidget`.

For Lobby2, do not make it the root of `Lobby_Slot.layout` or use it as the slot list's generic container unless the exact prefab/slot structure has been validated in Workbench. Our previous test demonstrated the practical failure mode: the wrapper retained the accent while the row content was not laid out as intended.

## Dynamic widget pattern

The reference scripts create a layout resource directly into a list and initialize the returned handler immediately.

`PS_RolesGroup.c` follows this pattern:

```text
CreateWidgets(characterLayout, CharactersList)
→ FindHandler(PS_CharacterSelector)
→ SetLobbyMenu(...)
→ SetRolesGroup(...)
→ SetPlayable(...)
→ store the handler in a map
```

The relevant methods are `InsertVehicle`, `InsertPlayable`, and `AddMember` around lines 154–200.

`PS_PlayersList.c` uses the same pattern in `AddPlayer` around lines 65–75:

```text
CreateWidgets(playerLayout, PlayersList)
→ FindHandler(PS_PlayerSelector)
→ SetCoopLobby(...)
→ SetPlayersList(...)
→ SetPlayer(playerId)
→ store the handler in a map
```

For the future Lobby2 dynamic slice:

1. `Lobby_Group_Tactical` owns the `Slots` vertical layout as an insertion target.
2. The group script creates `Lobby_Slot.layout` into `Slots`.
3. The returned slot handler receives one authoritative player through `SetPlayer(player)`.
4. That method updates the player name, UID, cache name, occupancy state, and status icon together.
5. The group tracks widget references for removal/rebuild, while base-game APIs remain authoritative for membership.

Do not statically embed `Slot1`, `Slot2`, and `Slot3` once the dynamic slice begins. Static preview instances are useful for layout prototyping, but runtime-created items are what let the row layout own its sizing cleanly.

The Official Wiki documents the same creation primitive as `GetGame().GetWorkspace().CreateWidgets(layoutResource, parentWidget)`: [Arma Reforger Layout Creation](https://community.bistudio.com/wiki/Arma_Reforger:Layout_Creation).

## Keyword and class sheet

### Widget classes observed in the reference layouts

| Keyword | Use |
|---|---|
| `FrameWidgetClass` | Explicit local coordinate rectangle; good for a fixed slot row's inner frame. |
| `ButtonWidgetClass` | Interactive root for a repeatable selectable item. Put the item button component here. |
| `OverlayWidgetClass` | Layering/padding shell; useful between a button and its visual frame. |
| `HorizontalLayoutWidgetClass` | Horizontal flow of role/status sections or toolbar items. |
| `VerticalLayoutWidgetClass` | Vertical flow for groups, slots, panels, and content lists. |
| `ScrollLayoutWidgetClass` | Viewport for content that may exceed the available screen area. |
| `ImageWidgetClass` | Accent, background, icon, divider, or state image. |
| `PanelWidgetClass` | Styled panel/border, such as `style pixel_outline`. |
| `TextWidgetClass` | Plain text. |
| `RichTextWidgetClass` | Text that may contain rich/localized formatting. |
| `ScaleWidgetClass` | Fixed icon area that scales a nested visual, as used by the reference role icon. |

### Slot types and where they belong

The following engine slot classes are verified in Game Data:

| Slot class | Parent/use |
|---|---|
| `LayoutSlot` | General layout parent. |
| `HorizontalLayoutSlot` | Child slot generated for a horizontal layout. |
| `VerticalLayoutSlot` | Child slot generated for a vertical layout. |
| `ScrollLayoutSlot` | Child slot associated with a scroll layout. |
| `SizeLayoutSlot` | Sizing-layout slot class; do not write it as a widget class. |

Workspace-authored layouts also contain parent-specific serialized slot forms such as `FrameWidgetSlot`, `ButtonWidgetSlot`, `OverlayWidgetSlot`, and `AlignableSlot`. Use the slot type Workbench gives the child under that parent. Do not copy an alignment keyword from a layout slot onto a frame slot.

### Layout and sizing properties

| Property | Use/caution |
|---|---|
| `Anchor` | Parent-relative anchor. Useful for a screen/viewport or a background that truly fills its parent. Avoid relying on it for a fixed repeatable row when explicit local dimensions are clearer. |
| `PositionX`, `PositionY` | Local position in a frame-like parent. |
| `OffsetLeft`, `OffsetTop`, `OffsetRight`, `OffsetBottom` | Local edge offsets. They make a fixed row's rectangle explicit. |
| `SizeX`, `SizeY` | Explicit desired region size. Use the slot's row height here or on its inner frame. |
| `Padding` | Spacing owned by the immediate parent/list item relationship. Do not use padding to fake the item's minimum height. |
| `HorizontalAlign` | Use only on an alignable/layout slot that supports it. |
| `VerticalAlign` | Use only on an alignable/layout slot that supports it. |
| `SizeMode Auto` | Content/desired-size behavior for the applicable layout slot. |
| `SizeMode Fill` | Fill available space in the applicable layout direction. Use for the list's width or a counter region, not to vertically compress fixed-height slots. |
| `FillWeight` | Proportional layout sizing where supported by the parent layout. |
| `SizeToContent` | Local content sizing; avoid using it on the whole repeatable row/list. |
| `Fill Origin` | Direction/origin behavior of a layout; the reference uses `Right` for its member layout. |

The verified `LayoutSizeMode` values are `Auto` and `Fill`. The verified `SizeLayoutWidget` API includes minimum desired width/height, maximum desired width/height, explicit width/height overrides, and aspect-ratio controls. Those APIs do not make `SizeLayoutWidget` a valid generic child container.

### Common visual properties

These are used by the reference layouts and are appropriate when the target widget supports them:

```text
Color
Texture
Image
Size
Font
Font Size
Min Font Size
Horizontal Alignment
Vertical Alignment
FontProperties
style
Z Order
Is Visible
Is Enabled
Ignore Cursor
no focus
Inherit Color
```

Keep behavioral properties on the script component when the widget already has a component-driven state system. For example, the reference button components own default, hovered, selected, and clicked background colors rather than requiring every visual state to be manually reconstructed in the layout.

## Keywords that caused recent Lobby2 errors

### `SizeLayoutSlot` used as a widget

`SizeLayoutSlot` is an engine slot class, not a layout widget. A declaration such as `SizeLayoutSlot { ... }` in a place where a widget is expected produces `Unknown class 'SizeLayoutSlot'`.

Correct approach: use a real widget (`FrameWidgetClass`, `OverlayWidgetClass`, or an actual layout widget) and let the parent generate the appropriate slot. Only author a `SizeLayoutSlot` where Workbench has established that the parent expects that slot type.

### `HorizontalAlign` on the wrong slot

`HorizontalAlign` is not a universal property. It belongs on the appropriate alignable/layout slot. Applying it to `FrameWidgetSlot` produced `Unknown keyword/data 'HorizontalAlign'`.

Correct approach: for a frame child, use `PositionX`, `OffsetLeft`, `SizeX`, and `OffsetRight`. For a child of a horizontal/vertical layout, use the layout's supported slot and then apply alignment there.

### “Fill” is not the same as “fixed minimum height”

`SizeMode Fill` asks a layout to use available space. It does not preserve the slot's intended row height. If the parent list is allowed to distribute or compress height, the row can still become visually smushed.

Correct approach: make the slot row a fixed-height item, then allow only the horizontal dimension to fill.

## Recommended Lobby2 implementation order

This is the smallest safe sequence for the next implementation slice:

1. Refactor `Lobby_Slot.layout` into a button-rooted shell modeled after `CharacterSelector.layout`.
2. Keep `RoleSection` and `StatusSection` as horizontal sections inside a fixed-height slot frame.
3. Give the slot frame, accent, icon regions, and text regions one shared row-height constant in the layout resource.
4. Keep player-related initialization behind one `SetPlayer(player)` method. That method updates every derived player field and the visible state together.
5. Keep `Lobby_Group_Tactical.layout` responsible only for the squad header, divider, and `Slots` insertion target.
6. During the dynamic slice, create each slot with `CreateWidgets` into `Slots`; remove the static preview slot instances.
7. Validate one slot, three slots, and a full squad in Workbench. Check that width fills the list while row height remains unchanged.
8. Inspect the Workbench log after reload. Treat any unknown keyword/class as a hierarchy or parent-slot mismatch before changing dimensions.

## What not to copy blindly

The other add-on is a useful structural reference, not a drop-in template. Do not copy its exact 608px width, 40px row height, faction-specific colors, group naming, or script classes into Lobby2. Preserve Lobby2's visual format and data model.

Do copy these patterns:

- fixed-size repeatable item;
- root button for interactive item state;
- visual frame inside the button;
- parent lists used as insertion targets;
- explicit group header separated from the child list;
- dynamic `CreateWidgets(layout, parent)` creation;
- immediate handler initialization;
- one method that sets the item's authoritative data;
- stable named child widgets for script binding;
- local, limited use of `SizeToContent`;
- no unsupported keywords on the wrong parent slot.

## Evidence index

### Comparison add-on

```text
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\CoopLobby.layout
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\RolesGroup.layout
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\CharacterSelector.layout
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\PlayerSelector.layout
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\UI\Lobby\MemberCounterLabel.layout
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\scripts\game\UI\Lobby\PS_RolesGroup.c
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\scripts\game\UI\Lobby\PS_PlayersList.c
C:\Users\Gray\Documents\My Games\ArmaReforgerWorkbench\addons\ReforgerLobbyCommunity\PlayableSelector\scripts\game\UI\Lobby\PS_PlayerSelector.c
```

### Lobby2 files compared

```text
UI/layouts/Groups/Lobby_Slot.layout
UI/layouts/Groups/Lobby_Group_Tactical.layout
```

### Engine/API declarations checked in Game Data

```text
scripts/Core/generated/UI/LayoutSlot.c
scripts/Core/generated/UI/HorizontalLayoutSlot.c
scripts/Core/generated/UI/VerticalLayoutSlot.c
scripts/Core/generated/UI/ScrollLayoutSlot.c
scripts/Core/generated/UI/SizeLayoutSlot.c
scripts/Core/generated/UI/LayoutSizeMode.c
scripts/Core/generated/UI/LayoutWidgetBase.c
scripts/Core/generated/UI/HorizontalLayoutWidget.c
scripts/Core/generated/UI/VerticalLayoutWidget.c
scripts/Core/generated/UI/SizeLayoutWidget.c
scripts/Core/generated/UI/WorkspaceWidget.c
```

### Official documentation

[Arma Reforger: Layout Creation](https://community.bistudio.com/wiki/Arma_Reforger:Layout_Creation)

The official page covers layout resources, widget-library prefabs, menu/dialog setup, and dynamic `CreateWidgets` usage. It also reinforces the practice of giving authored widgets stable names because scripts later find those widgets by name.

## Status of this sheet

This document is an evidence-based design handoff. It does not modify `Lobby_Slot.layout`, `Lobby_Group_Tactical.layout`, or any script. The next implementation should be a separate, small slice followed by Workbench reload and log inspection.

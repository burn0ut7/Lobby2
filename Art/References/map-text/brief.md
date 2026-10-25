# Map text visual target

Journey: open the existing map, read mission-authored labels at their world locations, pan/zoom, close normally. Text is passive and never captures input. Existing shape colors, clipping and navigation remain intact.

| Element | Player purpose | State owner | Decision |
|---|---|---|---|
| Shape label | Identify the region | Mission text/style, server-filtered snapshot | Optional native text at an explicit world anchor |
| Standalone label | Identify a place without a polygon | Same component/snapshot | Text-only prefab if included in scope |
| Font, size, color | Author readable mission notation | Workbench/mission API | Native font resource and UI-pixel size |
| New map controls or legends | None needed | None | Omit |

Concept prompt: preserve the supplied real briefing-map screenshot and all existing UI. Add a modest white label with a thin dark outline inside the blue polygon, two lines reading ALPHA then СЕКТОР АЛЬФА. Add a second modest off-white label СБОР / RV-1 on the open map to its left. No plates, icons, extra controls, panels or altered map art. Fixed readable screen size and balanced spacing. The image is a proposed UI reference, not runtime evidence.

Pass traits: actual Cyrillic glyphs, readable native typography over bright/dark terrain, clipping to the map, stable world anchoring during native pan/zoom, visible font/size changes, no input interception. Font glyph coverage is font-dependent; do not promise every Unicode script. Nominal editor viewport and 1280x720 peer will be checked.

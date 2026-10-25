# Minimal boundary HUD

Minor size/spacing revision to ../compact-hud/concept-v1.png, guided by user-reference.png. User requests 25% smaller content, about 80% less outer whitespace, and an even smaller skull-to-text gap. Keep the existing skull, white bold text, dark backing, custom labels and passive input. Content-sized cards replace the fixed-width empty space. Warning fonts 28/40 -> 21/30, skull widget 60x64 -> 45x48; countdown fonts 24/28 -> 18/21. Outer horizontal padding 2 design units; title and timer separated by 6. Warning icon overlaps its column through transparent texture margins, leaving the visible glyph beside text. Cards 60/26 high, stack strides64/30.

Acceptance: actual visible skull close to text, tight backing without clipping, long/custom headings and minute timers fit, setup/announcement/both return warnings remain distinct and nonoverlapping. No gameplay behavior changes.

User refinement: first in-game iteration was too tight. Preserve the smaller content but standardize breathing room: 6-unit horizontal outside margins and stack gaps, 8-unit heading/timer gap, visible skull/text gap around6, warning text inset4 vertically. Final heights68/32 and strides74/38. Content widths continue to fit labels. This supersedes the initial near-zero padding values above.

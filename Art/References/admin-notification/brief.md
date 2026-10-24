# Admin notification
Active reference: concept-v1.png, built-in image generation, 2026-10-23. Illustrative background, not runtime evidence. Prompt: passive top-center neutral charcoal banner with small muted Admin/sender label and larger white message, no border, outline, accent, icon, dismiss or focus; 32px top inset, compact content-sized height, 15-second automatic fade.

Journey: receive sound/notice -> read sender/message -> keep playing; automatic expiry. No choice or input required.

| Element | Purpose | Owner | Decision |
| --- | --- | --- | --- |
| Sender label | Identify authoritative sender | Received message | Keep small, muted |
| Message | Read announcement | Received message | Keep readable, wrap |
| Neutral surface | Contrast over bright/dark scenes | Layout | Keep translucent charcoal |
| Border/accent | No necessary meaning | Layout | Remove |
| Dismiss | No response required | UI | Remove |
| Scroll control | Incompatible with passive notice | UI | Remove; fit display text |

Native adaptation: square corners, normal game font; width capped at 780 layout units and viewport width minus 48; root anchored at top center with 86-unit inset (70-unit lobby navigation plus 16-unit gap; the initial 32-unit capture overlapped the existing navigation). Blank-line formatting flattened only in display copy, stored message unchanged. Sound, delivery audience, queue and 14-to-15-second fade lifecycle retained. All widgets ignore cursor. Keep existing resource/class identity to avoid breaking references.

Acceptance: screenshot shows top-centered short banner with clear view center, no buttons/border; long text fully wraps; queue advances automatically after 15 seconds; no focus/input context introduced. Compare short/long screenshots with concept. Physical controller input and real audio audibility require separate manual coverage.

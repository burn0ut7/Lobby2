# Slot reservations reference

Journey: Admin Tools > Slot reservations > select slot > enter Arma UID or choose a connected player > Reserve slot > confirmed result. Players may change slots under ordinary phase rules; reservation never pins them. Clearing an initial reservation is separate from removing an active player. Import/export is a secondary view, with explicit paste/copy and apply actions.

| Element | Purpose | Owner / visibility |
|---|---|---|
| Existing tools rail and Close | Find the tool and leave | Menu; always |
| Slot reservations title and short description | Explain pre-assignment | Tool; always |
| Reserve / Import & export tabs | Separate single-slot and batch tasks | Local presentation |
| Scrollable slot list with faction, group, role and number | Choose an unambiguous target and inspect status | Replicated roster; Reserve view |
| Selected slot and current assignment | Confirm affected target | Replicated roster; Reserve view |
| Arma UID field and connected-player picker | Supply one persistent identity | Local draft, server revalidates; Reserve view |
| Reserve slot / Clear reservation | Commit or release pending pre-assignment | Authenticated server; Reserve view |
| Text area, Export roster, Copy, Paste, Import reservations | Transfer an editable versioned roster manually | Local draft and authenticated server; Import view |
| Inline pending, success or failure status | Distinguish request from result; preserve draft on error | Correlated server response |

Theme: existing full-screen charcoal Admin Tools, left rail about 21%, off-white condensed type, quiet gray outlines, restrained amber selection and primary action. Square controls and large readable labels. No decorative cards, counters, debug information or duplicate instructions. Slot IDs and UIDs are relevant here because admins edit reservations.

Baseline: actual MCP screenshot `.local/admin-implementation/admin-menu-final-heal.png` in the MCP repository, inspected 2026-10-23. It records a previous build, not this slice's validation. Native button and input APIs are checked against installed data007/core source revision 7a245290a0654080b47e291e454feef8. Wiki research reported unprepared; cached UI research remains guidance, not live evidence.

Concept: built-in image generation, UI mockup using that baseline as reference. Preserve the rail/Close and replace main content with Reserve view: slot list left, selected slot, Arma UID field, Choose player field and Reserve slot/Clear reservation right. Include secondary Import & export tab. Representative data and geometry are illustrative. The generated image must be saved as concept-v1.png before visual implementation. Normal keyboard/mouse path is primary; constrained viewport and gamepad coverage must be recorded honestly in validation.

Acceptance: readable target and UID, clear selected slot, one primary reserve action; no silent mutation from selection; successful authoritative result, recoverable conflicts, permission loss, repeated open/close, and import error without partial claim changes. Compare original MCP screenshots to the saved concept; native adaptations and missing coverage must be recorded.

# Map plans — concept v1

Status: implemented and compared with actual embedded-Play rendering; independent review recorded in tests/MapPlans/README.md.

Request: admin-only Save, Load, and Clear for the whole mission's shared markers
and supported Till W drawings. Server recovery restores the immediately preceding
mission only when its last activity is no more than two hours old. Clearing updates
recovery. Manual plans do not expire. Load and Clear are preparation-phase actions.

Journey: Admin tools → Map plans → enter a plan name → Save, or confirm Load/Clear
→ receive the actual result → Close. Named manual plans live on the admin's computer
so prepared files can be shared; recovery lives on the server. Private player notes
are outside the shared mission map.

| Element | Purpose and visibility | Owner | Decision |
|---|---|---|---|
| Existing admin navigation and Close | Entry, location, and exit | Admin menu | Reuse |
| Map plans heading | Identify current task | Layout | Keep |
| Shared markers and drawings · All factions | Identify destructive scope | Layout | Keep |
| Plan name | Choose portable saved plan | Local UI | Keep |
| Save, Load, Clear map | Requested actions | Server authority / local file storage | Keep |
| File location/help | Find files for sharing | Local storage | Keep near name |
| Recovery sentence | Explain automatic two-hour behavior | Recovery service | Keep |
| Result / confirmation and Cancel | Actual outcome and safe dismissal | Request controller | Show when relevant |

Theme: existing dark blue-black surface, off-white primary text, subdued secondary
text, yellow selected/focused state, native rectangular buttons and condensed type.
No dashboard cards, extra counters, or repeated scope labels. Preserve existing
admin tools geometry. Keyboard/mouse first; gamepad and constrained viewport require
runtime verification and must not be inferred from this concept.

Source: built-in image-generation skill/tool, using the actual MCP screenshot
`.local/admin-faction-tools/heal.png` in the MCP workspace as a theme/context reference.
Active image: `concept-v1.png`. Original generated file:
`C:/Users/Gray/.codex/generated_images/01a16ced-9c0b-7ec2-add7-5ba373d9ed14/exec-6afc7468-cb78-4ad5-bd7e-965731b11a10.png`.

Prompt: preserve the screenshot's Workbench surround and Admin Tools workspace;
add Map plans below Heal, Teleport, Send message, Slot reservations, and Kick / Ban;
show heading Map plans, scope Shared markers and drawings · All factions, Plan name
field with Arland briefing, Save / Load / Clear map, local-file sharing help, a single
two-hour recovery sentence, Ready status, and existing Close. Focus Save. No extra
charts, cards, badges, duplicate headings, or invented interactions. Generated
image is concept art, not game evidence.

Concept critique: the task, scope, name and three actions are clear and retain the
existing tool composition. Implementation must replace the generic file-sharing
sentence with a useful location, show explicit Load/Clear confirmation, and display
actual save failures; the static concept cannot prove those behaviors.


Runtime comparison: original MCP capture `1792831849154685600-194390034ac545b792e066ae913b1319.png` in the MCP workspace `.local/map-plans/`, source 2560x1440, output 1920x1080. The native panel retains the concept's geometry, hierarchy, yellow selection, name field, action row and Close. The implementation adds explicit filename location, confirmation/Cancel and actual operation results. ASCII punctuation replaces the concept's typographic punctuation because the engine truncated those labels in the first observed rendering (failure capture `1792831305625272800-36a112f9985d47d9814c205575ef7296.png`). Handler-driven Save/Load/Clear, cancel, GAME restrictions and repeated close/reopen were exercised; physical input, gamepad and constrained viewport remain unverified.


User revision: remove explanatory text and all separate input labels; use only `Name` inside the field. The active implementation intentionally removes the concept's right-panel title, scope, file help and recovery sentence. Keep navigation, Close, input and action buttons; show only action confirmations/results or actual failures. Successful background recovery has no visible status. Guidance lives in docs/MapPlans.md. This explicit user revision supersedes the concept's text inventory; no additional visual concept is needed for this subtraction.

Final user-revised panel: `1792832777628558300-b50f6894c20541888e32ed24fbd11d0e.png` in MCP `.local/map-plans/`. Name placeholder and aligned buttons only, with blank initial status. Final compilation receipt `1792832762260070700-99e5c67d769f41988495c9d8a6e45abc`. Save overwrite, confirmation/cancel and reopen rerun after text reduction; no SCRIPT errors (`1792832714227398400-f7fa52b1e21343f794abaf0e0aae2e82`). Temporary button-anchor misalignment in earlier revision captures was corrected and recaptured.

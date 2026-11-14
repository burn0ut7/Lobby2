# GC Lobby Workbench extension

GC Lobby owns `gcworkbench.dll`, GC Tools and the dependency installer. Reforger
MCP is a separate generic development tool and is neither shipped nor invoked.
Workbench automation, suppression and script-authorization policy remain MCP-only.

## Build

From `external/source`, run `scripts/build-workbench-authoring.ps1 -QtRoot <Qt-6.8.3-SDK> -OutputDirectory <new-build-folder>`.
The SDK and MSVC/CMake are developer prerequisites; no sibling checkout is required.
Then run `scripts/package-workbench-integration.ps1 -NativePackage <build-folder>/package -OutputDirectory <new-package-folder>`.
This publishes only the GC UI, fixed DLL loader, Git scripts and their .NET runtime.
Source hashes and package hashes are recorded. Game binaries are not redistributed.

## Faction fields

The DLL recognizes the declared `FactionKey` type (including `array<FactionKey>`
entries) when Workbench creates a property editor. No field names, GC class list,
widget annotation or enum-provider marker is required. Ordinary strings remain
ordinary strings, even if their names contain "FactionKey". Hidden attributes
remain hidden. The shared provider reads the current editor world's factions on
refresh. Existing explicit provider markers remain supported for compatibility.

GC authored fields use `FactionKey`; existing string-based runtime/RPC interfaces
convert through `GC_FactionKeys`. Values, defaults and empty-list semantics stay
the same. The type audit checks GC's declarations, not the DLL's discovery scope.

Run `scripts/Assert-FactionChoiceFields.ps1` after script changes. The DLL build
also runs this audit. It checks authored faction fields use the semantic type
without manual faction-provider markers. Hidden fields and the mission header's
generated evidence are excluded.

Native reflection follows the same paths as `string.ToType`,
`typename.GetVariableName` and `typename.GetVariableType` in Workbench 1.9.0.37.
Evidence: `226fe901032b4d068e5a93da68576d9b` (type lookup),
`833a01e0f6b147b6ba49e5c8574aed5a` (field name),
`13e3061227cb47cb8570fd3a5ebbdd6e` and
`90ab9c816c1948808af21bc9c3991c61` (field type). Lookups are synchronous on the
editor thread and do not cache VM pointers across reloads. The exact-host checks
apply before this path is installed.

Non-entity property sources use the native root type predicate directly, rather
than the generated script cast guard (which raises on a type mismatch). Evidence:
`b925bc3430af42c9a162729010648e00`, `04b863a195504d1b9cd7f41a3dc33134`.
This keeps tool/configuration dialogs eligible without claiming an entity edit
destination for them.

Live verification on 2026-11-14, Workbench 1.9.0.37: an external temporary
ScriptDialog with unannotated, arbitrarily named inherited scalar, array and
nested `FactionKey` fields displayed dropdowns and returned `US`, `USSR`, `US`
after selection. A plain string named `FactionKeyLabel` remained a text field.
The AO entity's array destinations still resolved after the non-entity guard
change. The probe was removed and production scripts reloaded without compile
errors. Reload still reported existing Arland resource references and a
`PacejkaPlotTool` leaked instance; this is not a clean whole-project diagnostic
baseline. All ten native tests and six audit fixtures passed.

## Startup ownership

The profile bootstrap launches `GCWorkbench.Loader.exe` without arguments.
The loader accepts only the active installation and its actual Workbench parent,
checks the supported executable and packaged DLL hash, and queues the one fixed
native initializer. There is no arbitrary DLL, script, input or editor-control API.
The DLL validates the installed Qt identities before attaching to choice fields.
A process-specific attempt record prevents replay after an uncertain dispatch.
Native hook confirmation, rather than DLL presence, records successful loading.

`GC_EditorChoices.c` and `GC_EntityReferenceEdit.c` are GC Lobby-owned providers.
Their protocol fingerprint is `4df00e70851952d140c187dfefe9c59d274c81d0d9e30a12cd06d87b28accd8f`.
Change both consumers deliberately when changing the wire contract. Generic
MCP adapters have their own RST names and lifecycle. GC Tools has no editor-control
connection: submission and mod updates operate only on saved files and GitHub.

The installer reads Workbench's saved NET API preferences. It does not enable,
reconfigure or suppress the editor. Missing/disabled NET API is an explicit setup
requirement. Normal Workbench authorization prompts remain an editor policy.

## Verification

Native tests cover compatibility refusal, direct and queued initialization,
editable choices, array operations, wire protocol and menu presentation.
The build script runs these before creating a package. GC UI tests are under
`tools/tests`; Git tests use isolated repositories and mocked GitHub responses.
A native rebuild requires a normal saved-work Workbench restart before live testing;
never load two dropdown implementations into the same process.

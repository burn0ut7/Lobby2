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

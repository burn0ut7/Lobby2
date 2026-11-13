# GC tool windows

A small reusable Windows presentation layer for the external GC tools. It uses
WPF, whose runtime is already shipped in the Workbench integration package. No
browser engine, UI framework installer, network connection or game addon is needed.

This is the shared presentation layer for the tools hub, mission submission and
conflict review. The PowerShell workflows own permissions, validation, file changes
and verified outcomes. The hub stays open during actions and embeds review forms
in its right panel. Workbench launches it without waiting, so saving/export and
installer requests can reach the editor. A launch-scoped session authorizes the
project-owned GC_ToolsBridge NET API handler; it only accepts save/update and
mission-prepare actions, never code or caller-provided file paths.

`--tools` returns 2 only when closed. It no longer returns action exit codes.
Action buttons show progress and the workflow's explicit outcome; no success is
inferred merely because a child exited. A submitted PR also gets View submission.
A current-user-only named pipe carries JSON review requests/answers between the
hub and its PowerShell child. The hub cannot close mid-action or bypass required
review; Cancel on a review returns cancellation to the owning workflow. Background
checks cannot replace an active review. Update/submission safety checks remain in
the owning scripts, including fresh conflict checks before changes.

Generic standalone requests still return named `actions` or confirmation with
form values. The preview's statuses are illustrative, not machine observations.

## Try the design

Build this project, then run `GCWorkbench.UI.exe --preview`. The sidebar switches
between dependency setup, mod updates and mission submission. Fields and selection
cards work; required-field errors appear beside their inputs. Preview actions never
install, update or submit anything. Switching pages retains the current form values.

## Reuse a window

Pass a UTF-8 JSON request and a new result path:

```powershell
& $uiExecutable --request $requestPath --result $resultPath
```

Both paths must be absolute. Exit codes: `0` confirmed, `2` cancelled (including
Escape or closing the window), `1` failed. The result contains `Action` and `Values`,
with text values as strings and checkbox values as booleans. An existing result
file is never overwritten. Do not treat confirmation as completion of an operation.

```json
{
  "title": "Name your mission",
  "section": "Mission details",
  "primaryAction": "Continue",
  "secondaryAction": "Cancel",
  "accent": "#d4b874",
  "fields": [
    { "id": "name", "label": "Mission name", "kind": "text", "required": true },
    { "id": "notes", "label": "Notes", "kind": "multiline" },
    { "id": "tested", "label": "Tested in Workbench", "kind": "check", "checked": false }
  ]
}
```

`items` adds read-only status cards with `title`, `description`, `status` and
`state` (`ready`, `pending`, `warning`, `error`). Text is plain text; requests cannot
run commands or embed markup. Set `readOnly: true` on text fields for selectable file previews and result links.
Labels, helper text, sections, action text and accent
are configurable. `ToolWindow` and `DialogSpec` can also be used directly from C#.
Layout uses scalable Windows units, wraps descriptions, scrolls the form body and
keeps actions visible. Field validation checks required values only; workflows
must validate their own domain rules.

## Packaging

Publish alongside the existing helper into its payload directory using the same
.NET target/runtime version. Both executables share the bundled runtime files;
there is no second runtime installation. Do not add this preview to game scripts
or make gameplay depend on it. The same presentation layer hosts progress, reviews and confirmed outcomes.

The theme uses the GC website field palette: background #191e18, text #f0e9d5,
muted #a9b09b, border #404735, accent #d4b874, panel #242b20, button ink #1b2119.
The integrated caption retains dragging, resizing, minimize, maximize and close.
GC Tools launches its registered executable directly through WScript, without
PowerShell, package hashing or extraction on the normal opening path. Installation
registers the app in %LOCALAPPDATA%/GCWorkbench/ui-path.txt (UTF-16). An uninstalled
source checkout prepares and registers a package-hash cache once. Preview retains
the cache bootstrap path. Cache versions live under %LOCALAPPDATA%/GCWorkbench/ui.

## Account and dependency status

The GitHub page uses Git Credential Manager's browser OAuth flow. No embedded login,
password field, token field, or custom token store is provided. The owning helper
`GC-GitHub.ps1` forces Windows Credential Manager storage, verifies the token against
GitHub `/user`, and checks mission-repository access separately. Only a safe status
object crosses into the UI. Submission reuses this same credential implementation.
Public mod updates do not require sign-in. API calls necessarily use the access
token briefly in memory; it is never written into GC files or status results.

All hub checks run asynchronously after opening. Refresh rechecks; closing cancels
owned helper processes. Browser sign-in is started only on an explicit button press.
There is no forced logout or automatic account switch.

GC Workbench Tools compares installed payload/helper/profile-script hashes against
`external/workbench/GCWorkbench.zip` from the launching Lobby checkout. The package
checksum is checked first. This means current relative to that Lobby package, not
a remote GC release channel. A changed payload is reported as Update or repair
available; a missing payload is Installation incomplete. This checks installed
files, not whether the native extension is currently loaded into Workbench.

Git for Windows compares its installed Windows revision with the official latest
release API and checks that Git Credential Manager is present. Network failures,
missing source-package context, damaged manifests and unverified credentials are
shown as unavailable/unknown; they are never reported as current or connected.

Validation: `tests/GitHub.Tests.ps1`, `tests/Status/Status.csproj`, and the existing
submission suite. The account check was also exercised read-only with the current
account. Browser authorization itself requires a normal contributor acceptance pass.

## Mod update status

Opening GC Tools starts account, mod and dependency checks together in the background
after the quick startup presence check. Missing dependencies instead open setup and
check dependency status after the OK notice. Each mod
appears as its check finishes, and ready mods can be selected while others finish.
Switching pages reuses this window's results; Refresh explicitly checks again.
Closing and reopening starts fresh checks. Process creation also runs off the UI
thread. Workbench passes the loaded project paths to the hub;
unloaded sibling projects use the same verified discovery map as the updater.

Up to date includes personal branches already containing the shared default-branch
tip. These rows are unchecked and disabled. Behind branches offer Update available;
diverged personal branches offer an update with possible review. Diverged default
branches require review and are disabled, matching the updater's preservation rule.
Missing folders, failed checks and checks still in progress cannot be selected.
The update button requires at least one enabled, selected row. Disabled rows are
also filtered from result values, regardless of saved selection state.

Checks may fetch Git objects for ancestry comparison but do not change HEAD, branches,
the real index, working files or FETCH_HEAD. They never install an update. The actual
updater still fetches/rechecks before applying changes; status is an observation, not
a guarantee that remote state will remain unchanged.

Tests: `tests/ModStatus.Tests.ps1`, `tests/ModSelection/ModSelection.csproj` and the
status mapping checks in `tests/Status/Status.csproj`.

## First launch

GC Tools uses a quick local dependency-presence check before choosing its initial
page. Missing installation files, profile helpers or Git open External dependencies
immediately. A bounded local Credential Manager check also catches that missing
component. The window shows one owned 'External dependencies not found.' notice
with OK. Dismissing it never dispatches installation; users press Install / update.
The install button remains available while detailed dependency checks run.

Existing installations with updates available retain the normal starting page.
Startup presence checks do not query the network or hash the package. An uncertain
Credential Manager timeout does not falsely report a missing dependency. The
notice is shown once per window opening while dependencies remain missing.

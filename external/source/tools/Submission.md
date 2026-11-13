# Mission submission

GC Tools submits saved GC Missions files for approval through GitHub. It does not
save or export a Workbench world, inspect the editor, or require Workbench running.
Save your work before selecting Submit.

## Account

Git Credential Manager handles GitHub browser sign-in. GC Tools uses the same
credential provider for Git and API requests; it has no password form. Submission
creates a personal fork when needed, so contributors need no upstream write access.

## Workflow

1. Read the current branch and saved, unignored files in the GC Missions checkout.
2. Fetch the latest shared main and any existing personal submission branch.
3. Combine those changes, resolving conflicts in the hub before uploading.
4. Review the mission name, notes and complete list of changed files.
5. Save recovery references and a backup of local changes, then apply the reviewed
   result to the local branch. Starting on main creates a separate mission branch;
   an existing mission branch keeps its name.
6. Push without force to the contributor's fork and create or reuse its pull request.

All saved changes in that checkout are included, including dependency declarations
and supporting files. GC Tools does not infer or download missing mission assets.
The shared main branch is never pushed by submission. The local mission branch
contains the fetched main before upload. Cancellation during review leaves the
checkout unchanged. Recovery references live under refs/gc/backups.

## Conflicts and retries

Readable text conflicts show My version and Shared version with explicit file
choices. Binary, oversized and unsupported file conflicts stop for manual help;
GC Tools does not guess a winner. Changed local files, online submissions or main
invalidate a review. A rejected upload keeps the local result for a safe retry.

GitHub can change after any check. Pushing to an existing pull request immediately
updates its head. If main changes during upload, GC Tools requests another Submit
to synchronize it; that check cannot undo an already uploaded head. GitHub's final
mergeability check may still be pending when the request is created.

## Verification

Run external/source/tools/tests/Submission.Tests.ps1 with Windows PowerShell.
It uses isolated local Git repositories and mocked GitHub requests; no live pull
requests or user checkout mutations occur. Live contributor permissions remain
subject to GitHub's responses.

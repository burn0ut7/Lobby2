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
4. Review the mission name, overview, testing checklist, reviewer comments
   and Added / Changed / Deleted file lists. The form shows the
   supplied submission rules and testing-guide address. Only checks the author
   selects are marked complete in GitHub's Markdown checklist. Existing request
   text and checks are restored; a new request after merge starts unchecked.
5. Save recovery references and a backup of local changes, then apply the reviewed
   result to the contributor's one submission branch. If it already exists,
   reuse it; never create a per-mission or per-attempt branch.
6. Push without force to the contributor's fork and create or reuse its pull request.

All saved changes in that checkout are included, including dependency declarations
and supporting files. GC Tools does not infer or download missing mission assets.
The shared main branch is never pushed by submission. The local mission branch
contains the fetched main before upload. Cancellation during review leaves the
checkout unchanged. Recovery references live under refs/gc/backups.

## Units / loadouts first

Submit mission checks the GC Units source checkout first, including saved files,
local submission commits and the online submission branch. If it has pending
changes against the shared version, a separate units/loadouts review appears
before the mission form, with Added / Changed / Deleted lists and conflict review.
No pending changes means no units review, fork creation or upload.

The same submission engine handles both repositories. Each repository has its
own personal fork, reusable submission branch and one pending request. Confirming
units creates or updates its request; the mission form then shows that request
read-only and adds a Required units / loadouts link automatically to its PR body.
There is no manual Dependencies form. The units request and saved files are
rechecked before mission upload; a closed/conflicting or changed dependency stops
submission for another review. Merged units with no remaining changes need no
pending dependency link.

Cancelling the units step stops before mission upload. If units were uploaded and
the mission step is cancelled or fails, the units request remains saved and its
link is shown. Retrying reuses it. These two GitHub requests are not an atomic
transaction; reviewers still approve each separately and should merge units first.

## One contributor, one pending request

One personal fork is reused for all missions. The first online branch is
`gc-submissions`; an existing request from an earlier tool version keeps its
branch, locally and online. Starting from another local editing branch combines
its saved changes with the existing submission and switches back to that same
submission branch. Other user-created branches are preserved. Lookup uses the
contributor's fork and target main, not their current local branch.

Additional missions update the same pending request. A closed, unmerged request
shows Resubmit in review; confirming attempts to reopen that same request. If
GitHub refuses, the hub shows Request closed with Retry and View submission available and reports
that it could not reopen. It does not create a duplicate as a fallback. A merged
request is complete; subsequent changes create the next request using the same
fork and online branch. Multiple pre-existing open requests stop with an explicit
request for a reviewer to consolidate them; the tool never guesses which to close.

Request state is rechecked around upload and after the API update. A request
closed or merged during the operation is never reported as awaiting approval
when the final check observes that state. External changes after that check are
picked up on the next submission; the displayed result is not a live GitHub feed.

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

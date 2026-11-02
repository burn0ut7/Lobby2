# Debrief navigation validation — 2026-11-02

User order: All, factions, Private, Other players. All is the opening overview;
Players / AI continues to separate those populations. [Live capture](all-final.png).

All player totals are built once from each sealed player record, with ammunition
from its assignments. Participants are unique identities, including records
without assignments. All AI totals aggregate the bounded transient faction
counters. Both use existing report delivery limits and the client page cache.

The reported sidebar click failure reproduced through native hit-testing:
clicking the text resolved Cell1, not a button (rceb723ac4de34f1788289d90053e26b4).
Sidebar labels and their decorative ancestors now ignore cursor input. Table
cell tooltips elsewhere remain unchanged.

Reload e989ea949d5040089b0ec8c263c26144 succeeded, with only the existing three
Arland resource GUID errors. Native pointer hit-testing followed by the resolved
button's normal handler activated each sidebar entry:

- All: rf5a7263c044c47ef8ddeb0fdacc38d82
- Soviet Army: r4f50349f3e3b465781ba2430682ffa9d
- US Army: rb837d10cb27342abb64648f2902a6c4e
- Private: rc9321adc9f6943008dcf5e166ad7434d
- Other players: rb79425f50b5d4e6cb701b1dfee89738c; resulting real player row:
  r77fa14f297db4c7c83be73d96ff6677e.

Header Lobby and Debrief buttons also passed pointer hit-testing and navigation:
rb82c3251c4174a91934c5e83684e53fe / rdd21199a9a374ffdaf0c148fc8140890.
The menu actually exited and reopened to All (rb0ef1292678e45ffb95ef56e4d9a2710).
All AI empty state loaded successfully (re840dba20cf6489487a61f4ba0fd8515).

The isolated 128-player contract fixture passed (r52d275500c3b4ac5ae9c58bc05ccf911):
All totals were 158024576 shots, 8128 kills and 128 participants; directory still
paged into eight bounded responses. The original batch/reports were restored.
Build took 8 ms, complete fixture 11 ms on this host; not a network-load test.

Physical OS clicks, gamepad and a real 128-client session were not exercised.

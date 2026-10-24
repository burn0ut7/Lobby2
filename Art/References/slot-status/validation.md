# Slot icon validation — 2026-10-24

Active screenshots: runtime-gallery-v2.png (display-only cases) and
runtime-occupied-locked.png (actual occupied and admin-locked slot).
The earlier runtime-gallery.png is rejected/superseded: it used two status icons
for combined states. User requested exactly one status icon per slot.

Native reload 76a0b442ea0b4ed5a3f668e0d0533c4d succeeded on Workbench 1.8.0.13.
No new compiler error; existing BlockListApi leak and test-world/resource errors
remain documented in ../faction-ratio/validation.md. No clean diagnostic claim.

The eight-case presentation gallery exercised the production widget with detached
slot copies: locked, KIA, reserved, reserved+locked, KIA+locked, disconnected,
occupied+locked, free. Each nonfree row has one icon; free row has none. Names
remain visible without prefixes/suffixes. Skull takes precedence over admin lock;
otherwise admin lock replaces the person/reservation/disconnected glyph.

Receipt r0b5a12f575994a008b6b20024e6e1ece records final cases. Checks before and
after restoring authoritative bindings pass unchanged for all42 slots
(r5ec81b5415d543de993bd32909b1f45a and r04af8f345cb742edb36bb9aef8eed358).
Actual production slot request accepted slot1, then the bulk button locked all42;
claim-preservation check r38bda798db4c4d9c971cdea4b3326b04 passed. Snapshot
r490fb03716e346de9a4939e66bd7d071 and runtime-occupied-locked.png show one
padlock beside Burn0ut7. No person icon alongside it.

Full call receipts: ../faction-ratio/runtime/runtime-results.json. Source review
and git diff --check are separate from runtime checks. Gallery does not verify
real death/disconnect/reservation transitions or multiplayer replication. Physical
keyboard/controller, alternate UI scales and long-name coverage remain untested.

# Voice status indicators

Narrow extension of the existing shared voice/player row. Preserve the composition
shown in ../slot-status/runtime-presence.png and the existing speaking/deafen
indicators and controls. No new input handlers or gameplay state.

| Element | Player question | Source |
| --- | --- | --- |
| microphone | Is this player designated a priority speaker? | Replicated voice assignment |
| lockedCircle | Has an admin muted this player? | Replicated voice assignment |
| Existing speaker/disabled icon | Existing activity and personal hearing state | Existing presentation |
| gameMechanics on slot rows | Is this role in a lottery? | Existing lottery state |

Two 26-unit white status icons sit immediately before the existing speech icon,
with four-unit spacing; hidden icons collapse through the native horizontal
layout. Both remain independently visible if both flags are set. Icons ignore
cursor so they do not intercept the existing row controls. No control layout or
mute behavior changes. Base-game wrapper atlas keys were verified through MCP
in the prior slot-presence session, revision 9b2fea665c0943b9bcf87ec2891bc011.

Acceptance: priority-only, admin-muted-only, both, cleared, and personal deafen
must remain distinguishable in Voice and Players; updates must use production
admin requests and the existing assignment events. Check the actual rendered row
and lottery status. Physical input/audio routing need not be changed or inferred
from these display checks.

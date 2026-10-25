# Independent review

Reviewer: /root/ao_enrollment_review, fresh context.

Initial finding: occupied-character enrollment used the seat position, while enforcement used vehicle origin. Corrected ObserveCharacter to use the same mover origin as enforcement, retaining the character enrollment key. Verified the seat/body boundary case through MCP; receipts and validation.md record results.

Final review: Approved. Enrollment and enforcement use the same mover position. Recorded test confirms exemption with driver inside but vehicle outside, enrollment when vehicle enters, and warning after exit. No further actionable findings. Embedded-only and untested freeze/network coverage remain documented.

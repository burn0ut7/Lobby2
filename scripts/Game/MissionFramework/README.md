# Mission framework source

Start with the authored definitions, then follow their compiled forms into the runtime.

| Folder | Responsibility |
| --- | --- |
| `Events` | Mission events, random event groups and event options. |
| `Conditions` | Authored checks and their settings, including area targets and capture requirements. |
| `Actions` | Authored operations and their settings, including waypoint orders and respawn destinations. |
| `Flags` | Remembered mission flags and their options. Flag checks belong in `Conditions`; flag actions belong in `Actions`. |
| `Areas` | Passive polygon and radius entities placed in the world. Their authored checks live in `Conditions`. |
| `Interactions` | World interaction component and its native player use action. This is distinct from a mission event action. |
| `Runtime` | Compiled rules, evaluation, shared observations/trackers and execution support. These are not authoring choices. |

`GC_MissionScript.c` is the shared extension contract used by both custom script conditions and actions.

## Finding the execution path

- `Events/GC_Event.c` describes events and random groups.
- `Conditions/GC_ConditionBase.c` and `Actions/GC_ActionBase.c` define authored contracts.
- `Runtime/GC_CompiledMissionRule.c` and `Runtime/GC_RuleCondition.c` hold compiled representations.
- `Runtime/GC_MissionRuleEngine.c` evaluates and dispatches them.
- `Runtime/GC_FrameworkModeActions.c` implements manager-owned execution.

Some files contain several related types: `Actions/GC_FrameworkActions.c` holds flag, event-control, message, random-reset, end-mission and respawn-ticket actions; `Conditions/GC_NumberConditions.c` holds count comparisons. Search by class name when following a serialized type.

Place new code with its responsibility. Keep authored settings beside their condition/action and shared execution state in `Runtime`; avoid a generic helpers folder. Folder changes do not rename serialized classes, fields or callback contracts. Keep tests and development evidence outside this release addon.

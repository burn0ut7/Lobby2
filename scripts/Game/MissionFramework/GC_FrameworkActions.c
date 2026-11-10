//! Related operations share one action. Existing serialized action classes remain readable.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag Action")]
class GC_FlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Exact name in Mission Flags.")]
	string m_sFlagName;
	[Attribute("0", UIWidgets.ComboBox, "Call checks the flag's conditions and repeatability. All other operations edit its remembered count directly. Zero means false; a positive count means true. Reset does not reset mission events.", enums: {ParamEnum("Call", "0"), ParamEnum("Set count", "1"), ParamEnum("Add", "2"), ParamEnum("Subtract", "3"), ParamEnum("Reset to zero", "4"), ParamEnum("Restore starting count", "5")})]
	int m_iOperation;
	[Attribute("1", UIWidgets.SpinBox, "Amount for Set count, Add or Subtract only. Call always adds one when accepted. Subtract stops at zero. Reset and Restore use no amount.", "0 1000000 1")]
	int m_iAmount;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.FLAG_CONTROL;
		action.m_sTextOrName = m_sFlagName;
		action.m_iOperation = m_iOperation;
		action.m_iAmount = m_iAmount;
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Event Control Action")]
class GC_EventControlAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Unique mission event or random event group name. A child event still belongs to its group: these controls never select or run a child directly.")]
	string m_sEventName;
	[Attribute("0", UIWidgets.ComboBox, "Enable allows future checks. Disable cancels pending actions without erasing whether the event ran. Reset clears its run history and pending delay, but leaves enabled/disabled unchanged. Check now tests conditions, chance and delay normally. Resetting a group does not reset its children's run history. Flags and random rolls are unchanged.", enums: {ParamEnum("Enable", "0"), ParamEnum("Disable", "1"), ParamEnum("Reset", "2"), ParamEnum("Check now", "3")})]
	int m_iOperation;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.EVENT_CONTROL;
		action.m_sTextOrName = m_sEventName;
		action.m_iOperation = m_iOperation;
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission Message Action")]
class GC_MissionMessageAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Message shown to connected players using the same notification stack as admin messages. Maximum 512 characters. Messages wait their turn when the stack is full; joining later does not replay them.")]
	string m_sMessage;
	[Attribute("", UIWidgets.EditBox, "Faction keys receiving the message. Empty means everyone, including spectators. Multiple matching factions never send a player duplicate messages.")]
	ref array<string> m_aFactions;
	[Attribute("0", UIWidgets.CheckBox, "Checked: show a Mission Warning with the warning icon and sound. Unchecked: show a normal Mission message.")]
	bool m_bWarning;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.MISSION_MESSAGE;
		action.m_sTextOrName = m_sMessage;
		action.m_sTextOrName.TrimInPlace();
		action.m_bFlagAnswer = m_bWarning;
		action.m_aFlagNames = {};
		if (m_aFactions)
			action.m_aFlagNames.Copy(m_aFactions);
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Reset Random Chance Action")]
class GC_ResetRandomChanceAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Unique Reset Name on a Random Chance Condition. Clears its answer to false and waits its full Check Interval before rolling again. Also allows Roll Once to roll again. Does not reset flags, events or random groups.")]
	string m_sResetName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RESET_RANDOM;
		action.m_sTextOrName = m_sResetName;
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("End Mission Action")]
class GC_EndMissionAction : GC_MissionAction
{
	[Attribute("0", UIWidgets.ComboBox, "Ends the active Match and opens the after-action phase. Has no effect before Match or after the mission has ended.", enums: {ParamEnum("Mission complete", "0"), ParamEnum("Faction victory", "1"), ParamEnum("Faction defeat", "2"), ParamEnum("Faction draw", "3")})]
	int m_iResult;
	[Attribute("", UIWidgets.EditBox, "Faction key for a faction result. Leave empty for Mission complete.")]
	string m_sFaction;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.END_MISSION;
		action.m_iOperation = m_iResult;
		action.m_sTextOrName = m_sFaction;
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Respawn Tickets Action")]
class GC_RespawnTicketsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Faction key whose remaining respawn tickets will change. This action does not respawn anyone.")]
	string m_sFaction;
	[Attribute("0", UIWidgets.ComboBox, "Set replaces the remaining tickets. Add increases them. Subtract removes tickets, stopping at zero.", enums: {ParamEnum("Set", "0"), ParamEnum("Add", "1"), ParamEnum("Subtract", "2")})]
	int m_iOperation;
	[Attribute("1", UIWidgets.SpinBox, "Number of tickets to set, add or subtract.", "0 1000000 1")]
	int m_iAmount;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RESPAWN_TICKETS;
		action.m_sTextOrName = m_sFaction;
		action.m_iOperation = m_iOperation;
		action.m_iAmount = m_iAmount;
		return action;
	}
}

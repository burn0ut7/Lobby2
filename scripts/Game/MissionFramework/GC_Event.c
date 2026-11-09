[BaseContainerProps(), BaseContainerCustomStringTitleField("Advanced event settings")]
//! Attribute defaults apply to Workbench deserialization. Script fixtures must set authored fields explicitly.
class GC_RuleOptions
{
	[Attribute("0", UIWidgets.ComboBox, "Once is enough for most events. Every time allows later matching state changes, not repeated execution while a state stays the same.", enums: {ParamEnum("Once", "0"), ParamEnum("Every time", "1")})]
	int m_iRun;
	[Attribute("0", UIWidgets.SpinBox, "Wait this many seconds before running. Zero means now. Conditions must still pass when the wait ends.", "0 86400 0.1")]
	float m_fWaitSeconds;
	[Attribute("1", UIWidgets.SpinBox, "Chance to run this event's actions, from 0 to 1. For example, 0.8 is 80%, 1 is 100%, and 0 never runs. The default is 1; leaving this setting or Advanced unset means 100%. This does not spawn anything unless the actions do so.", "0 1 0.01")]
	float m_fRandomChance;
	[Attribute("0", UIWidgets.CheckBox, "Keep this event in the list without running it.")]
	bool m_bDisabled;
}

//! Shared serialized list type. Incomplete entries fail closed.
[BaseContainerProps(visible: false, insertable: false)]
class GC_EventDefinition
{
	//------------------------------------------------------------------------------------------------
	GC_CompiledMissionRule BuildRule()
	{
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_CompiledMissionRule BuildSettings(string name, array<ref GC_MissionCondition> conditions, GC_RuleOptions advanced)
	{
		GC_CompiledMissionRule rule = new GC_CompiledMissionRule();
		rule.m_sName = name;
		rule.m_bEnabled = true;
		rule.m_bRunOnlyOnce = true;
		rule.m_fEverySeconds = 5;
		rule.m_aThen = {};
		if (advanced)
		{
			if (advanced.m_iRun < 0 || advanced.m_iRun > 1 || advanced.m_fRandomChance < 0 || advanced.m_fRandomChance > 1)
				return null;
			rule.m_bEnabled = !advanced.m_bDisabled;
			rule.m_bRunOnlyOnce = advanced.m_iRun == 0;
			rule.m_fWaitSeconds = advanced.m_fWaitSeconds;
			rule.m_fChance = advanced.m_fRandomChance;
		}
		if (conditions && !conditions.IsEmpty())
		{
			rule.m_If = GC_MissionCondition.BuildGroup(conditions, GC_ERuleCheck.ALL, 0, 0);
			if (!rule.m_If)
				return null;
			if (rule.m_If.UsesGameModeState())
				rule.m_eWhen = GC_ERuleEvent.GAME_MODE_STATE;
		}
		return rule;
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_Event : GC_EventDefinition
{
	[Attribute("Event", UIWidgets.EditBox, "A short name for this event.")]
	string m_sName;
	[Attribute("", UIWidgets.Object, "No conditions means always true: there are no requirements to meet. Otherwise, every listed condition must pass. A top-level event with no conditions runs at match start. Inside a random group, it is eligible whenever the group selects events.")]
	ref array<ref GC_MissionCondition> m_aConditions;
	[Attribute("", UIWidgets.Object, "Actions to run, in list order.")]
	ref array<ref GC_MissionAction> m_aActions;
	[Attribute("", UIWidgets.Object, "Optional repeat, delay, random chance and disable settings. Unset means once, no delay, 100% chance.")]
	ref GC_RuleOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	override GC_CompiledMissionRule BuildRule()
	{
		GC_CompiledMissionRule rule = BuildSettings(m_sName, m_aConditions, m_Advanced);
		if (!rule || !m_aActions || m_aActions.IsEmpty() || m_aActions.Count() > 256)
			return null;
		foreach (GC_MissionAction definition : m_aActions)
		{
			if (!definition)
				return null;
			GC_RuleAction action = definition.BuildAction();
			if (!action || !action.IsValid())
				return null;
			rule.m_aThen.Insert(action);
		}
		if (!rule.IsValid())
			return null;
		return rule;
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_RandomEventGroup : GC_EventDefinition
{
	[Attribute("Random event group", UIWidgets.EditBox, "A short name for this random selection.")]
	string m_sName;
	[Attribute("", UIWidgets.Object, "No conditions means always true: there are no requirements to meet, and the group selects at match start. Otherwise, every listed condition must pass. Child conditions filter the selection; they do not trigger the group themselves.")]
	ref array<ref GC_MissionCondition> m_aConditions;
	[Attribute("1", UIWidgets.SpinBox, "How many different events to pick from those whose conditions pass. No repeats in one selection. If fewer qualify, pick those available. Each selected event's own chance and delay still apply.", "1 255 1")]
	int m_iPickCount;
	[Attribute("", UIWidgets.Object, "Regular events to choose from. Disabled, already-completed once-only events, waiting events and events with failing conditions are excluded. Groups cannot be nested here; conditions can still nest normally.")]
	ref array<ref GC_Event> m_aEvents;
	[Attribute("", UIWidgets.Object, "Optional repeat, delay, chance and disable settings for the whole selection. Child events keep their own Advanced settings.")]
	ref GC_RuleOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	override GC_CompiledMissionRule BuildRule()
	{
		GC_CompiledMissionRule rule = BuildSettings(m_sName, m_aConditions, m_Advanced);
		if (!rule || !m_aEvents || m_aEvents.IsEmpty() || m_aEvents.Count() > 255 || m_iPickCount < 1 || m_iPickCount > m_aEvents.Count())
			return null;
		rule.m_iPickCount = m_iPickCount;
		rule.m_aChoices = {};
		foreach (GC_Event childEvent : m_aEvents)
		{
			if (!childEvent)
				return null;
			GC_CompiledMissionRule child = childEvent.BuildRule();
			if (!child)
				return null;
			rule.m_aChoices.Insert(child);
		}
		if (!rule.IsValid())
			return null;
		return rule;
	}
}

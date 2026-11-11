//! Shared serialized list type. Incomplete entries fail closed.
[BaseContainerProps(visible: false, insertable: false)]
class GC_EventDefinition
{
	//------------------------------------------------------------------------------------------------
	//! Keep names even when an authored definition cannot compile.
	void CollectEventNames(array<string> names)
	{
	}

	//------------------------------------------------------------------------------------------------
	GC_CompiledMissionRule BuildRule()
	{
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_CompiledMissionRule BuildSettings(string name, array<ref GC_ConditionBase> conditions, GC_EventOptions options)
	{
		bool repeatable;
		float delay;
		float chance = 1;
		bool disabled;
		if (options)
		{
			repeatable = options.m_bRepeatable;
			delay = options.m_fDelay;
			chance = options.m_fRandomChance;
			disabled = options.m_bDisabled;
		}
		GC_CompiledMissionRule rule = new GC_CompiledMissionRule();
		rule.m_sName = name;
		rule.m_bEnabled = true;
		rule.m_bRunOnlyOnce = true;
		rule.m_fEverySeconds = 5;
		rule.m_aThen = {};
		if (chance < 0 || chance > 1)
			return null;
		rule.m_bEnabled = !disabled;
		rule.m_bRunOnlyOnce = !repeatable;
		rule.m_fWaitSeconds = delay;
		rule.m_fChance = chance;

		if (conditions && !conditions.IsEmpty())
		{
			rule.m_If = GC_ConditionBase.BuildGroup(conditions, GC_ERuleCheck.ALL, 0, 0);
			if (!rule.m_If)
				return null;
			rule.m_bObserveFlags = rule.m_If.UsesFlag();
			rule.m_bObserveEvents = rule.m_If.UsesEvent();
			if (rule.m_If.UsesGameModeState() || rule.m_bObserveFlags || rule.m_bObserveEvents)
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
	ref array<ref GC_ConditionBase> m_aConditions;
	[Attribute("", UIWidgets.Object, "Actions to run, in list order.")]
	ref array<ref GC_ActionBase> m_aActions;

	[Attribute("", UIWidgets.Object, "Optional advanced settings. Leave empty to run once, with no delay, 100% chance and enabled. Expand this object only when you need to change those defaults.")]
	ref GC_EventOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	override void CollectEventNames(array<string> names)
	{
		if (!m_sName.IsEmpty())
			names.Insert(m_sName);
	}

	//------------------------------------------------------------------------------------------------
	override GC_CompiledMissionRule BuildRule()
	{
		GC_CompiledMissionRule rule = BuildSettings(m_sName, m_aConditions, m_Advanced);
		if (!rule || !m_aActions || m_aActions.IsEmpty() || m_aActions.Count() > 256)
			return null;
		foreach (GC_ActionBase definition : m_aActions)
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
	ref array<ref GC_ConditionBase> m_aConditions;
	[Attribute("1", UIWidgets.SpinBox, "How many different events to pick from those whose conditions pass. No repeats in one selection. If fewer qualify, pick those available. Each selected event's own chance and delay still apply.", "1 255 1")]
	int m_iPickCount;
	[Attribute("", UIWidgets.Object, "Regular events to choose from. Disabled, already-completed once-only events, waiting events and events with failing conditions are excluded. Groups cannot be nested here; conditions can still nest normally.")]
	ref array<ref GC_Event> m_aEvents;

	[Attribute("", UIWidgets.Object, "Optional advanced settings. Leave empty to run once, with no delay, 100% chance and enabled. Expand this object only when you need to change those defaults.")]
	ref GC_EventOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	override void CollectEventNames(array<string> names)
	{
		if (!m_sName.IsEmpty())
			names.Insert(m_sName);
		if (!m_aEvents)
			return;
		foreach (GC_Event child : m_aEvents)
		{
			if (child)
				child.CollectEventNames(names);
		}
	}

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

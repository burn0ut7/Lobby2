enum GC_EMissionEventOperation
{
	RUN,
	REARM,
	DISABLE
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Flag")]
class GC_SetFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Flag name. Created automatically when this action writes it; no separate declaration needed.")]
	string m_sFlag;
	[Attribute("1", UIWidgets.CheckBox, "On sets the flag. Off clears it.")]
	bool m_bSet;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sFlag.IsEmpty())
			return true;
		error = "Set Flag: enter a flag name. " + m_sFlag;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.SetFlag(m_sFlag, m_bSet);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Randomize Flag (%)")]
class GC_RandomFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Flag name. Created automatically when this action writes it; no separate declaration needed.")]
	string m_sFlag;
	[Attribute("50", UIWidgets.EditBox, "Chance to set the flag. 0 never sets it; 100 always sets it.", params: "0 100 1")]
	float m_fChance;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_fChance < 0 || m_fChance > 100)
		{
			error = "Randomize Flag: chance must be from 0 to 100.";
			return false;
		}
		if (m_sFlag.IsEmpty())
		{
			error = "Randomize Flag: enter a flag name. " + m_sFlag;
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		bool selected = m_fChance >= 100 || (m_fChance > 0 && Math.RandomFloat01() < m_fChance * 0.01);
		return context.SetFlag(m_sFlag, selected);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Choose Random Flags")]
class GC_ChooseFlagsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Pool of unique flag names. Created automatically when written; no separate declarations needed.")]
	ref array<string> m_aFlags;
	[Attribute("1", UIWidgets.EditBox, "Number to choose.", params: "1 inf 1")]
	int m_iNumber;
	[Attribute("0", UIWidgets.CheckBox, "Allow a name to be drawn more than once, which can set fewer different flags.", category: "Advanced")]
	bool m_bAllowRepeatedDraws;
	[Attribute("0", UIWidgets.CheckBox, "Clear the pool first. Off leaves previously set flags unchanged.")]
	bool m_bClearFirst;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aFlags || m_aFlags.IsEmpty() || m_iNumber < 1 || (!m_bAllowRepeatedDraws && m_iNumber > m_aFlags.Count()))
		{
			error = "Choose Random Flags: add a pool and a valid draw count.";
			return false;
		}
		set<string> unique = new set<string>();
		foreach (string name : m_aFlags)
		{
			if (name.IsEmpty() || unique.Contains(name))
			{
				error = "Choose Random Flags: empty or duplicate flag " + name;
				return false;
			}
			unique.Insert(name);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		array<string> pool = {};
		pool.Copy(m_aFlags);
		bool applied = true;
		if (m_bClearFirst)
		{
			foreach (string name : pool)
				applied = context.SetFlag(name, false) && applied;
		}
		for (int i; i < m_iNumber; i++)
		{
			int index = Math.RandomInt(0, pool.Count());
			applied = context.SetFlag(pool[index], true) && applied;
			if (!m_bAllowRepeatedDraws)
				pool.RemoveOrdered(index);
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Control Events")]
class GC_ControlEventsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of events to control.")]
	ref array<string> m_aEvents;
	[Attribute("0", UIWidgets.ComboBox, "Run bypasses conditions on the next evaluation. Rearm allows another normal trigger. Disable prevents triggers and cancels queued actions.", enums: ParamEnumArray.FromEnum(GC_EMissionEventOperation))]
	GC_EMissionEventOperation m_eOperation;
	[Attribute("0", UIWidgets.CheckBox, "For Run: allow events that already ran. Disabled events must first be rearmed.")]
	bool m_bAllowRepeat;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aEvents || m_aEvents.IsEmpty() || m_eOperation < 0 || m_eOperation > 2)
		{
			error = "Control Events: add event names and choose an operation.";
			return false;
		}
		foreach (string name : m_aEvents)
		{
			if (!context.GetRuntime() || context.GetRuntime().FindEvent(name) < 0)
			{
				error = "Control Events: unknown event " + name;
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive() || !context.GetRuntime())
			return false;
		bool applied = true;
		foreach (string name : m_aEvents)
		{
			if (!context.GetRuntime().ControlEvent(name, m_eOperation, m_bAllowRepeat))
				applied = false;
		}
		return applied;
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Do not use")]
class GC_AreaTargetsBase
{
	//------------------------------------------------------------------------------------------------
	bool BuildTargets(GC_RuleCondition check)
	{
		return false;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Named Entities")]
class GC_NamedAreaTargets : GC_AreaTargetsBase
{
	[Attribute("", UIWidgets.EditBox, "Exact unique world names of any entity: vehicles, props, cargo, characters or mixed types. Not prefab paths or account names. Count and Proportion both support capture timing. One entry per target. Duplicate names count once. Every listed name must resolve; a missing object is not treated as Outside. Spawned replacements with the same name are resolved on a later check. Dead or destroyed named objects still count while they exist.")]
	ref array<string> m_aNames;

	//------------------------------------------------------------------------------------------------
	override bool BuildTargets(GC_RuleCondition check)
	{
		if (!m_aNames || m_aNames.IsEmpty() || m_aNames.Count() > 256)
			return false;
		check.m_aAreaNames = {};
		foreach (string name : m_aNames)
		{
			if (name.IsEmpty())
				return false;
			if (!check.m_aAreaNames.Contains(name))
				check.m_aAreaNames.Insert(name);
		}
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Players / AI")]
class GC_CharacterAreaTargets : GC_AreaTargetsBase
{
	[Attribute("0", UIWidgets.ComboBox, "Living characters only. Players means characters currently controlled by connected players. AI means registered AI characters, excluding playable slots. Both includes both groups once. Spectators, dead characters and empty playable slots never count. Respawns and new registered AI are included on the next check.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")})]
	int m_iWho;

	[Attribute("", UIWidgets.EditBox, "Optional faction keys, for example US or USSR. Empty means all factions. Selected factions are counted together. Uses the character's current faction; no separate faction condition is required.")]
	ref array<string> m_aFactions;

	//------------------------------------------------------------------------------------------------
	override bool BuildTargets(GC_RuleCondition check)
	{
		return GC_AreaPopulation.BuildSelection(check, m_iWho, m_aFactions);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Count")]
class GC_AreaCount : GC_LivingRequirementBase
{
	[Attribute("0", UIWidgets.ComboBox, "Compare the number of targets on the selected side. Greater than 0 means at least one. Greater than 4 means five or more. Equal 0 means none. An empty Players / AI group has count zero; missing named targets make the whole condition unavailable.", enums: {ParamEnum("Greater than", "0"), ParamEnum("Less than", "1"), ParamEnum("Equal", "2"), ParamEnum("Not equal", "3")})]
	int m_iComparison;

	[Attribute("0", UIWidgets.SpinBox, "Number of targets to compare against. Living Characters counts anywhere. Area Condition counts targets on its selected side; Outside does not mean all targets are outside.", "0 4096 1")]
	int m_iRequiredCount;

	//------------------------------------------------------------------------------------------------
	override bool BuildRequirement(GC_RuleCondition check)
	{
		check.m_iAreaComparison = m_iComparison;
		check.m_iAreaRequired = m_iRequiredCount;
		return true;
	}
}

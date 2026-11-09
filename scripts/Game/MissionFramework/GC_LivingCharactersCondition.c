[BaseContainerProps(), BaseContainerCustomStringTitleField("Living Characters")]
class GC_LivingCharactersCondition : GC_ConditionBase
{
	[Attribute("0", UIWidgets.ComboBox, "Count living characters: connected players, registered AI, or both. Spectators, dead characters and empty playable slots never count. Unconscious living characters count. Updated once per second during Match and on phase changes.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")})]
	int m_iWho;

	[Attribute("", UIWidgets.EditBox, "Optional faction keys, such as US or USSR. Empty means all factions. Selected factions are counted together, using each character's current faction.")]
	ref array<string> m_aFactions;

	[Attribute("", UIWidgets.Hidden, "Legacy area configuration. Move this check to Area Condition; legacy area-based Living Characters checks are rejected rather than silently counting everywhere.")]
	ref GC_LivingCharacterArea m_Area;

	[Attribute("", UIWidgets.Object, "Optional living-character count anywhere in the mission. New checks default to at least one. Earlier saved count settings apply until you choose Count. For Inside/Outside or extraction proportions, use Area Condition with Players / AI targets.")]
	ref GC_AreaCount m_Count;

	[Attribute("", UIWidgets.Hidden, "Legacy Requirement reader. Count remains readable; old Proportion must move to Area Condition.")]
	ref GC_LivingRequirementBase m_Requirement;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (m_Area || (m_Requirement && !GC_AreaCount.Cast(m_Requirement)))
		{
			Print("GC | Mission Framework | Move legacy Living Characters Area settings to Area Condition. This condition is disabled until migrated.", LogLevel.ERROR);
			return null;
		}
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 10;
		check.m_bAreaAnywhere = true;
		if (!GC_AreaPopulation.BuildSelection(check, m_iWho, m_aFactions))
			return null;
		GC_AreaCount count = m_Count;
		if (!count)
			count = GC_AreaCount.Cast(m_Requirement);
		if (count && !count.BuildRequirement(check))
			return null;
		if (!check.IsValid(depth))
			return null;
		return check;
	}

}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy area - migrate to Area Condition")]
class GC_LivingCharacterArea
{
	[Attribute("", UIWidgets.EditBox, "Exact unique world name of a Radius Area or closed Polygon Area. The area supplies only its shape. Missing or invalid areas cannot pass, even inside a None group.")]
	string m_sName;

	[Attribute("0", UIWidgets.ComboBox, "Which side contains the characters to count. Outside counts characters outside; it does not require everybody to be outside. Position checks use the character's origin.", enums: {ParamEnum("Inside", "0"), ParamEnum("Outside", "1")})]
	int m_iSide;
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Do not use")]
class GC_LivingRequirementBase
{
	//------------------------------------------------------------------------------------------------
	bool BuildRequirement(GC_RuleCondition check)
	{
		return false;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Proportion")]
class GC_LivingProportion : GC_LivingRequirementBase
{
	[Attribute("0.9", UIWidgets.Slider, "Minimum proportion on the selected side of Area: 0.9 means at least 90%, 1 means everyone, and 0 means any proportion of a nonempty population. The total includes all currently living characters matching Who and Factions, wherever they are. Deaths, respawns and faction changes update that total. No eligible living characters means false. Requires Area.", "0 1 0.01")]
	float m_fRequiredProportion;

	//------------------------------------------------------------------------------------------------
	override bool BuildRequirement(GC_RuleCondition check)
	{
		check.m_bAreaProportion = true;
		check.m_fAreaProportion = m_fRequiredProportion;
		return true;
	}
}

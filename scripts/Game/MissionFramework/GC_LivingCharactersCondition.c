[BaseContainerProps(), BaseContainerCustomStringTitleField("Living Characters")]
class GC_LivingCharactersCondition : GC_ConditionBase
{
	[Attribute("0", UIWidgets.ComboBox, "Count living characters: connected players, registered AI, or both. Spectators, dead characters and empty playable slots never count. Unconscious living characters count. Updated once per second during Match and on phase changes.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")})]
	int m_iWho;

	[Attribute("", UIWidgets.EditBox, "Optional faction keys, such as US or USSR. Empty means all factions. Selected factions are counted together, using each character's current faction.")]
	ref array<string> m_aFactions;

	[Attribute("", UIWidgets.Object, "Optional area filter. Leave unset to count living characters anywhere. Add Area to count Inside or Outside a named radius or polygon. This contains location settings only, not more conditions.")]
	ref GC_LivingCharacterArea m_Area;

	[Attribute("", UIWidgets.Object, "Choose Count or Proportion. Leave unset to require at least one matching living character. Proportion requires an Area and compares that side with all matching living characters everywhere. There are no child conditions here.")]
	ref GC_LivingRequirementBase m_Requirement;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 10;
		check.m_bAreaAnywhere = !m_Area;
		if (!BuildPopulation(check, m_iWho, m_aFactions))
			return null;
		if (m_Area)
		{
			if (m_Area.m_iSide < 0 || m_Area.m_iSide > 1)
				return null;
			check.m_sAreaName = m_Area.m_sName;
			check.m_bAreaOutside = m_Area.m_iSide == 1;
		}
		if (m_Requirement && !m_Requirement.BuildRequirement(check))
			return null;
		if (!check.IsValid(depth))
			return null;
		return check;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared with the hidden serialized Players / AI target from the previous prototype.
	static bool BuildPopulation(GC_RuleCondition check, int who, array<string> factions)
	{
		check.m_iAreaTargets = 1;
		check.m_iAreaWho = who;
		check.m_aAreaFactions = {};
		if (factions)
		{
			if (factions.Count() > 256)
				return false;
			foreach (string faction : factions)
			{
				if (faction.IsEmpty())
					return false;
				if (!check.m_aAreaFactions.Contains(faction))
					check.m_aAreaFactions.Insert(faction);
			}
		}
		return who >= 0 && who <= 2;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Area")]
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

[BaseContainerProps(), BaseContainerCustomStringTitleField("Local faction share")]
class GC_LocalFactionShare : GC_LivingRequirementBase
{
	[Attribute("1", UIWidgets.Slider, "Selected factions' share of living characters on the selected side of the area. 1 requires no other counted factions; 0.7 requires 70%. Players / AI targets must name at least one capturing faction. Multiple selected factions combine as one force.", "0 1 0.01")]
	float m_fRequiredShare;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: share must be at least the threshold. Checked: at most. An empty area fails At least and passes At most, matching TilW faction triggers.")]
	bool m_bAtMost;
	[Attribute("", UIWidgets.EditComboBox, "Factions excluded from both sides of the ratio, for example civilians. Unlisted factions compete regardless of their diplomatic relations. A capturing faction cannot also be ignored.", enums: {ParamEnum("", "", "gc-choices:1:GC_FactionChoices")})]
	ref array<string> m_aIgnoredFactions;

	//------------------------------------------------------------------------------------------------
	override bool BuildRequirement(GC_RuleCondition check)
	{
		check.m_bLocalFactionShare = true;
		check.m_fAreaProportion = m_fRequiredShare;
		check.m_bShareAtMost = m_bAtMost;
		check.m_aIgnoredFactions = {};
		if (m_aIgnoredFactions)
			check.m_aIgnoredFactions.Copy(m_aIgnoredFactions);
		return true;
	}
}

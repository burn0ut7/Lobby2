[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Condition")]
class GC_AreaCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.Object, "Choose Named Entities for any specific vehicles, props, cargo or other world entities, or Players / AI for living characters with optional faction filters. Capture timing works with either selection. These are target settings, not nested conditions.")]
	ref GC_AreaTargetsBase m_Targets;

	[Attribute("", UIWidgets.EditBox, "Exact unique name of a Radius Area or closed polygon in the world. Radius Area uses a 3D distance in metres. A polygon checks its horizontal outline and ignores height, like the AO limit. Configure the shape on that area, not here.")]
	string m_sArea;

	[Attribute("0", UIWidgets.CheckBox, "Checked: count targets outside the area. Unchecked: count targets inside. Requirement controls how many are needed. Checked once per second; brief crossings can be missed. Missing targets or invalid areas never pass.")]
	bool m_bOutside;

	[Attribute("", UIWidgets.Object, "Choose Count, Proportion for extraction, or Local faction share for competing factions. New checks default to at least one matching target on the selected side. Older saved Count settings remain in use until you choose a Requirement. Proportion 0.9 means at least 90% of eligible targets are on that side. Named Entities uses the deduplicated list; every name must resolve. Players / AI uses all eligible living characters. Local faction share is for Players / AI only.")]
	ref GC_LivingRequirementBase m_Requirement;

	[Attribute("", UIWidgets.Hidden, "Legacy Count. Used only when Requirement is unset, preserving older saved area checks.")]
	ref GC_AreaCount m_Count;

	[Attribute("", UIWidgets.Object, "Optional capture timing and status messages for any selected target type, including players, AI, vehicles and objects. Leave unset for the existing immediate area check. Available during Match only.")]
	ref GC_AreaCaptureOptions m_Capture;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition condition = new GC_RuleCondition();
		condition.m_iCheck = 10;
		condition.m_sAreaName = m_sArea;
		condition.m_bAreaOutside = m_bOutside;
		if (!m_Targets || !m_Targets.BuildTargets(condition))
			return null;
		GC_LivingRequirementBase requirement = m_Requirement;
		if (!requirement)
			requirement = m_Count;
		if (requirement && !requirement.BuildRequirement(condition))
			return null;
		if (m_Capture)
			condition.m_Capture = m_Capture.Copy();
		if (!condition.IsValid(depth))
			return null;
		return condition;
	}
}

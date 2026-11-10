[BaseContainerProps(), BaseContainerCustomStringTitleField("JIP Action")]
class GC_JipAction : GC_ActionBase
{
	[Attribute("0", UIWidgets.ComboBox, "Change join-in-progress teleport policy. Manual: eligible late joiners choose when to teleport. Automatic: new eligible offers teleport automatically. Disabled: deny JIP teleport requests. This does not block server connections, reset used eligibility, change faction restrictions or remove the mission's JIP cutoff. Existing offers are not sent again when switching modes.", enums: {ParamEnum("Manual", "0"), ParamEnum("Automatic", "1"), ParamEnum("Disabled", "2")})]
	GC_EJipMode m_eJipMode;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.JIP;
		action.m_iOperation = m_eJipMode;
		return action;
	}
}

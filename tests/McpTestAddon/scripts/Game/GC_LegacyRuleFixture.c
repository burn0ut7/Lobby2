[BaseContainerProps(insertable: false), BaseContainerCustomTitleField("m_sName")]
class GC_LegacyRuleFixture
{
	[Attribute("New rule", UIWidgets.EditBox, "Describe what this rule does, for example: Mark the test complete.")]
	string m_sName;
	[Attribute("1", UIWidgets.CheckBox, "Unchecked rules do nothing.")]
	bool m_bEnabled;
	[Attribute("0", UIWidgets.ComboBox, "When should this rule check its conditions? Match begins means after the lobby and briefing.", enums: {ParamEnum("Match started", "0"), ParamEnum("Every few seconds", "1"), ParamEnum("A named test event arrives", "2")}, category: "When")]
	GC_ERuleEvent m_eWhen;
	[Attribute("test", UIWidgets.EditBox, "For a named test event: use exactly the same name as the action that sends it.", category: "When")]
	string m_sTestEventName;
	[Attribute("5", UIWidgets.SpinBox, "For Every few seconds: wait this long between checks. First check is this long after match start.", "0.1 86400 0.1", category: "When")]
	float m_fEverySeconds;
	[Attribute("", UIWidgets.Object, "Add a check or group of checks. Leave empty to always pass.", category: "If")]
	ref GC_RuleCondition m_If;
	[Attribute("", UIWidgets.Object, "Actions run in list order after the conditions pass.", category: "Then")]
	ref array<ref GC_RuleAction> m_aThen;
	[Attribute("1", UIWidgets.CheckBox, "Checked: this rule can run only once. Unchecked: later matching events may run it again.", category: "Then")]
	bool m_bRunOnlyOnce;
	[Attribute("0", UIWidgets.SpinBox, "Wait this many seconds after the conditions pass. Conditions are checked again before actions run. Extra events while waiting are ignored.", "0 86400 0.1", category: "Then")]
	float m_fWaitSeconds;

	//------------------------------------------------------------------------------------------------
	//! Preserve saved rules from the previous editor format.
	GC_CompiledMissionRule BuildRule()
	{
		GC_CompiledMissionRule rule = new GC_CompiledMissionRule();
		rule.m_sName = m_sName;
		rule.m_bEnabled = m_bEnabled;
		rule.m_eWhen = m_eWhen;
		rule.m_sTestEventName = m_sTestEventName;
		rule.m_fEverySeconds = m_fEverySeconds;
		rule.m_If = m_If;
		rule.m_aThen = m_aThen;
		rule.m_bRunOnlyOnce = m_bRunOnlyOnce;
		rule.m_fWaitSeconds = m_fWaitSeconds;
		return rule;
	}

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		if (m_eWhen < 0 || m_eWhen > GC_ERuleEvent.FLAG_OFF || m_fWaitSeconds < 0 || m_fWaitSeconds > 86400)
			return false;
		if (m_eWhen == GC_ERuleEvent.TIMER && (m_fEverySeconds < 0.1 || m_fEverySeconds > 86400))
			return false;
		if (m_eWhen >= GC_ERuleEvent.TEST_EVENT && m_sTestEventName.IsEmpty())
			return false;
		if (m_If && !m_If.IsValid())
			return false;
		if (!m_aThen || m_aThen.IsEmpty() || m_aThen.Count() > 256)
			return false;
		foreach (GC_RuleAction action : m_aThen)
		{
			if (!action || !action.IsValid())
				return false;
		}
		return true;
	}
}

class GC_LegacyRuleFixtureComponentClass : ScriptComponentClass
{
}

class GC_LegacyRuleFixtureComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Object)]
	ref array<ref GC_LegacyRuleFixture> m_aLegacyRuleFixtures;
}

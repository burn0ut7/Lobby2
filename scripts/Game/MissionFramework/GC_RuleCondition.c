enum GC_ERuleCheck
{
	ALL,
	ANY,
	NONE,
	EXACTLY,
	AT_LEAST,
	AT_MOST,
	ALL_THE_SAME,
	NOT_ALL,
	ODD_TRUE,
	EVEN_TRUE
}

//! Plain data owned by one mission; evaluation never changes flags.
[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_RuleCondition
{
	[Attribute("Check", UIWidgets.EditBox, "A short name to help you find this check.")]
	string m_sName;

	[Attribute("0", UIWidgets.ComboBox, "Choose a group of checks, a named yes/no flag, or a fixed test answer.", enums: {ParamEnum("Group of checks", "0"), ParamEnum("Flag is yes", "1"), ParamEnum("Test answer", "2")})]
	int m_iCheck;

	[Attribute("0", UIWidgets.CheckBox, "Turn a passing answer into a failing answer, and a failing answer into a passing answer.")]
	bool m_bReverseAnswer;

	[Attribute("", UIWidgets.EditBox, "For Flag is yes: the exact flag name. Missing flags are no.")]
	string m_sFlagName;

	[Attribute("1", UIWidgets.CheckBox, "For Test answer: checked means yes; unchecked means no.")]
	bool m_bTestAnswer;

	[Attribute("0", UIWidgets.ComboBox, "For a group: choose how many checks must pass. All the same means all pass or all fail.", enums: {ParamEnum("All pass", "0"), ParamEnum("Any pass", "1"), ParamEnum("None pass", "2"), ParamEnum("Exactly this many pass", "3"), ParamEnum("At least this many pass", "4"), ParamEnum("At most this many pass", "5"), ParamEnum("All give the same answer", "6")})]
	GC_ERuleCheck m_eGroupNeeds;

	[Attribute("1", UIWidgets.SpinBox, "Used by Exactly, At least, and At most. Zero is allowed.", "0 256 1")]
	int m_iHowMany;

	[Attribute("", UIWidgets.Object, "Checks inside this group. A check may itself be another group. An empty All group passes.")]
	ref array<ref GC_RuleCondition> m_aChecks;

	//------------------------------------------------------------------------------------------------
	//! Validate before inversion, so malformed data can never become a passing check.
	bool IsValid(int depth = 0)
	{
		if (depth > 16 || m_iCheck < 0 || m_iCheck > 2)
			return false;
		if (m_iCheck == 1)
			return !m_sFlagName.IsEmpty();
		if (m_iCheck == 2)
			return true;
		if (m_eGroupNeeds < 0 || m_eGroupNeeds > GC_ERuleCheck.EVEN_TRUE || m_iHowMany < 0)
			return false;
		if (!m_aChecks)
			return true;
		if (m_aChecks.Count() > 256)
			return false;
		foreach (GC_RuleCondition check : m_aChecks)
		{
			if (!check || !check.IsValid(depth + 1))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Evaluate(map<string, bool> flags)
	{
		if (!IsValid())
			return false;
		return EvaluateValid(flags);
	}

	//------------------------------------------------------------------------------------------------
	protected bool EvaluateValid(map<string, bool> flags)
	{
		bool answer;
		if (m_iCheck == 1)
		{
			if (flags)
				flags.Find(m_sFlagName, answer);
		}
		else if (m_iCheck == 2)
			answer = m_bTestAnswer;
		else
		{
			int passed;
			int count;
			if (m_aChecks)
			{
				count = m_aChecks.Count();
				foreach (GC_RuleCondition check : m_aChecks)
				{
					if (check.EvaluateValid(flags))
						passed++;
				}
			}
			switch (m_eGroupNeeds)
			{
				case GC_ERuleCheck.ALL:
					answer = passed == count;
					break;
				case GC_ERuleCheck.ANY:
					answer = passed > 0;
					break;
				case GC_ERuleCheck.NONE:
					answer = passed == 0;
					break;
				case GC_ERuleCheck.EXACTLY:
					answer = passed == m_iHowMany;
					break;
				case GC_ERuleCheck.AT_LEAST:
					answer = passed >= m_iHowMany;
					break;
				case GC_ERuleCheck.AT_MOST:
					answer = passed <= m_iHowMany;
					break;
				case GC_ERuleCheck.ALL_THE_SAME:
					answer = passed == 0 || passed == count;
					break;
				case GC_ERuleCheck.NOT_ALL:
					answer = passed < count;
					break;
				case GC_ERuleCheck.ODD_TRUE:
					answer = (passed % 2) == 1;
					break;
				case GC_ERuleCheck.EVEN_TRUE:
					answer = (passed % 2) == 0;
					break;
			}
		}
		if (m_bReverseAnswer)
			return !answer;
		return answer;
	}
}

//! Fixed numeric inputs for the logic foundation. Live game-value sources are a future slice.
class GC_NumberConditionValue
{
	//------------------------------------------------------------------------------------------------
	//! Positive comparisons reject non-finite values as well as numbers outside the editor range.
	static bool IsValid(float number)
	{
		return number >= -1000000000 && number <= 1000000000;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Equal to (=)")]
class GC_EqualCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must equal Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber == m_fSecondNumber;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Not equal to (!=)")]
class GC_NotEqualCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must differ from Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber != m_fSecondNumber;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Less than (<)")]
class GC_LessThanCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must be smaller than Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber < m_fSecondNumber;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Less than or equal (<=)")]
class GC_LessOrEqualCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must be smaller than or equal to Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber <= m_fSecondNumber;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Greater than (>)")]
class GC_GreaterThanCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must be larger than Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber > m_fSecondNumber;
		return check;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Greater than or equal (>=)")]
class GC_GreaterOrEqualCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.SpinBox, "First number must be larger than or equal to Second number. Fixed number; decimals allowed. Range: -1 billion to 1 billion.", "-1000000000 1000000000 0.1")]
	float m_fFirstNumber;
	[Attribute("0", UIWidgets.SpinBox, "The fixed number to compare against. This does not read a game value.", "-1000000000 1000000000 0.1")]
	float m_fSecondNumber;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || !GC_NumberConditionValue.IsValid(m_fFirstNumber) || !GC_NumberConditionValue.IsValid(m_fSecondNumber))
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = m_fFirstNumber >= m_fSecondNumber;
		return check;
	}
}

modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunStandardLogic()
	{
		GC_AllConditions all = new GC_AllConditions();
		GC_AnyCondition any = new GC_AnyCondition();
		GC_NoneConditions none = new GC_NoneConditions();
		GC_NotAllConditions nand = new GC_NotAllConditions();
		GC_XorConditions xorCondition = new GC_XorConditions();
		GC_XnorConditions xnorCondition = new GC_XnorConditions();
		array<ref GC_MissionCondition> operators = {all, any, none, nand, xorCondition, xnorCondition};
		array<ref GC_MissionCondition> children = {};
		all.m_aConditions = children;
		any.m_aConditions = children;
		none.m_aConditions = children;
		nand.m_aConditions = children;
		xorCondition.m_aConditions = children;
		xnorCondition.m_aConditions = children;
		foreach (GC_MissionCondition operation : operators)
			Check(!operation.BuildCheck(), "empty explicit operator fails closed");
		// Independent truth-table bitsets, including parity cases with three inputs.
		for (int inputs = 1; inputs <= 3; inputs++)
		{
			array<int> expected = {2, 2, 1, 1, 2, 1};
			if (inputs == 2)
				expected = {8, 14, 1, 7, 6, 9};
			if (inputs == 3)
				expected = {128, 254, 1, 127, 150, 105};
			for (int mask; mask < (1 << inputs); mask++)
			{
				children.Clear();
				for (int bit; bit < inputs; bit++)
				{
					GC_BooleanCondition value = new GC_BooleanCondition();
					value.m_bValue = (mask & (1 << bit)) != 0;
					children.Insert(value);
				}
				foreach (int index, GC_MissionCondition operation : operators)
				{
					GC_RuleCondition built = operation.BuildCheck();
					Check(built && built.Evaluate(null) == ((expected[index] & (1 << mask)) != 0), "standard Boolean truth table");
				}
			}
		}
		children.Clear();
		children.Insert(null);
		foreach (GC_MissionCondition operation : operators)
			Check(!operation.BuildCheck(), "missing child cannot pass even under negation");
		children.Clear();
		children.Insert(none);
		Check(!none.BuildCheck(), "None cycle bounded");
		children.Clear();

		GC_EqualCondition equal = new GC_EqualCondition();
		GC_NotEqualCondition different = new GC_NotEqualCondition();
		GC_LessThanCondition less = new GC_LessThanCondition();
		GC_LessOrEqualCondition lessEqual = new GC_LessOrEqualCondition();
		GC_GreaterThanCondition greater = new GC_GreaterThanCondition();
		GC_GreaterOrEqualCondition greaterEqual = new GC_GreaterOrEqualCondition();
		array<ref GC_MissionCondition> comparisons = {equal, different, less, lessEqual, greater, greaterEqual};
		array<float> firstNumbers = {-2.5, 3, 4.5};
		array<int> comparisonExpected = {2, 5, 1, 3, 4, 6};
		foreach (int row, float first : firstNumbers)
		{
			equal.m_fFirstNumber = first;
			different.m_fFirstNumber = first;
			less.m_fFirstNumber = first;
			lessEqual.m_fFirstNumber = first;
			greater.m_fFirstNumber = first;
			greaterEqual.m_fFirstNumber = first;
			equal.m_fSecondNumber = 3;
			different.m_fSecondNumber = 3;
			less.m_fSecondNumber = 3;
			lessEqual.m_fSecondNumber = 3;
			greater.m_fSecondNumber = 3;
			greaterEqual.m_fSecondNumber = 3;
			foreach (int index, GC_MissionCondition comparison : comparisons)
				Check(comparison.BuildCheck().Evaluate(null) == ((comparisonExpected[index] & (1 << row)) != 0), "numeric comparison truth table");
		}
		greater.m_fFirstNumber = 1000000000;
		Check(greater.BuildCheck() && greater.BuildCheck().Evaluate(null), "numeric range endpoint accepted");
		greater.m_fFirstNumber = 2000000000;
		Check(!greater.BuildCheck(), "out of range number rejected");
		greater.m_fFirstNumber = 4.5;
		GC_NoneConditions notEqualGroup = new GC_NoneConditions();
		notEqualGroup.m_aConditions = {equal};
		GC_AllConditions cascade = new GC_AllConditions();
		cascade.m_aConditions = {greater, notEqualGroup};
		Check(cascade.BuildCheck().Evaluate(null), "numeric comparisons cascade through All and None");
		GC_NoneConditions doubleNone = new GC_NoneConditions();
		doubleNone.m_aConditions = {notEqualGroup};
		Check(!doubleNone.BuildCheck().Evaluate(null), "nested single-child None is double NOT");
		GC_MissionRule missionEvent = SimpleRule("logic_passed");
		missionEvent.m_aConditions = {cascade};
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({missionEvent.BuildRule()});
		Check(engine.GetFlag("logic_passed"), "nested logic reaches production action execution");
		greater.m_fFirstNumber = 1;
		engine.Start({missionEvent.BuildRule()});
		Check(!engine.GetFlag("logic_passed"), "false numeric condition blocks production action execution");
		engine.Stop();
		return string.Format("StandardLogic: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	string GC_TestStandardLogic()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunStandardLogic();
	}
}

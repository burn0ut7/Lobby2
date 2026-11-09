modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunStandardLogic()
	{
		GC_AllConditions all = new GC_AllConditions();
		GC_AnyCondition any = new GC_AnyCondition();
		GC_NoneConditions none = new GC_NoneConditions();
		GC_NotAllConditions nand = new GC_NotAllConditions();
		array<ref GC_MissionCondition> operators = {all, any, none, nand};
		array<ref GC_MissionCondition> children = {};
		all.m_aConditions = children;
		any.m_aConditions = children;
		none.m_aConditions = children;
		nand.m_aConditions = children;
		foreach (GC_MissionCondition operation : operators)
			Check(!operation.BuildCheck(), "empty explicit operator fails closed");
		// Independent truth-table bitsets, with up to three inputs.
		for (int inputs = 1; inputs <= 3; inputs++)
		{
			array<int> expected = {2, 2, 1, 1};
			if (inputs == 2)
				expected = {8, 14, 1, 7};
			if (inputs == 3)
				expected = {128, 254, 1, 127};
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
		array<ref GC_MissionCondition> counted = {};
		equal.m_aConditions = counted;
		different.m_aConditions = counted;
		less.m_aConditions = counted;
		lessEqual.m_aConditions = counted;
		greater.m_aConditions = counted;
		greaterEqual.m_aConditions = counted;
		for (int passed; passed <= 3; passed++)
		{
			counted.Clear();
			for (int child; child < 3; child++)
			{
				GC_BooleanCondition leaf = new GC_BooleanCondition();
				leaf.m_bValue = child < passed;
				counted.Insert(leaf);
			}
			for (int required; required <= 4; required++)
			{
				equal.m_iRequiredCount = required;
				different.m_iRequiredCount = required;
				less.m_iRequiredCount = required;
				lessEqual.m_iRequiredCount = required;
				greater.m_iRequiredCount = required;
				greaterEqual.m_iRequiredCount = required;
				array<bool> answers = {passed == required, passed != required, passed < required, passed <= required, passed > required, passed >= required};
				foreach (int index, GC_MissionCondition comparison : comparisons)
				{
					GC_RuleCondition built = comparison.BuildCheck();
					Check(built && built.Evaluate(null) == answers[index], "count comparison boundary");
				}
			}
		}
		counted.Clear();
		foreach (GC_MissionCondition comparison : comparisons)
			Check(!comparison.BuildCheck(), "empty count group fails closed");
		counted.Insert(null);
		foreach (GC_MissionCondition comparison : comparisons)
			Check(!comparison.BuildCheck(), "missing count child fails closed");
		GC_BooleanCondition a = new GC_BooleanCondition();
		GC_BooleanCondition b = new GC_BooleanCondition();
		GC_BooleanCondition c = new GC_BooleanCondition();
		GC_BooleanCondition d = new GC_BooleanCondition();
		GC_AllConditions left = new GC_AllConditions();
		left.m_aConditions = {a, b};
		GC_AllConditions right = new GC_AllConditions();
		right.m_aConditions = {c, d};
		GC_AnyCondition either = new GC_AnyCondition();
		either.m_aConditions = {left, right};
		GC_NoneConditions inverse = new GC_NoneConditions();
		inverse.m_aConditions = {left};
		greaterEqual.m_iRequiredCount = 2;
		greaterEqual.m_aConditions = {left, right, inverse};
		for (int mask; mask < 16; mask++)
		{
			a.m_bValue = (mask & 1) != 0;
			b.m_bValue = (mask & 2) != 0;
			c.m_bValue = (mask & 4) != 0;
			d.m_bValue = (mask & 8) != 0;
			Check(either.BuildCheck().Evaluate(null) == ((63624 & (1 << mask)) != 0), "OR of two AND groups");
			Check(inverse.BuildCheck().Evaluate(null) == ((30583 & (1 << mask)) != 0), "None reverses an entire group");
			// Left and its inverse contribute exactly one; right decides whether two pass.
			Check(greaterEqual.BuildCheck().Evaluate(null) == ((61440 & (1 << mask)) != 0), "two of three nested groups count each group once");
		}
		GC_MissionRule missionEvent = SimpleRule("logic_passed");
		missionEvent.m_aConditions = {greaterEqual};
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({missionEvent.BuildRule()});
		Check(engine.GetFlag("logic_passed"), "nested count reaches production action execution");
		c.m_bValue = false;
		engine.Start({missionEvent.BuildRule()});
		Check(!engine.GetFlag("logic_passed"), "false nested count blocks production action execution");
		engine.Stop();
		greaterEqual.m_iRequiredCount = -1;
		Check(!greaterEqual.BuildCheck(), "negative required count rejected");
		greaterEqual.m_iRequiredCount = 257;
		Check(!greaterEqual.BuildCheck(), "oversized required count rejected");
		for (int removed = 8; removed <= 9; removed++)
		{
			GC_RuleCondition obsolete = new GC_RuleCondition();
			obsolete.m_eGroupNeeds = removed;
			obsolete.m_bReverseAnswer = true;
			Check(!obsolete.IsValid() && !obsolete.Evaluate(null), "removed operator cannot be reinterpreted or inverted");
		}
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

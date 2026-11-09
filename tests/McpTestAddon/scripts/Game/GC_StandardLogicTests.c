modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunStandardLogic()
	{
		GC_ConditionGroup all = new GC_ConditionGroup();
		all.m_iMatch = 0;
		GC_ConditionGroup any = new GC_ConditionGroup();
		any.m_iMatch = 1;
		GC_ConditionGroup none = new GC_ConditionGroup();
		none.m_iMatch = 2;
		GC_ConditionGroup nand = new GC_ConditionGroup();
		nand.m_iMatch = 3;
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
					GC_TestFixedCondition value = new GC_TestFixedCondition();
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

		GC_TestFixedCondition validLeaf = new GC_TestFixedCondition();
		all.m_aConditions = {validLeaf};
		all.m_iMatch = -1;
		Check(!all.BuildCheck(), "negative group mode rejected");
		all.m_iMatch = 4;
		Check(!all.BuildCheck(), "unknown group mode rejected");

		GC_ConditionCount equal = new GC_ConditionCount();
		equal.m_iComparison = 2;
		GC_ConditionCount less = new GC_ConditionCount();
		less.m_iComparison = 1;
		GC_ConditionCount greater = new GC_ConditionCount();
		GC_ConditionCount notEqual = new GC_ConditionCount();
		notEqual.m_iComparison = 3;
		array<ref GC_MissionCondition> comparisons = {equal, less, greater, notEqual};
		array<ref GC_MissionCondition> counted = {};
		equal.m_aConditions = counted;
		less.m_aConditions = counted;
		greater.m_aConditions = counted;
		notEqual.m_aConditions = counted;
		for (int passed; passed <= 3; passed++)
		{
			counted.Clear();
			for (int child; child < 3; child++)
			{
				GC_TestFixedCondition leaf = new GC_TestFixedCondition();
				leaf.m_bValue = child < passed;
				counted.Insert(leaf);
			}
			for (int required; required <= 4; required++)
			{
				equal.m_iRequiredCount = required;
				less.m_iRequiredCount = required;
				greater.m_iRequiredCount = required;
				notEqual.m_iRequiredCount = required;
				array<bool> answers = {passed == required, passed < required, passed > required, passed != required};
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
		GC_TestFixedCondition a = new GC_TestFixedCondition();
		GC_TestFixedCondition b = new GC_TestFixedCondition();
		GC_TestFixedCondition c = new GC_TestFixedCondition();
		GC_TestFixedCondition d = new GC_TestFixedCondition();
		GC_ConditionGroup left = new GC_ConditionGroup();
		left.m_iMatch = 0;
		left.m_aConditions = {a, b};
		GC_ConditionGroup right = new GC_ConditionGroup();
		right.m_iMatch = 0;
		right.m_aConditions = {c, d};
		GC_ConditionGroup either = new GC_ConditionGroup();
		either.m_iMatch = 1;
		either.m_aConditions = {left, right};
		GC_ConditionGroup inverse = new GC_ConditionGroup();
		inverse.m_iMatch = 2;
		inverse.m_aConditions = {left};
		greater.m_iRequiredCount = 1;
		greater.m_aConditions = {left, right, inverse};
		notEqual.m_iRequiredCount = 1;
		notEqual.m_aConditions = {left, right, inverse};
		for (int mask; mask < 16; mask++)
		{
			a.m_bValue = (mask & 1) != 0;
			b.m_bValue = (mask & 2) != 0;
			c.m_bValue = (mask & 4) != 0;
			d.m_bValue = (mask & 8) != 0;
			Check(either.BuildCheck().Evaluate(null) == ((63624 & (1 << mask)) != 0), "OR of two AND groups");
			Check(inverse.BuildCheck().Evaluate(null) == ((30583 & (1 << mask)) != 0), "None reverses an entire group");
			// Left and its inverse contribute exactly one; right decides whether two pass.
			Check(greater.BuildCheck().Evaluate(null) == ((61440 & (1 << mask)) != 0), "two of three nested groups count each group once");
			Check(notEqual.BuildCheck().Evaluate(null) == ((61440 & (1 << mask)) != 0), "not equal counts nested groups once");
		}
		GC_Event missionEvent = SimpleRule("logic_passed");
		missionEvent.m_aConditions = {greater};
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({missionEvent.BuildRule()});
		Check(engine.GetFlag("logic_passed"), "nested count reaches production action execution");
		c.m_bValue = false;
		engine.Start({missionEvent.BuildRule()});
		Check(!engine.GetFlag("logic_passed"), "false nested count blocks production action execution");
		engine.Stop();
		greater.m_iRequiredCount = -1;
		Check(!greater.BuildCheck(), "negative required count rejected");
		greater.m_iRequiredCount = 257;
		Check(!greater.BuildCheck(), "oversized required count rejected");
		greater.m_iRequiredCount = 1;
		greater.m_iComparison = -1;
		Check(!greater.BuildCheck(), "negative comparison rejected");
		greater.m_iComparison = 4;
		Check(!greater.BuildCheck(), "unknown comparison rejected");
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

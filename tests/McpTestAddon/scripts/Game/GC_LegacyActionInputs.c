//! Script-only legacy interpreter inputs. No container registration or editor attributes.
class GC_TestSetFlagAction : GC_ActionBase
{
	string m_sFlagName;
	bool m_bSetTrue;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SET_FLAG;
		action.m_sTextOrName = m_sFlagName;
		action.m_bFlagAnswer = m_bSetTrue;
		return action;
	}
}

class GC_TestSendEventAction : GC_ActionBase
{
	string m_sEventName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SEND_TEST_EVENT;
		action.m_sTextOrName = m_sEventName;
		return action;
	}
}

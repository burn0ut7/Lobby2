#ifdef WORKBENCH
class GC_TimerChoices : GC_ConditionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected void CollectConditionName(BaseContainer condition, typename type, array<string> values)
	{
		string name;
		if (!type.IsInherited(GC_TimerCondition) || !condition.Get("m_sTimerName", name))
			return;
		// Timer compilation trims its identifier; offer the same usable string.
		name.TrimInPlace();
		if (!name.IsEmpty())
			values.Insert(name);
	}
}
#endif

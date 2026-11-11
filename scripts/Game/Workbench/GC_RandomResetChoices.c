#ifdef WORKBENCH
class GC_RandomResetChoices : GC_ConditionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected void CollectConditionName(BaseContainer condition, typename type, array<string> values)
	{
		string name;
		if (type.IsInherited(GC_RandomChanceCondition) && condition.Get("m_sResetName", name) && !name.IsEmpty())
			values.Insert(name);
	}
}
#endif

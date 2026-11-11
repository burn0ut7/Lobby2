#ifdef WORKBENCH
//! Reads named conditions without building or evaluating mission rules.
class GC_ConditionReferenceChoices : GC_MissionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected void CollectNames(BaseContainer manager, array<string> values)
	{
		BaseContainerList events = manager.GetObjectArray("m_aMissionEvents");
		BaseContainerList flags = manager.GetObjectArray("m_aMissionFlags");
		CollectList(events, values, 0);
		CollectList(flags, values, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectList(BaseContainerList items, array<string> values, int depth)
	{
		if (!items || depth > 64)
			return;
		for (int i; i < items.Count(); i++)
		{
			BaseContainer item = items.Get(i);
			if (!item)
				continue;
			typename type = item.GetClassName().ToType();
			if (!type)
				continue;
			CollectConditionName(item, type, values);
			bool countFlagCalls;
			if (type.IsInherited(GC_ConditionCount) && item.Get("m_bCountFlagCalls", countFlagCalls) && countFlagCalls)
				continue;
			if (type.IsInherited(GC_Event) || type.IsInherited(GC_RandomEventGroup) || type.IsInherited(GC_MissionFlag)
				|| type.IsInherited(GC_ConditionGroup) || type.IsInherited(GC_ConditionCount))
			{
				BaseContainerList conditions = item.GetObjectArray("m_aConditions");
				CollectList(conditions, values, depth + 1);
			}
			if (type.IsInherited(GC_RandomEventGroup))
			{
				BaseContainerList children = item.GetObjectArray("m_aEvents");
				CollectList(children, values, depth + 1);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectConditionName(BaseContainer condition, typename type, array<string> values)
	{
	}
}
#endif

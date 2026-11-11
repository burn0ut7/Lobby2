#ifdef WORKBENCH
//! Includes regular events, random groups and their authored child events.
//! Discovery does not build or evaluate rules, so incomplete definitions remain visible.
class GC_MissionEventChoices : GC_MissionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected void CollectNames(BaseContainer manager, array<string> values)
	{
		BaseContainerList events = manager.GetObjectArray("m_aMissionEvents");
		if (!events)
			return;

		for (int eventIndex; eventIndex < events.Count(); eventIndex++)
		{
			BaseContainer definition = events.Get(eventIndex);
			if (!definition)
				continue;
			CollectName(definition, values);

			typename definitionType = definition.GetClassName().ToType();
			if (!definitionType || !definitionType.IsInherited(GC_RandomEventGroup))
				continue;

			// The serialized group contract permits regular children, not nested groups.
			BaseContainerList children = definition.GetObjectArray("m_aEvents");
			if (!children)
				continue;
			for (int childIndex; childIndex < children.Count(); childIndex++)
				CollectName(children.Get(childIndex), values);
		}
	}
}
#endif

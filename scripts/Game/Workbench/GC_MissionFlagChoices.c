#ifdef WORKBENCH
//! Reads authored definitions, including flags whose conditions are not currently valid.
//! No runtime cache, world selection, or authoring mutation is needed for discovery.
class GC_MissionFlagChoices : GC_MissionReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected void CollectNames(BaseContainer manager, array<string> values)
	{
		BaseContainerList flags = manager.GetObjectArray("m_aMissionFlags");
		if (!flags)
			return;

		for (int flagIndex; flagIndex < flags.Count(); flagIndex++)
			CollectName(flags.Get(flagIndex), values);
	}
}
#endif

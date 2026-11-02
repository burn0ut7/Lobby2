//! Disposable editor lifecycle experiment; no runtime behavior.
class GC_MetadataProbeHeader : SCR_MissionHeader
{
	[Attribute(desc: "Add a container to test whether config editing invokes its constructor.")]
	ref array<ref GC_MetadataProbeContainer> m_aProbe;
}

[BaseContainerProps()]
class GC_MetadataProbeContainer
{
	[Attribute("untouched")]
	string m_sValue;

	void GC_MetadataProbeContainer()
	{
#ifdef WORKBENCH
		Print("GC_METADATA_PROBE container constructor");
		m_sValue = "constructor ran";
#endif
	}
}

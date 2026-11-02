//! Native metadata schema. Automatic population is currently an optional test-addon experiment.
class GC_MissionHeader : SCR_MissionHeader
{
	[Attribute(desc: "Placed-content snapshot. Not populated or refreshed automatically by production Lobby2; creation lifecycle is experimental.")]
	ref GC_GeneratedMissionMetadata m_GC_GeneratedMetadata;
}

[BaseContainerProps()]
class GC_GeneratedMissionMetadata
{
	[Attribute("1")]
	int m_iSchemaVersion;
	[Attribute()]
	ResourceName m_sWorld;
	[Attribute()]
	string m_sCaptureScope;
	[Attribute()]
	int m_iEntityCount;
	[Attribute()]
	ref array<string> m_aFactions;
	[Attribute()]
	ref array<string> m_aConfiguredFactions;
	[Attribute()]
	ref array<string> m_aReferencedAddonIds;
	[Attribute()]
	ref array<ref GC_MissionMetadataAsset> m_aAssets;
	[Attribute()]
	ref array<ref GC_MissionMetadataPrefab> m_aPrefabs;
	[Attribute()]
	ref array<ref GC_MissionMetadataAddon> m_aLoadedAddons;
}

[BaseContainerProps()]
class GC_MissionMetadataAsset
{
	[Attribute()]
	string m_sCategory;
	[Attribute()]
	int m_iCount;
}

[BaseContainerProps()]
class GC_MissionMetadataAddon
{
	[Attribute()]
	string m_sId;
	[Attribute()]
	string m_sGuid;
	[Attribute()]
	string m_sTitle;
	[Attribute()]
	bool m_bVanilla;
	[Attribute()]
	bool m_bReferencedByPlacedPrefab;
}

//! Shared data model for the native config and optional JSON export.
[BaseContainerProps()]
class GC_MissionMetadataPrefab
{
	[Attribute()]
	ResourceName m_sPrefab;
	[Attribute()]
	string m_sClass;
	[Attribute()]
	string m_sFaction;
	[Attribute()]
	string m_sAsset;
	[Attribute()]
	string m_sAssetEvidence;
	[Attribute()]
	int m_iCount;
	[Attribute()]
	ref array<string> m_aAddonIds;

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	bool Write(SaveContext context)
	{
		return context.WriteValue("prefab", m_sPrefab)
			&& context.WriteValue("entityClass", m_sClass)
			&& context.WriteValue("faction", m_sFaction)
			&& context.WriteValue("asset", m_sAsset)
			&& context.WriteValue("assetEvidence", m_sAssetEvidence)
			&& context.WriteValue("count", m_iCount)
			&& context.WriteValue("sourceAddonIds", m_aAddonIds);
	}
#endif
}

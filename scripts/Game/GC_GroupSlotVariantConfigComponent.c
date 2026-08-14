[BaseContainerProps()]
class GC_UnitSlotVariantConfig
{
	[Attribute("0", UIWidgets.EditBox, "Index of the corresponding entry in the group's unit prefab slots")]
	protected int m_iUnitPrefabSlotIndex;

	[Attribute("0", UIWidgets.CheckBox, "Resolve a new random variant on future respawns instead of keeping this slot's initial variant")]
	protected bool m_bRerollOnRespawn;

	int GetUnitPrefabSlotIndex()
	{
		return m_iUnitPrefabSlotIndex;
	}

	GC_SlotVariantPolicy GetVariantPolicy()
	{
		if (m_bRerollOnRespawn)
			return GC_SlotVariantPolicy.REROLL_ON_RESPAWN;

		return GC_SlotVariantPolicy.KEEP_VARIANT;
	}
}

class GC_GroupSlotVariantConfigComponentClass : ScriptComponentClass
{
}

//! Optional Lobby2-owned configuration attached to an SCR_AIGroup.
//! Entries are keyed by the matching m_aUnitPrefabSlots index.
class GC_GroupSlotVariantConfigComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Object, "Optional per-unit random variant policies")]
	protected ref array<ref GC_UnitSlotVariantConfig> m_aUnitConfigurations;

	//! Produces exactly one policy for each authored unit slot. Missing entries keep their variant.
	void ResolvePolicies(int unitSlotCount, notnull array<GC_SlotVariantPolicy> policies)
	{
		policies.Resize(unitSlotCount);
		array<bool> configuredIndices = {};
		configuredIndices.Resize(unitSlotCount);

		for (int index = 0; index < unitSlotCount; index++)
			policies[index] = GC_SlotVariantPolicy.KEEP_VARIANT;

		if (!m_aUnitConfigurations)
			return;

		foreach (GC_UnitSlotVariantConfig configuration : m_aUnitConfigurations)
		{
			if (!configuration)
				continue;

			int unitIndex = configuration.GetUnitPrefabSlotIndex();
			if (unitIndex < 0 || unitIndex >= unitSlotCount)
			{
				PrintFormat("GC | Ignoring random variant policy with invalid unit index %1; group has %2 unit slots", unitIndex, unitSlotCount, level: LogLevel.WARNING);
				continue;
			}

			if (configuredIndices[unitIndex])
			{
				PrintFormat("GC | Ignoring duplicate random variant policy for unit index %1", unitIndex, level: LogLevel.WARNING);
				continue;
			}

			configuredIndices[unitIndex] = true;
			policies[unitIndex] = configuration.GetVariantPolicy();
		}
	}
}

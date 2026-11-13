//! Stateless destination policy. Mission addons may extend this class for custom per-player placement.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Original slot position")]
class GC_RespawnLocation
{
	bool IsValid()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Null parameters deliberately request the existing original-slot spawn path.
	bool Resolve(BaseWorld world, GC_Slot slot, int index, out EntitySpawnParams parameters)
	{
		parameters = null;
		return world && index >= 0 && IsValid();
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Current character position")]
class GC_CurrentRespawnLocation : GC_RespawnLocation
{
	//------------------------------------------------------------------------------------------------
	override bool Resolve(BaseWorld world, GC_Slot slot, int index, out EntitySpawnParams parameters)
	{
		parameters = null;
		if (!world || !slot || index < 0 || !slot.GetEntity())
			return false;
		parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		slot.GetEntity().GetWorldTransform(parameters.Transform);
		return true;
	}
}

[BaseContainerProps(visible: false, insertable: false)]
class GC_PlacedRespawnLocation : GC_RespawnLocation
{
	[Attribute("0 0 0", UIWidgets.EditBox, "Offset in metres relative to the location's heading: X right, Y up, Z forward.")]
	vector m_vOffset;
	[Attribute("2", UIWidgets.EditBox, "Metres between players in a wave. Zero places everyone at the exact location. The grid extends right and behind the location.", "0 100")]
	float m_fSpacing = 2;
	[Attribute("4", UIWidgets.EditBox, "Players per row before starting the next row.", "1 128")]
	int m_iColumns = 4;
	[Attribute("0", UIWidgets.CheckBox, "Place on terrain height, then add the offset's Y value. Leave off for roofs, bridges and elevated locations. This is not a collision or safe-ground search.")]
	bool m_bSnapToTerrain;

	//------------------------------------------------------------------------------------------------
	override bool IsValid()
	{
		return m_fSpacing >= 0 && m_fSpacing <= 100 && m_iColumns >= 1 && m_iColumns <= 128
			&& ValidVector(m_vOffset);
	}

	//------------------------------------------------------------------------------------------------
	static bool ValidVector(vector value)
	{
		for (int i; i < 3; i++)
		{
			if (!(value[i] >= -10000000 && value[i] <= 10000000))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GetAnchor(BaseWorld world, int index, out vector origin, out float heading, out int placementIndex)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Resolve(BaseWorld world, GC_Slot slot, int index, out EntitySpawnParams parameters)
	{
		parameters = null;
		if (!world || index < 0 || !IsValid())
			return false;
		vector origin;
		float heading;
		int placementIndex;
		if (!GetAnchor(world, index, origin, heading, placementIndex) || !ValidVector(origin))
			return false;
		EntitySpawnParams result = new EntitySpawnParams();
		result.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(heading, 0, 0), result.Transform);
		result.Transform[3] = origin;
		int row = placementIndex / m_iColumns;
		int column = placementIndex % m_iColumns;
		vector offset = m_vOffset + Vector(column * m_fSpacing, 0, -row * m_fSpacing);
		vector position = offset.Multiply4(result.Transform);
		if (m_bSnapToTerrain)
			position[1] = world.GetSurfaceY(position[0], position[2]) + m_vOffset[1];
		if (!ValidVector(position))
			return false;
		result.Transform[3] = position;
		parameters = result;
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Named entities")]
class GC_EntityRespawnLocation : GC_PlacedRespawnLocation
{
	[Attribute("", UIWidgets.EditComboBox, "Exact unique names of destination entities. Players are distributed through this list in order, then into rows at each location. Position and heading are read again for every spawn, so moving or rotating an entity moves its respawn point. A missing selected entity skips that player without spending a ticket; it never falls back to the original position. Use an offset to spawn beside a vehicle, not inside it.", enums: {ParamEnum("", "", "gc-choices:2:GC_EntityReferenceChoices")})]
	ref array<string> m_aEntities;

	//------------------------------------------------------------------------------------------------
	override bool IsValid()
	{
		if (!super.IsValid() || !m_aEntities || m_aEntities.IsEmpty() || !GC_RuleAction.ValidTargetNames(m_aEntities))
			return false;
		array<string> seen = {};
		foreach (string name : m_aEntities)
		{
			if (seen.Contains(name))
				return false;
			seen.Insert(name);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool GetAnchor(BaseWorld world, int index, out vector origin, out float heading, out int placementIndex)
	{
		IEntity anchor = world.FindEntityByName(m_aEntities[index % m_aEntities.Count()]);
		if (!anchor)
			return false;
		origin = anchor.GetOrigin();
		heading = anchor.GetYawPitchRoll()[0];
		placementIndex = index / m_aEntities.Count();
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("World position")]
class GC_PositionRespawnLocation : GC_PlacedRespawnLocation
{
	[Attribute("0 0 0", UIWidgets.EditBox, "World coordinates in metres. Y is height unless Snap To Terrain is enabled.")]
	vector m_vPosition;
	[Attribute("0", UIWidgets.EditBox, "World heading in degrees.", "-360 360")]
	float m_fHeading;

	//------------------------------------------------------------------------------------------------
	override bool IsValid()
	{
		return super.IsValid() && ValidVector(m_vPosition) && m_fHeading >= -360 && m_fHeading <= 360;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool GetAnchor(BaseWorld world, int index, out vector origin, out float heading, out int placementIndex)
	{
		origin = m_vPosition;
		heading = m_fHeading;
		placementIndex = index;
		return true;
	}
}

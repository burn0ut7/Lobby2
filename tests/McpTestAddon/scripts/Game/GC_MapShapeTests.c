//! Optional map-only fixture; mutations use the production component's trusted APIs.
modded class GC_LobbyManager
{
	protected ref array<IEntity> m_aGC_MapShapeFixtures = {};

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeSpawn()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity())
			return "UNAVAILABLE authority/player";
		m_vGC_AOOrigin = player.GetControlledEntity().GetOrigin();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = m_vGC_AOOrigin;
		IEntity shape = GetGame().SpawnEntityPrefab(Resource.Load("{8F470B543A20D963}Prefabs/Logic/Map/MapShape.et"), GetGame().GetWorld(), params);
		if (!shape)
			return "FAILED spawn";
		shape.SetName("MapShapeFixture" + m_aGC_MapShapeFixtures.Count());
		m_aGC_MapShapeFixtures.Insert(shape);
		return string.Format("spawned index=%1 AO=%2", m_aGC_MapShapeFixtures.Count() - 1, GC_AOLimitEntity.Cast(shape) != null);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeConfigure(int index, int geometry, string factionKey, int fillColor)
	{
		if (!m_aGC_MapShapeFixtures.IsIndexValid(index) || !m_aGC_MapShapeFixtures[index])
			return "UNAVAILABLE shape";
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(m_aGC_MapShapeFixtures[index].FindComponent(GC_MapShapeComponent));
		array<vector> points = {};
		if (geometry == 0)
			points = {"-180 0 -100", "-20 0 -100", "-20 0 100", "-180 0 100"};
		else if (geometry == 1)
			points = {"20 0 -100", "180 0 -100", "180 0 -30", "90 0 -30", "90 0 100", "20 0 100"};
		else
			points = {"0 0 100", "95 0 31", "59 0 -81", "-59 0 -81", "-95 0 31"};
		for (int i; i < points.Count(); i++)
			points[i] = points[i] + m_vGC_AOOrigin;
		array<string> factions = {};
		if (!factionKey.IsEmpty())
			factions.Insert(factionKey);
		return string.Format("polygon=%1 visibility=%2 style=%3 enabled=%4", shape.SetPolygon(points),
			shape.SetFactionKeys(factions),
			shape.SetStyle(Color.BLUE, 3, false, true, fillColor), shape.SetEnabled(true));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeStyle(int index, float width, bool fill)
	{
		if (!m_aGC_MapShapeFixtures.IsIndexValid(index) || !m_aGC_MapShapeFixtures[index])
			return "UNAVAILABLE shape";
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(m_aGC_MapShapeFixtures[index].FindComponent(GC_MapShapeComponent));
		return shape.SetStyle(Color.BLUE, width, false, fill, 0x333366FF).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeInvert(int index, bool inverted)
	{
		if (!m_aGC_MapShapeFixtures.IsIndexValid(index) || !m_aGC_MapShapeFixtures[index])
			return "UNAVAILABLE shape";
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(m_aGC_MapShapeFixtures[index].FindComponent(GC_MapShapeComponent));
		return shape.SetInvertFill(inverted).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeToggle(int index, bool enabled, bool remove)
	{
		if (!m_aGC_MapShapeFixtures.IsIndexValid(index) || !m_aGC_MapShapeFixtures[index])
			return "UNAVAILABLE shape";
		if (remove)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_aGC_MapShapeFixtures[index]);
			m_aGC_MapShapeFixtures[index] = null;
			return "removed";
		}
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(m_aGC_MapShapeFixtures[index].FindComponent(GC_MapShapeComponent));
		return shape.SetEnabled(enabled).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMapShapeAO(int index, bool enabled, bool fill)
	{
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !m_aGC_AOFixtures[index])
			return "UNAVAILABLE AO";
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(m_aGC_AOFixtures[index].FindComponent(GC_MapShapeComponent));
		if (!shape)
			return "UNAVAILABLE component";
		return string.Format("enabled=%1 style=%2", shape.SetEnabled(enabled), shape.SetStyle(0xFFFFB000, 3, false, fill, 0x33FFB000));
	}
}

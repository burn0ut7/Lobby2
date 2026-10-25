//! Counters wrap production presentation; no substitute enforcement or automatic fixtures.
class GC_BoundaryPresentationProbe
{
	static int s_iSnapshots;
	static int s_iProjections;
	static int s_iViewRebuilds;
	static int s_iMapTicks;
	static int s_iSetupStates;
	static int s_iAOWarnings;
	static int s_iSetupWarnings;
	static int s_iAnnouncements;
}

modded class GC_MapShapeOverlay
{
	//------------------------------------------------------------------------------------------------
	override void Receive(int generation, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<string> names = null, array<vector> frameBounds = null)
	{
		GC_BoundaryPresentationProbe.s_iSnapshots++;
		super.Receive(generation, counts, colors, fills, inverted, widths, points, names, frameBounds);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Project(SCR_MapEntity mapEntity)
	{
		GC_BoundaryPresentationProbe.s_iProjections++;
		if (m_Canvas && mapEntity && mapEntity.IsOpen())
		{
			float width, height, x, z;
			m_Canvas.GetScreenSize(width, height);
			mapEntity.ScreenToWorld(0, 0, x, z);
			if (width > 0 && height > 0 && (m_bGeometryDirty || width != m_fCanvasWidth || height != m_fCanvasHeight
				|| x != m_fProjectedX || z != m_fProjectedZ || mapEntity.GetCurrentZoom() != m_fProjectedZoom))
				GC_BoundaryPresentationProbe.s_iViewRebuilds++;
		}
		super.Project(mapEntity);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationMap()
	{
		string labels;
		foreach (string name : m_aNames)
		{
			if (!name.IsEmpty())
				labels += " [" + name + "]";
		}
		return string.Format("cachedShapes=%1 cachedPoints=%2 canvas=%3 commands=%4 framed=%5", m_aCounts.Count(), m_aPoints.Count(), m_Canvas != null, m_aCommands.Count(), m_bFramed) + labels;
	}
}

modded class GC_AOWarning
{
	//------------------------------------------------------------------------------------------------
	override void Update(IEntity character, float seconds, bool setupWarning = false, string warningText = "RETURN TO AO")
	{
		if (setupWarning)
			GC_BoundaryPresentationProbe.s_iSetupWarnings++;
		else
			GC_BoundaryPresentationProbe.s_iAOWarnings++;
		super.Update(character, seconds, setupWarning, warningText);
	}
}

modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	override protected void GC_UpdateMapShapes()
	{
		GC_BoundaryPresentationProbe.s_iMapTicks++;
		super.GC_UpdateMapShapes();
	}

	//------------------------------------------------------------------------------------------------
	override protected void GC_ApplySetupState(IEntity character, float seconds, float outsideSeconds, bool safeStart, bool frozen, string displayText)
	{
		GC_BoundaryPresentationProbe.s_iSetupStates++;
		super.GC_ApplySetupState(character, seconds, outsideSeconds, safeStart, frozen, displayText);
	}

	//------------------------------------------------------------------------------------------------
	override protected void GC_RpcDoAnnouncement(string title, float seconds, WorldTimestamp sentAt)
	{
		GC_BoundaryPresentationProbe.s_iAnnouncements++;
		super.GC_RpcDoAnnouncement(title, seconds, sentAt);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationState()
	{
		string mapState = "overlay=absent";
		if (m_GC_MapShapes)
			mapState = m_GC_MapShapes.GC_TestPresentationMap();
		return string.Format("%1 setup=%2 combat=%3 announcement=%4", mapState, GC_SetupActive(), GC_IsCombatRestricted(), GC_AnnouncementActive());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationSubscription()
	{
		return string.Format("player=%1 subscribed=%2 revision=%3 faction=%4", GetPlayerId(), m_bGC_MapSubscribed, m_iGC_MapRevision, m_sGC_MapFaction);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationSubscriber(int playerId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return "UNAVAILABLE controller";
		return controller.GC_TestPresentationSubscription();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationReset()
	{
		GC_BoundaryPresentationProbe.s_iSnapshots = 0;
		GC_BoundaryPresentationProbe.s_iProjections = 0;
		GC_BoundaryPresentationProbe.s_iViewRebuilds = 0;
		GC_BoundaryPresentationProbe.s_iMapTicks = 0;
		GC_BoundaryPresentationProbe.s_iSetupStates = 0;
		GC_BoundaryPresentationProbe.s_iAOWarnings = 0;
		GC_BoundaryPresentationProbe.s_iSetupWarnings = 0;
		GC_BoundaryPresentationProbe.s_iAnnouncements = 0;
		return "RESET";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationReport()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE controller";
		string counts = string.Format("snapshots=%1 projections=%2 mapTicks=%3 setupStates=%4 aoWarnings=%5 setupWarnings=%6 announcements=%7", GC_BoundaryPresentationProbe.s_iSnapshots, GC_BoundaryPresentationProbe.s_iProjections, GC_BoundaryPresentationProbe.s_iMapTicks, GC_BoundaryPresentationProbe.s_iSetupStates, GC_BoundaryPresentationProbe.s_iAOWarnings, GC_BoundaryPresentationProbe.s_iSetupWarnings, GC_BoundaryPresentationProbe.s_iAnnouncements);
		return counts + " rebuilds=" + GC_BoundaryPresentationProbe.s_iViewRebuilds.ToString() + "\n" + controller.GC_TestPresentationState();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPresentationShape(string entityName, bool enabled, string factionKey)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return "UNAVAILABLE entity";
		GC_MapShapeComponent shape = GC_MapShapeComponent.Cast(entity.FindComponent(GC_MapShapeComponent));
		if (!shape)
			return "UNAVAILABLE shape";
		array<string> factions = {};
		if (!factionKey.IsEmpty())
			factions.Insert(factionKey);
		bool visible = shape.SetEnabled(enabled);
		bool audience = shape.SetFactionKeys(factions);
		return string.Format("enabled=%1 audience=%2", visible, audience);
	}
}

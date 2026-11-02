//! Recorded map presentation. Geometry is world X/Z; native colors are packed ARGB.
class GC_TelemetryMapShape
{
	ref array<float> m_aPoints = {};
	int m_iLineColor;
	int m_iFillColor;
	float m_fWidth;
	bool m_bInverted;
	bool m_bClosed = true;
	bool m_bWorldWidth;

	void Write(JsonSaveContext context)
	{
		context.WriteValue("points", m_aPoints);
		context.WriteValue("lineColor", m_iLineColor);
		context.WriteValue("fillColor", m_iFillColor);
		context.WriteValue("width", m_fWidth);
		context.WriteValue("inverted", m_bInverted);
		context.WriteValue("closed", m_bClosed);
		context.WriteValue("worldWidth", m_bWorldWidth);
	}
}

class GC_TelemetryMapMarker
{
	vector m_vPosition;
	string m_sLabel;
	ResourceName m_sIcon;
	string m_sQuad;
	int m_iColor = Color.WHITE;
	float m_fRotation;
	float m_fSize = 32;
	float m_fMinSize;
	bool m_bWorldScale;
	int m_iNativeType = -1;
	int m_iNativeConfig = -1;
	int m_iNativeFlags;
	int m_iMilitaryIdentity = -1;
	int m_iMilitaryDimension = -1;

	void Write(JsonSaveContext context)
	{
		GC_TelemetryRecorder.WriteVector(context, "position", m_vPosition);
		context.WriteValue("label", m_sLabel);
		context.WriteValue("iconResource", m_sIcon);
		context.WriteValue("iconName", m_sQuad);
		context.WriteValue("color", m_iColor);
		context.WriteValue("rotation", m_fRotation);
		context.WriteValue("size", m_fSize);
		context.WriteValue("minSize", m_fMinSize);
		context.WriteValue("worldScale", m_bWorldScale);
		context.WriteValue("nativeType", m_iNativeType);
		context.WriteValue("nativeConfig", m_iNativeConfig);
		context.WriteValue("nativeFlags", m_iNativeFlags);
		context.WriteValue("militaryIdentity", m_iMilitaryIdentity);
		context.WriteValue("militaryDimension", m_iMilitaryDimension);
	}
}

//! Complete replacement snapshots make deletion and backward seeking unambiguous.
class GC_TelemetryMapSnapshot
{
	string m_sFaction;
	bool m_bMarkersAvailable;
	bool m_bDrawingsAvailable;
	ref array<ref GC_TelemetryMapShape> m_aShapes = {};
	ref array<ref GC_TelemetryMapMarker> m_aMarkers = {};

	void Write(JsonSaveContext context)
	{
		context.WriteValue("faction", m_sFaction);
		context.WriteValue("markersAvailable", m_bMarkersAvailable);
		context.WriteValue("drawingsAvailable", m_bDrawingsAvailable);
		int count = m_aShapes.Count();
		context.StartArray("shapes", count);
		foreach (GC_TelemetryMapShape shape : m_aShapes)
		{
			context.StartObject();
			shape.Write(context);
			context.EndObject();
		}
		context.EndArray();
		count = m_aMarkers.Count();
		context.StartArray("markers", count);
		foreach (GC_TelemetryMapMarker marker : m_aMarkers)
		{
			context.StartObject();
			marker.Write(context);
			context.EndObject();
		}
		context.EndArray();
	}
}

//! Read-only adapter; map systems retain all geometry and visibility ownership.
class GC_TelemetryMap
{
	protected ref map<string, string> m_mPrevious = new map<string, string>();
	protected ref GC_MapPlanDrawings m_Drawings = new GC_MapPlanDrawings();
	protected int m_iFaction = -1;
	protected float m_fNextCycle;

	//------------------------------------------------------------------------------------------------
	void Sample(GC_TelemetryRecorder recorder, float now)
	{
		if (now < m_fNextCycle)
			return;
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		FactionManager factions = GetGame().GetFactionManager();
		if (!Replication.IsServer() || !mode || !manager || !factions)
			return;
		if (factions.GetFactionsCount() > 64)
		{
			recorder.Stop("overflow");
			return;
		}
		Faction faction;
		if (m_iFaction >= 0)
			faction = factions.GetFactionByIndex(m_iFaction);
		GC_TelemetryMapSnapshot snapshot = Capture(mode, manager, faction, m_iFaction);
		m_iFaction++;
		if (m_iFaction >= factions.GetFactionsCount())
		{
			m_iFaction = -1;
			m_fNextCycle = now + 1000;
		}
		JsonSaveContext check = new JsonSaveContext();
		check.SetMaxDecimalPlaces(6);
		snapshot.Write(check);
		string encoded = check.SaveToString();
		if (!check.IsValid() || encoded.Length() > 65536 || snapshot.m_aShapes.Count() > 384 || snapshot.m_aMarkers.Count() > 512)
		{
			recorder.Stop("overflow");
			return;
		}
		if (m_mPrevious.Contains(snapshot.m_sFaction) && m_mPrevious.Get(snapshot.m_sFaction) == encoded)
			return;
		JsonSaveContext context = recorder.BeginRecord("map-state");
		snapshot.Write(context);
		if (recorder.CommitRecord(context) > 0)
			m_mPrevious.Set(snapshot.m_sFaction, encoded);
	}

	//------------------------------------------------------------------------------------------------
	protected GC_TelemetryMapSnapshot Capture(GC_LobbyGameMode mode, GC_LobbyManager manager, Faction faction, int factionIndex)
	{
		GC_TelemetryMapSnapshot result = new GC_TelemetryMapSnapshot();
		if (faction)
			result.m_sFaction = faction.GetFactionKey();
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		mode.GetMapShapes().GetFactionSnapshot(faction, counts, colors, fills, inverted, widths, points);
		int cursor;
		foreach (int index, int count : counts)
		{
			GC_TelemetryMapShape shape = new GC_TelemetryMapShape();
			shape.m_iLineColor = colors[index];
			shape.m_iFillColor = fills[index];
			shape.m_fWidth = widths[index];
			shape.m_bInverted = inverted[index];
			for (int i; i < count; i++)
			{
				shape.m_aPoints.Insert(points[cursor][0]);
				shape.m_aPoints.Insert(points[cursor++][2]);
			}
			result.m_aShapes.Insert(shape);
		}
		foreach (GC_BriefingMarker authored : manager.GetBriefingMarkers())
		{
			if (!authored.IsVisibleToFaction(faction, manager.GetLobbyPhase()))
				continue;
			GC_TelemetryMapMarker marker = new GC_TelemetryMapMarker();
			marker.m_vPosition = authored.GetWorldPosition();
			marker.m_sLabel = authored.GetDescription();
			marker.m_sIcon = authored.GetImage();
			marker.m_sQuad = authored.GetQuadName();
			marker.m_iColor = authored.GetColor();
			marker.m_fRotation = authored.GetRotation();
			marker.m_fSize = authored.GetSize();
			marker.m_fMinSize = authored.GetMinSize();
			marker.m_bWorldScale = authored.IsWorldScale();
			result.m_aMarkers.Insert(marker);
		}
		CaptureSharedMarkers(result, factionIndex);
		CaptureDrawings(result, factionIndex);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void CaptureSharedMarkers(GC_TelemetryMapSnapshot result, int factionIndex)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager || !manager.GetMarkerConfig())
			return;
		result.m_bMarkersAvailable = true;
		array<SCR_MapMarkerBase> markers = manager.GetStaticMarkers();
		markers.InsertAll(manager.GetDisabledMarkers());
		foreach (SCR_MapMarkerBase sourceMarker : markers)
		{
			if (sourceMarker.GetMarkerID() < 0)
				continue;
			if (sourceMarker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM && sourceMarker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY)
				continue;
			if (sourceMarker.GetMarkerFactionFlags() != 0 && (factionIndex < 0 || !sourceMarker.IsFaction(factionIndex)))
				continue;
			GC_TelemetryMapMarker marker = new GC_TelemetryMapMarker();
			int xy[2];
			sourceMarker.GetWorldPos(xy);
			marker.m_vPosition = Vector(xy[0], 0, xy[1]);
			marker.m_sLabel = sourceMarker.GetCustomText();
			marker.m_fRotation = sourceMarker.GetRotation();
			marker.m_iNativeType = sourceMarker.GetType();
			marker.m_iNativeConfig = sourceMarker.GetMarkerConfigID();
			marker.m_iNativeFlags = sourceMarker.GetFlags();
			SCR_MapMarkerEntryPlaced config = SCR_MapMarkerEntryPlaced.Cast(manager.GetMarkerConfig().GetMarkerEntryConfigByType(sourceMarker.GetType()));
			if (config)
			{
				ResourceName glow;
				config.GetIconEntry(sourceMarker.GetIconEntry(), marker.m_sIcon, glow, marker.m_sQuad);
				marker.m_iColor = config.GetColorEntry(sourceMarker.GetColorEntry()).PackToInt();
			}
			SCR_MapMarkerEntryMilitary military = SCR_MapMarkerEntryMilitary.Cast(manager.GetMarkerConfig().GetMarkerEntryConfigByType(sourceMarker.GetType()));
			if (military)
			{
				SCR_MarkerMilitaryFactionEntry identity = military.GetFactionEntry(marker.m_iNativeConfig % 100);
				SCR_MarkerMilitaryDimension dimension = military.GetDimensionEntry(marker.m_iNativeConfig / 100);
				if (identity)
				{
					marker.m_iMilitaryIdentity = identity.GetFactionIdentity();
					marker.m_iColor = identity.GetColor().PackToInt();
				}
				if (dimension)
					marker.m_iMilitaryDimension = dimension.GetDimension();
			}
			result.m_aMarkers.Insert(marker);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CaptureDrawings(GC_TelemetryMapSnapshot result, int factionIndex)
	{
		result.m_bDrawingsAvailable = m_Drawings.IsAvailable();
		if (!result.m_bDrawingsAvailable)
			return;
		array<ref array<int>> rows = {};
		m_Drawings.Capture(rows);
		if (rows.IsEmpty())
			return;
		SCR_MapEntity mapEntity = SCR_MapEntity.Cast(GetGame().GetMapManager());
		if (!mapEntity)
		{
			result.m_bDrawingsAvailable = false;
			return;
		}
		vector size = mapEntity.Size();
		vector offset = mapEntity.Offset();
		foreach (array<int> row : rows)
		{
			if (!row || row.Count() < 3)
				continue;
			int properties = row[0];
			int visibility = (properties >> 10) & 63;
			if (visibility != 63 && visibility != factionIndex)
				continue;
			int width = (properties >> 1) & 15;
			int opacity = (properties >> 5) & 3;
			int color = (properties >> 7) & 7;
			GC_TelemetryMapShape shape = new GC_TelemetryMapShape();
			int packed = Color.FromRGBA(191 * ((color >> 2) & 1), 191 * ((color >> 1) & 1), 191 * (color & 1), 64 * (opacity + 1) - 1).PackToInt();
			shape.m_fWidth = 0.5 * width * width;
			shape.m_bClosed = width == 0;
			shape.m_bWorldWidth = true;
			if (shape.m_bClosed)
				shape.m_iFillColor = packed;
			else
				shape.m_iLineColor = packed;
			for (int i = 1; i < row.Count(); i++)
			{
				int pointData = row[i];
				int pointX = (pointData >> 16) & 65535;
				int pointZ = pointData & 65535;
				shape.m_aPoints.Insert(pointX / 65535.0 * size[0] - offset[0]);
				shape.m_aPoints.Insert(pointZ / 65535.0 * size[2] - offset[2]);
			}
			result.m_aShapes.Insert(shape);
		}
	}
}

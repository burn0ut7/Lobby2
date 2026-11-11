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
	protected ref GC_TelemetryMapSnapshot m_Snapshot;
	protected ref JsonSaveContext m_Encoding;
	protected ref array<SCR_MapMarkerBase> m_aNativeMarkers;
	protected int m_iStage;
	protected int m_iIndex;
	protected int m_iShapeRevision;
	protected int m_iCountedShapes;
	protected int m_iCountedMarkers;
	protected int m_iMinimumBytes;
	protected GC_MapPlans m_Plans;
	protected Faction m_Faction;
	protected int m_iPlanRevision;
	protected int m_iMarkerRevision;
	protected SCR_EGameModeState m_ePhase;

	//------------------------------------------------------------------------------------------------
	void Sample(GC_TelemetryRecorder recorder, float now, int started)
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
		GC_MapPlans plans = GC_MapPlans.GetInstance();
		int planRevision;
		if (plans)
			planRevision = plans.GetContentRevision();
		if (m_Snapshot && (m_iShapeRevision != mode.GetMapShapes().GetRevision()
			|| m_iMarkerRevision != manager.GetBriefingMarkerRevision() || m_Plans != plans
			|| m_iPlanRevision != planRevision || m_Faction != faction || m_ePhase != manager.GetLobbyPhase()))
		{
			ResetCapture();
			return;
		}
		if (!m_Snapshot)
		{
			m_Snapshot = new GC_TelemetryMapSnapshot();
			if (faction)
				m_Snapshot.m_sFaction = faction.GetFactionKey();
			m_iShapeRevision = mode.GetMapShapes().GetRevision();
			m_iMarkerRevision = manager.GetBriefingMarkerRevision();
			m_iPlanRevision = planRevision;
			m_Plans = plans;
			m_Faction = faction;
			m_ePhase = manager.GetLobbyPhase();
		}
		// Each unit visits one source/encoded item. Share the recorder deadline;
		// retaining cursors avoids restarting expensive collections next frame.
		for (int unit; unit < 32 && recorder.IsRunning() && System.GetTickCount(started) < GC_TelemetryRecorder.TICK_BUDGET_MS; unit++)
		{
			if (m_iStage == 0)
			{
				if (m_iIndex < Math.Max(mode.GetMapShapes().GetShapeCount(), manager.GetBriefingMarkers().Count()))
				{
					GC_TelemetryMapSnapshot part = Capture(mode, manager, faction, m_iIndex++);
					foreach (GC_TelemetryMapShape shape : part.m_aShapes)
						m_Snapshot.m_aShapes.Insert(shape);
					foreach (GC_TelemetryMapMarker marker : part.m_aMarkers)
						m_Snapshot.m_aMarkers.Insert(marker);
				}
				else
				{
					m_aNativeMarkers = {};
					SCR_MapMarkerManagerComponent nativeMarkers = SCR_MapMarkerManagerComponent.GetInstance();
					if (nativeMarkers && nativeMarkers.GetMarkerConfig())
					{
						m_Snapshot.m_bMarkersAvailable = true;
						m_aNativeMarkers.InsertAll(nativeMarkers.GetStaticMarkers());
						m_aNativeMarkers.InsertAll(nativeMarkers.GetDisabledMarkers());
					}
					NextStage();
				}
			}
			else if (m_iStage == 1)
			{
				if (m_iIndex < m_aNativeMarkers.Count())
					CaptureSharedMarker(m_Snapshot, m_iFaction, m_aNativeMarkers[m_iIndex++]);
				else
					NextStage();
			}
			else if (m_iStage == 2)
			{
				if (!CaptureDrawing(recorder, m_Snapshot, m_iFaction, m_iIndex++))
				{
					m_Encoding = new JsonSaveContext();
					m_Encoding.SetMaxDecimalPlaces(6);
					m_Encoding.WriteValue("faction", m_Snapshot.m_sFaction);
					m_Encoding.WriteValue("markersAvailable", m_Snapshot.m_bMarkersAvailable);
					m_Encoding.WriteValue("drawingsAvailable", m_Snapshot.m_bDrawingsAvailable);
					int shapeCount = m_Snapshot.m_aShapes.Count();
					m_Encoding.StartArray("shapes", shapeCount);
					NextStage();
				}
			}
			else if (m_iStage == 3)
			{
				if (m_iIndex < m_Snapshot.m_aShapes.Count())
				{
					m_Encoding.StartObject();
					m_Snapshot.m_aShapes[m_iIndex++].Write(m_Encoding);
					m_Encoding.EndObject();
				}
				else
				{
					m_Encoding.EndArray();
					int markerCount = m_Snapshot.m_aMarkers.Count();
					m_Encoding.StartArray("markers", markerCount);
					NextStage();
				}
			}
			else if (m_iStage == 4)
			{
				if (m_iIndex < m_Snapshot.m_aMarkers.Count())
				{
					m_Encoding.StartObject();
					m_Snapshot.m_aMarkers[m_iIndex++].Write(m_Encoding);
					m_Encoding.EndObject();
				}
				else
				{
					m_Encoding.EndArray();
					NextStage();
				}
			}
			else
			{
				string encoded = m_Encoding.SaveToString();
				if (!m_Encoding.IsValid() || encoded.Length() > 65536)
					recorder.Stop("overflow");
				else if (!m_mPrevious.Contains(m_Snapshot.m_sFaction) || m_mPrevious.Get(m_Snapshot.m_sFaction) != encoded)
				{
					if (recorder.CommitMapSnapshot(encoded) > 0)
						m_mPrevious.Set(m_Snapshot.m_sFaction, encoded);
				}
				ResetCapture();
				if (++m_iFaction >= factions.GetFactionsCount())
				{
					m_iFaction = -1;
					m_fNextCycle = now + 1000;
				}
				return;
			}
			if (!WithinLimits())
				recorder.Stop("overflow");
		}
	}

	//! Account only newly appended items. Reject certainly oversized payloads
	//! before native JSON serialization; the exact encoded size is checked too.
	protected bool WithinLimits()
	{
		if (m_Snapshot.m_aShapes.Count() > 384 || m_Snapshot.m_aMarkers.Count() > 512)
			return false;
		while (m_iCountedShapes < m_Snapshot.m_aShapes.Count())
			m_iMinimumBytes += m_Snapshot.m_aShapes[m_iCountedShapes++].m_aPoints.Count() * 2;
		while (m_iCountedMarkers < m_Snapshot.m_aMarkers.Count())
		{
			GC_TelemetryMapMarker marker = m_Snapshot.m_aMarkers[m_iCountedMarkers++];
			m_iMinimumBytes += marker.m_sLabel.Length() + marker.m_sIcon.Length() + marker.m_sQuad.Length();
		}
		return m_iMinimumBytes <= 65536;
	}

	protected void NextStage()
	{
		m_iStage++;
		m_iIndex = 0;
	}

	protected void ResetCapture()
	{
		m_Snapshot = null;
		m_Encoding = null;
		m_aNativeMarkers = null;
		m_iStage = 0;
		m_iIndex = 0;
		m_iCountedShapes = 0;
		m_iCountedMarkers = 0;
		m_iMinimumBytes = 0;
		m_Plans = null;
		m_Faction = null;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_TelemetryMapSnapshot Capture(GC_LobbyGameMode mode, GC_LobbyManager manager, Faction faction, int sourceIndex)
	{
		GC_TelemetryMapSnapshot result = new GC_TelemetryMapSnapshot();
		if (faction)
			result.m_sFaction = faction.GetFactionKey();
		array<int> counts = {}, colors = {}, fills = {};
		array<bool> inverted = {};
		array<float> widths = {};
		array<vector> points = {};
		mode.GetMapShapes().GetFactionSnapshot(faction, counts, colors, fills, inverted, widths, points, sourceIndex, 1);
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
		if (sourceIndex < manager.GetBriefingMarkers().Count())
		{
			GC_BriefingMarker authored = manager.GetBriefingMarkers()[sourceIndex];
			if (!authored.IsVisibleToFaction(faction, manager.GetLobbyPhase()))
				return result;
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
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void CaptureSharedMarker(GC_TelemetryMapSnapshot result, int factionIndex, SCR_MapMarkerBase sourceMarker)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!sourceMarker || !manager || !manager.GetMarkerConfig())
			return;
		if (sourceMarker.GetMarkerID() < 0)
			return;
		if (sourceMarker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM && sourceMarker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY)
			return;
		if (sourceMarker.GetMarkerFactionFlags() != 0 && (factionIndex < 0 || !sourceMarker.IsFaction(factionIndex)))
			return;
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

	//------------------------------------------------------------------------------------------------
	protected bool CaptureDrawing(GC_TelemetryRecorder recorder, GC_TelemetryMapSnapshot result, int factionIndex, int index)
	{
		result.m_bDrawingsAvailable = m_Drawings.IsAvailable();
		if (!result.m_bDrawingsAvailable)
			return false;
		array<int> row;
		if (!m_Drawings.CaptureRow(index, row))
			return false;
		SCR_MapEntity mapEntity = SCR_MapEntity.Cast(GetGame().GetMapManager());
		if (!mapEntity)
		{
			result.m_bDrawingsAvailable = false;
			return false;
		}
		vector size = mapEntity.Size();
		vector offset = mapEntity.Offset();
		if (row && row.Count() > 256)
		{
			recorder.Stop("overflow");
			return true;
		}
		if (!row || row.Count() < 3)
			return true;
		int properties = row[0];
		int visibility = (properties >> 10) & 63;
		if (visibility != 63 && visibility != factionIndex)
			return true;
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
		return true;
	}
}

[ComponentEditorProps(category: "GC Lobby/Mission", description: "Passive map polygon on a closed PolylineShapeEntity. Solid fill and outline; no gameplay restriction.")]
class GC_MapShapeComponentClass : ScriptComponentClass
{
}

class GC_MapShapeComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Show this shape on maps. Independent of AO enforcement.", category: "Map shape")]
	protected bool m_bEnabled;
	[Attribute("0", UIWidgets.CheckBox, "Frame this visible polygon when the map opens in lobby or briefing, with a margin around all opted-in shapes. Also frame once on the first map opening during play; later openings preserve manual pan and zoom.", category: "Map shape")]
	protected bool m_bAutoFrameMap;
	[Attribute("", UIWidgets.EditComboBox, "Empty shows everyone; otherwise only these native faction keys can see the shape.", category: "Map shape", enums: {ParamEnum("", "", "gc-choices:1:GC_FactionChoices")})]
	protected ref array<string> m_aFactionKeys;
	[Attribute("0.2 0.5 1 1", UIWidgets.ColorPicker, "Outline color and opacity. Set opacity to zero to hide the outline.", category: "Outline")]
	protected ref Color m_LineColor;
	[Attribute("3", UIWidgets.EditBox, "Line width in UI pixels, independent of map zoom.", params: "1 20", category: "Outline")]
	protected float m_fLineWidth;
	[Attribute("0", UIWidgets.CheckBox, "Use the first listed faction's color for the outline. No matching faction uses Outline Color.", category: "Outline")]
	protected bool m_bFactionColor;
	[Attribute("1", UIWidgets.CheckBox, "Draw a uniform solid fill. Disable for outline only.", category: "Fill")]
	protected bool m_bFillEnabled;
	[Attribute("0", UIWidgets.CheckBox, "Fill outside the polygon instead of inside. The interior remains clear.", category: "Fill")]
	protected bool m_bInvertFill;
	[Attribute("0.2 0.5 1 0.2", UIWidgets.ColorPicker, "Solid fill color and opacity. Low opacity keeps terrain and grid readable.", category: "Fill")]
	protected ref Color m_FillColor;
	protected PolylineShapeEntity m_Shape;
	protected GC_AOLimitEntity m_AO;
	protected GC_MapShapeSystem m_System;
	protected ref array<vector> m_aRuntimePoints;

	//------------------------------------------------------------------------------------------------
	//! Trusted server API; existing and joining map viewers receive current state.
	bool SetEnabled(bool enabled)
	{
		if (!Replication.IsServer() || !m_System)
			return false;
		m_bEnabled = enabled;
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetFactionKeys(array<string> factions)
	{
		if (!Replication.IsServer() || !m_System)
			return false;
		m_aFactionKeys = {};
		if (factions)
			m_aFactionKeys.Copy(factions);
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetAutoFrameMap(bool enabled)
	{
		if (!Replication.IsServer() || !m_System)
			return false;
		m_bAutoFrameMap = enabled;
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetStyle(int lineColor, float width, bool factionColor, bool fillEnabled, int fillColor)
	{
		if (!Replication.IsServer() || !m_System || width < 1 || width > 20)
			return false;
		m_LineColor = Color.FromInt(lineColor);
		m_fLineWidth = width;
		m_bFactionColor = factionColor;
		m_bFillEnabled = fillEnabled;
		m_FillColor = Color.FromInt(fillColor);
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetInvertFill(bool inverted)
	{
		if (!Replication.IsServer() || !m_System)
			return false;
		m_bInvertFill = inverted;
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Standalone runtime geometry is copied in world coordinates. On an AO, use
	//! GC_AOLimitEntity.SetPolygon so enforcement and drawing share one polygon.
	bool SetPolygon(array<vector> points)
	{
		if (!Replication.IsServer() || !m_System || m_AO || !ValidPolygon(points))
			return false;
		m_aRuntimePoints = {};
		m_aRuntimePoints.Copy(points);
		m_System.Changed();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidPolygon(array<vector> points)
	{
		return GC_MapPolygon.IsValid(points);
	}

	//------------------------------------------------------------------------------------------------
	//! Recipient filtering happens on authority, before geometry is sent.
	void AppendSnapshot(int playerId, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<vector> frameBounds = null)
	{
		AppendFactionSnapshot(GC_LobbyManager.GetManager().GetPlayerFaction(playerId), counts, colors, fills, inverted, widths, points, frameBounds);
	}

	//------------------------------------------------------------------------------------------------
	//! Same presentation rules for recorded faction perspectives, without a connected player.
	void AppendFactionSnapshot(Faction faction, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<vector> frameBounds = null)
	{
		if (!m_bEnabled || !m_Shape)
			return;
		if (m_aFactionKeys && !m_aFactionKeys.IsEmpty())
		{
			if (!faction || !m_aFactionKeys.Contains(faction.GetFactionKey()))
				return;
		}
		array<vector> polygon = {};
		if (m_AO)
			m_AO.GetMapPolygon(polygon);
		else if (m_aRuntimePoints)
			polygon.Copy(m_aRuntimePoints);
		else
		{
			if (!m_Shape.IsClosed())
				return;
			m_Shape.GetPointsPositions(polygon);
			// The shared polygon contract rejects larger shapes. Check before
			// transforming every point of an invalid authored polyline.
			if (polygon.Count() > 128)
				return;
			for (int i; i < polygon.Count(); i++)
				polygon[i] = m_Shape.CoordToParent(polygon[i]);
		}
		if (!ValidPolygon(polygon))
			return;
		if (m_bAutoFrameMap && frameBounds)
		{
			if (frameBounds.IsEmpty())
			{
				frameBounds.Insert(polygon[0]);
				frameBounds.Insert(polygon[0]);
			}
			vector minimum = frameBounds[0];
			vector maximum = frameBounds[1];
			foreach (vector point : polygon)
			{
				minimum[0] = Math.Min(minimum[0], point[0]);
				minimum[2] = Math.Min(minimum[2], point[2]);
				maximum[0] = Math.Max(maximum[0], point[0]);
				maximum[2] = Math.Max(maximum[2], point[2]);
			}
			frameBounds[0] = minimum;
			frameBounds[1] = maximum;
		}
		Color lineColor = GetOutlineColor();
		int fill;
		if (m_bFillEnabled)
			fill = m_FillColor.PackToInt();
		counts.Insert(polygon.Count());
		colors.Insert(lineColor.PackToInt());
		fills.Insert(fill);
		inverted.Insert(m_bInvertFill);
		widths.Insert(Math.Clamp(m_fLineWidth, 1, 20));
		points.InsertAll(polygon);
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetOutlineColor()
	{
		Color lineColor = m_LineColor;
		if (m_bFactionColor)
		{
			string key;
			if (m_aFactionKeys && !m_aFactionKeys.IsEmpty())
				key = m_aFactionKeys[0];
			Faction colorFaction;
			FactionManager factions = GetGame().GetFactionManager();
			if (factions)
				colorFaction = factions.GetFactionByKey(key);
			if (colorFaction)
				lineColor = colorFaction.GetFactionColor();
		}
		return lineColor;
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterShape()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !Replication.IsServer())
			return;
		m_Shape = PolylineShapeEntity.Cast(GetOwner());
		m_AO = GC_AOLimitEntity.Cast(GetOwner());
		m_System = mode.GetMapShapes();
		if (m_Shape && m_System)
			m_System.Register(this);
		else
			Print("GC | Map shape | Requires a PolylineShapeEntity and active GC Lobby map system", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
#ifdef WORKBENCH
		if (!SCR_Global.IsEditMode())
			m_EditorPreview = null;
#endif
		if (!SCR_Global.IsEditMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(RegisterShape, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
#ifdef WORKBENCH
		m_EditorPreview = null;
#endif
		if (GetGame())
			GetGame().GetCallqueue().Remove(RegisterShape);
		if (m_System)
			m_System.Unregister(this);
		super.OnDelete(owner);
	}

#ifdef WORKBENCH
	protected ref GC_MapShapePreview m_EditorPreview;

	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		PolylineShapeEntity shape = PolylineShapeEntity.Cast(owner);
		if (!m_bEnabled || !GC_MapShapePreview.IsSelected(shape))
		{
			m_EditorPreview = null;
			return;
		}
		// Enabled authoring previews show the configured style regardless of audience.
		if (!m_EditorPreview)
			m_EditorPreview = new GC_MapShapePreview();
		int fill;
		if (m_bFillEnabled && m_FillColor)
			fill = m_FillColor.PackToInt();
		Color lineColor = GetOutlineColor();
		m_EditorPreview.Update(shape, lineColor.PackToInt(), m_fLineWidth, fill, m_bInvertFill);
	}
#endif
}

[ComponentEditorProps(category: "Lobby2/Mission", description: "Passive map polygon on a closed PolylineShapeEntity. Solid fill and outline; no gameplay restriction.")]
class GC_MapShapeComponentClass : ScriptComponentClass
{
}

class GC_MapShapeComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Show this shape on maps. Independent of AO enforcement.", category: "Map shape")]
	protected bool m_bEnabled;
	[Attribute("", UIWidgets.Auto, "Empty shows everyone; otherwise only these native faction keys can see the shape.", category: "Map shape")]
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
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetInvertFill(bool inverted)
	{
		if (!Replication.IsServer() || !m_System)
			return false;
		m_bInvertFill = inverted;
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
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidPolygon(array<vector> points)
	{
		if (!points || points.Count() < 3 || points.Count() > 128)
			return false;
		float area;
		for (int i; i < points.Count(); i++)
		{
			vector a = points[i] - points[0];
			vector b = points[(i + 1) % points.Count()] - points[0];
			area += a[0] * b[2] - b[0] * a[2];
		}
		return Math.AbsFloat(area) > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	//! Recipient filtering happens on authority, before geometry is sent.
	void AppendSnapshot(int playerId, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points)
	{
		if (!m_bEnabled || !m_Shape)
			return;
		if (m_aFactionKeys && !m_aFactionKeys.IsEmpty())
		{
			Faction faction = GC_LobbyManager.GetManager().GetPlayerFaction(playerId);
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
			for (int i; i < polygon.Count(); i++)
				polygon[i] = m_Shape.CoordToParent(polygon[i]);
		}
		if (!ValidPolygon(polygon))
			return;
		Color lineColor = m_LineColor;
		if (m_bFactionColor)
		{
			string key;
			if (m_aFactionKeys && !m_aFactionKeys.IsEmpty())
				key = m_aFactionKeys[0];
			Faction colorFaction = GetGame().GetFactionManager().GetFactionByKey(key);
			if (colorFaction)
				lineColor = colorFaction.GetFactionColor();
		}
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
			Print("GC | Map shape | Requires a PolylineShapeEntity and active Lobby2 map system", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!SCR_Global.IsEditMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(RegisterShape, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(RegisterShape);
		if (m_System)
			m_System.Unregister(this);
		super.OnDelete(owner);
	}
}

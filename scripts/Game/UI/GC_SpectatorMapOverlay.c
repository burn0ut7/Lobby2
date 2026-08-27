//! One 2D map symbol for a character proxy currently known to the local
//! editable-entity registry. Position, facing, and faction are always read
//! from the base-game entity instead of copied into Lobby2 state.
class GC_SpectatorMapCharacterSymbol
{
	protected static const ResourceName MARKER_LAYOUT = "{6A60000000000100}UI/layouts/Spectator/Lobby_Spectator_Map_Character.layout";
	protected static const float DOT_RADIUS = 5;
	protected static const float VIEWPORT_MARGIN = 128.0;

	protected SCR_EditableEntityComponent m_Entity;
	protected WorkspaceWidget m_Workspace;
	protected Widget m_wRoot;
	protected ImageWidget m_wDot;
	protected ImageWidget m_wDirection;
	protected int m_iScreenX;
	protected int m_iScreenY;
	protected float m_fHoverRadius;
	protected float m_fAppliedSize = -1.0;
	protected bool m_bVisible;
	protected bool m_bHasWorldState;
	protected vector m_vWorldPosition;
	protected vector m_vWorldFacingPosition;
	protected TNodeId m_iHeadBoneIndex = -1;
	protected Faction m_Faction;
	protected ref Color m_FactionColor;

	void GC_SpectatorMapCharacterSymbol(SCR_EditableEntityComponent entity, Widget markerParent, WorkspaceWidget workspace)
	{
		m_Entity = entity;
		m_Workspace = workspace;
		if (!markerParent || !m_Workspace)
			return;

		m_wRoot = m_Workspace.CreateWidgets(MARKER_LAYOUT, markerParent);
		if (m_wRoot)
		{
			m_wDot = ImageWidget.Cast(m_wRoot.FindAnyWidget("LobbySpectatorMapCharacterDot"));
			m_wDirection = ImageWidget.Cast(m_wRoot.FindAnyWidget("LobbySpectatorMapCharacterDirection"));
		}
		if ((!m_wDot || !m_wDirection) && m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}

	bool IsReady()
	{
		return m_wRoot && m_wDot && m_wDirection;
	}

	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_wDot = null;
		m_wDirection = null;
		m_Workspace = null;
		m_Entity = null;
	}

	void SampleWorldState()
	{
		m_bHasWorldState = false;
		if (!m_Entity)
		{
			Hide();
			return;
		}

		IEntity owner = m_Entity.GetOwner();
		if (!owner || owner.IsDeleted())
		{
			Hide();
			return;
		}

		vector transform[4];
		owner.GetTransform(transform);
		Faction faction = m_Entity.GetFaction();
		if (!faction)
		{
			Hide();
			return;
		}
		if (faction != m_Faction)
		{
			m_Faction = faction;
			m_FactionColor = faction.GetFactionColor();
			m_wDot.SetColor(m_FactionColor);
			m_wDirection.SetColor(m_FactionColor);
		}

		Animation animation = owner.GetAnimation();
		if (!animation)
		{
			Hide();
			return;
		}

		if (m_iHeadBoneIndex < 0)
			m_iHeadBoneIndex = animation.GetBoneIndex("Head");
		vector headBoneTransform[4];
		if (m_iHeadBoneIndex < 0 || !animation.GetBoneMatrix(m_iHeadBoneIndex, headBoneTransform))
		{
			Hide();
			return;
		}

		vector headWorldTransform[4];
		Math3D.MatrixMultiply4(transform, headBoneTransform, headWorldTransform);
		// Character head bones face opposite the camera/look direction. Flatten the
		// animated look basis onto the map so pitch does not change arrow length.
		vector aimDirection = headWorldTransform[2] * -1.0;
		aimDirection[1] = 0;
		if (aimDirection.LengthSq() <= 0.0001)
		{
			Hide();
			return;
		}
		aimDirection.Normalize();
		m_vWorldPosition = transform[3];
		m_vWorldFacingPosition = m_vWorldPosition + aimDirection * 10.0;
		m_bHasWorldState = true;
	}

	bool ProjectToScreen(SCR_MapEntity mapEntity, float displayScale, float viewportX, float viewportY, float viewportWidth, float viewportHeight)
	{
		if (!mapEntity || !m_bHasWorldState)
		{
			Hide();
			return false;
		}

		int screenX;
		int screenY;
		mapEntity.WorldToScreen(m_vWorldPosition[0], m_vWorldPosition[2], screenX, screenY, true);
		if (screenX < viewportX - VIEWPORT_MARGIN || screenX > viewportX + viewportWidth + VIEWPORT_MARGIN
			|| screenY < viewportY - VIEWPORT_MARGIN || screenY > viewportY + viewportHeight + VIEWPORT_MARGIN)
		{
			Hide();
			return false;
		}

		int facingScreenX;
		int facingScreenY;
		mapEntity.WorldToScreen(m_vWorldFacingPosition[0], m_vWorldFacingPosition[2], facingScreenX, facingScreenY, true);

		float directionX = facingScreenX - screenX;
		float directionY = facingScreenY - screenY;
		float directionLength = Math.Sqrt(directionX * directionX + directionY * directionY);
		if (directionLength <= 0.001)
		{
			directionX = 0;
			directionY = -1;
		}
		else
		{
			directionX /= directionLength;
			directionY /= directionLength;
		}

		float dotRadius = DOT_RADIUS * displayScale;
		float widgetSize = m_Workspace.DPIUnscale(dotRadius * 2.0);
		if (Math.AbsFloat(widgetSize - m_fAppliedSize) >= 0.01)
		{
			m_fAppliedSize = widgetSize;
			FrameSlot.SetSize(m_wRoot, widgetSize, widgetSize);
			FrameSlot.SetPos(m_wDot, 0.0, 0.0);
			FrameSlot.SetSize(m_wDot, widgetSize, widgetSize);
			FrameSlot.SetPos(m_wDirection, 0.0, 0.0);
			FrameSlot.SetSize(m_wDirection, widgetSize, widgetSize);
		}
		FrameSlot.SetPos(m_wRoot, m_Workspace.DPIUnscale(screenX) - widgetSize * 0.5, m_Workspace.DPIUnscale(screenY) - widgetSize * 0.5);
		m_wDirection.SetRotation(Math.Atan2(directionY, directionX) * Math.RAD2DEG + 90.0);
		m_iScreenX = screenX;
		m_iScreenY = screenY;
		m_fHoverRadius = dotRadius + 6.0;
		if (!m_bVisible)
		{
			m_bVisible = true;
			m_wRoot.SetVisible(true);
		}
		return true;
	}

	float GetCursorDistanceSq(int cursorX, int cursorY)
	{
		if (!m_bVisible)
			return -1;

		float offsetX = cursorX - m_iScreenX;
		float offsetY = cursorY - m_iScreenY;
		float distanceSq = offsetX * offsetX + offsetY * offsetY;
		if (distanceSq > m_fHoverRadius * m_fHoverRadius)
			return -1;

		return distanceSq;
	}

	string GetHoverLabel()
	{
		if (!m_Entity)
			return string.Empty;

		IEntity owner = m_Entity.GetOwner();
		if (!owner)
			return string.Empty;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(owner);
			if (playerId > 0)
			{
				string playerName = playerManager.GetPlayerName(playerId);
				if (!playerName.IsEmpty())
					return playerName;
			}
		}

		SCR_UIInfo info = m_Entity.GetInfo(owner);
		SCR_EditableEntityUIInfo editableInfo = SCR_EditableEntityUIInfo.Cast(info);
		if (editableInfo)
			return editableInfo.GetName();

		return string.Empty;
	}

	IEntity GetTargetEntity()
	{
		if (!m_Entity)
			return null;

		IEntity owner = m_Entity.GetOwner();
		if (!owner || owner.IsDeleted())
			return null;

		return owner;
	}

	void Hide()
	{
		if (!m_bVisible)
			return;

		m_bVisible = false;
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}
}

//! Client-local 2D spectator map presentation. The editable-entity registry is
//! used as the local list of streamed replication proxies; no entity, player,
//! AI, faction, or affiliation state is owned or replicated by this renderer.
class GC_SpectatorMapOverlay
{
	protected static const float WORLD_STATE_SAMPLE_INTERVAL = 0.2;
	protected static const float MIN_ZOOM_SCALE = 2.0;
	protected static const float MAX_ZOOM_SCALE = 20.0;

	protected CanvasWidget m_Canvas;
	protected Widget m_wMarkerParent;
	protected Widget m_wTooltip;
	protected TextWidget m_wTooltipText;
	protected WorkspaceWidget m_Workspace;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_EditableEntityCore m_EditableEntityCore;
	protected GC_SpectatorProjectileTracer m_ProjectileTracer;
	protected IEntity m_HoveredEntity;
	protected bool m_bTooltipVisible;
	protected float m_fWorldStateSampleAccumulator;
	protected ref map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol> m_mSymbols = new map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol>();
	protected ref array<ref GC_SpectatorMapCharacterSymbol> m_aVisibleSymbols = {};
	protected ref array<ref CanvasWidgetCommand> m_aFrameDrawCommands = {};

	bool Start(CanvasWidget canvas, Widget tooltip, TextWidget tooltipText)
	{
		if (!canvas || !tooltip || !tooltipText)
			return false;

		m_Canvas = canvas;
		m_wMarkerParent = m_Canvas.GetParent();
		m_wTooltip = tooltip;
		m_wTooltipText = tooltipText;
		m_Workspace = GetGame().GetWorkspace();
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		m_EditableEntityCore = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!m_wMarkerParent || !m_Workspace || !m_MapEntity || !m_EditableEntityCore)
			return false;

		m_EditableEntityCore.Event_OnEntityRegistered.Insert(OnEntityRegistered);
		m_EditableEntityCore.Event_OnEntityUnregistered.Insert(OnEntityUnregistered);

		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		m_EditableEntityCore.GetAllEntities(entities);
		foreach (SCR_EditableEntityComponent entity : entities)
			OnEntityRegistered(entity);

		PrintFormat("GC | Spectator Map | overlay ready characters=%1", m_mSymbols.Count());
		return true;
	}

	void Destroy()
	{
		if (m_EditableEntityCore)
		{
			m_EditableEntityCore.Event_OnEntityRegistered.Remove(OnEntityRegistered);
			m_EditableEntityCore.Event_OnEntityUnregistered.Remove(OnEntityUnregistered);
		}

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorMapCharacterSymbol symbol : m_mSymbols)
			symbol.Destroy();
		m_aVisibleSymbols.Clear();
		m_aFrameDrawCommands.Clear();
		if (m_Canvas)
			m_Canvas.SetDrawCommands(m_aFrameDrawCommands);
		m_mSymbols.Clear();
		m_HoveredEntity = null;
		m_bTooltipVisible = false;
		m_Canvas = null;
		m_wMarkerParent = null;
		m_wTooltip = null;
		m_wTooltipText = null;
		m_Workspace = null;
		m_MapEntity = null;
		m_EditableEntityCore = null;
		m_ProjectileTracer = null;
	}

	void SetProjectileTracer(GC_SpectatorProjectileTracer projectileTracer)
	{
		m_ProjectileTracer = projectileTracer;
	}

	void Update(float timeSlice, bool force = false)
	{
		if (!m_Canvas || !m_MapEntity || !m_MapEntity.IsOpen())
			return;

		m_fWorldStateSampleAccumulator += timeSlice;
		if (force || m_fWorldStateSampleAccumulator >= WORLD_STATE_SAMPLE_INTERVAL)
		{
			m_fWorldStateSampleAccumulator = 0.0;
			SampleWorldStates();
		}
		ProjectFrame();

		int cursorX;
		int cursorY;
		WidgetManager.GetMousePos(cursorX, cursorY);
		float closestDistanceSq = 1000000000.0;
		GC_SpectatorMapCharacterSymbol hoveredSymbol;
		foreach (GC_SpectatorMapCharacterSymbol symbol : m_aVisibleSymbols)
		{
			float cursorDistanceSq = symbol.GetCursorDistanceSq(cursorX, cursorY);
			if (cursorDistanceSq >= 0 && cursorDistanceSq < closestDistanceSq)
			{
				closestDistanceSq = cursorDistanceSq;
				hoveredSymbol = symbol;
			}
		}

		IEntity hoveredEntity;
		if (hoveredSymbol)
			hoveredEntity = hoveredSymbol.GetTargetEntity();
		if (hoveredEntity != m_HoveredEntity)
		{
			m_HoveredEntity = hoveredEntity;
			string hoverLabel;
			if (hoveredSymbol && hoveredEntity)
				hoverLabel = hoveredSymbol.GetHoverLabel();
			m_bTooltipVisible = !hoverLabel.IsEmpty();
			m_wTooltip.SetVisible(m_bTooltipVisible);
			if (m_bTooltipVisible)
				m_wTooltipText.SetText(hoverLabel);
		}
		if (!m_bTooltipVisible)
			return;

		float tooltipX = m_Workspace.DPIUnscale(cursorX) + 14.0;
		float tooltipY = m_Workspace.DPIUnscale(cursorY) + 14.0;
		if (cursorX > m_Workspace.GetWidth() * 0.75)
			tooltipX -= 240.0;
		if (cursorY > m_Workspace.GetHeight() * 0.85)
			tooltipY -= 50.0;
		FrameSlot.SetPos(m_wTooltip, tooltipX, tooltipY);
	}

	protected void SampleWorldStates()
	{
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorMapCharacterSymbol symbol : m_mSymbols)
			symbol.SampleWorldState();
	}

	protected void ProjectFrame()
	{
		float viewportX;
		float viewportY;
		float viewportWidth;
		float viewportHeight;
		m_Canvas.GetScreenPos(viewportX, viewportY);
		m_Canvas.GetScreenSize(viewportWidth, viewportHeight);
		if (viewportWidth <= 0.0 || viewportHeight <= 0.0)
		{
			viewportX = 0.0;
			viewportY = 0.0;
			viewportWidth = m_Workspace.GetWidth();
			viewportHeight = m_Workspace.GetHeight();
		}

		// GetCurrentZoom is pixels per world meter. Square-root scaling keeps
		// symbols responsive to zoom without allowing extreme map zoom levels
		// to make them disappear or cover a large area of the map.
		float displayScale = Math.Clamp(Math.Sqrt(m_MapEntity.GetCurrentZoom()), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
		m_aVisibleSymbols.Clear();
		m_aFrameDrawCommands.Clear();
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorMapCharacterSymbol symbol : m_mSymbols)
		{
			if (!symbol.ProjectToScreen(m_MapEntity, displayScale, viewportX, viewportY, viewportWidth, viewportHeight))
				continue;

			m_aVisibleSymbols.Insert(symbol);
		}
		if (m_ProjectileTracer)
			m_ProjectileTracer.AppendMapDrawCommands(m_MapEntity, m_aFrameDrawCommands);
		m_Canvas.SetDrawCommands(m_aFrameDrawCommands);
	}

	IEntity GetHoveredEntity()
	{
		if (!m_HoveredEntity || m_HoveredEntity.IsDeleted())
			return null;

		return m_HoveredEntity;
	}

	void Clear()
	{
		if (!m_Canvas)
			return;

		array<ref CanvasWidgetCommand> noCommands = {};
		m_Canvas.SetDrawCommands(noCommands);
		m_fWorldStateSampleAccumulator = 0.0;
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorMapCharacterSymbol symbol : m_mSymbols)
			symbol.Hide();
		m_aVisibleSymbols.Clear();
		m_aFrameDrawCommands.Clear();
		m_HoveredEntity = null;
		m_bTooltipVisible = false;
		if (m_wTooltip)
			m_wTooltip.SetVisible(false);
	}

	protected void OnEntityRegistered(SCR_EditableEntityComponent entity)
	{
		if (!entity || m_mSymbols.Contains(entity))
			return;
		if (entity.GetEntityType() != EEditableEntityType.CHARACTER)
			return;
		if (!SCR_EditableCharacterComponent.Cast(entity))
			return;

		IEntity owner = entity.GetOwner();
		if (!owner || owner.FindComponent(GC_LobbySpectatorBodyComponent))
			return;

		GC_SpectatorMapCharacterSymbol symbol = new GC_SpectatorMapCharacterSymbol(entity, m_wMarkerParent, m_Workspace);
		if (!symbol.IsReady())
		{
			symbol.Destroy();
			return;
		}
		m_mSymbols.Insert(entity, symbol);
		m_fWorldStateSampleAccumulator = WORLD_STATE_SAMPLE_INTERVAL;
	}

	protected void OnEntityUnregistered(SCR_EditableEntityComponent entity)
	{
		GC_SpectatorMapCharacterSymbol symbol;
		if (!entity || !m_mSymbols.Find(entity, symbol))
			return;

		m_mSymbols.Remove(entity);
		m_aVisibleSymbols.RemoveItem(symbol);
		symbol.Destroy();
		m_fWorldStateSampleAccumulator = WORLD_STATE_SAMPLE_INTERVAL;
	}
}

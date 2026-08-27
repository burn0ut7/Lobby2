//! One 2D map symbol for a character proxy currently known to the local
//! editable-entity registry. Position, facing, and faction are always read
//! from the base-game entity instead of copied into Lobby2 state.
class GC_SpectatorMapCharacterSymbol
{
	protected static const float DOT_RADIUS = 5;
	protected static const float ARROW_GAP = 1.0;
	protected static const float ARROW_LENGTH = 3.0;
	protected static const float ARROW_HALF_WIDTH = 1.75;
	protected static const float VIEWPORT_MARGIN = 128.0;

	protected SCR_EditableEntityComponent m_Entity;
	protected ref PolygonDrawCommand m_Dot = new PolygonDrawCommand();
	protected ref PolygonDrawCommand m_Arrow = new PolygonDrawCommand();
	protected int m_iScreenX;
	protected int m_iScreenY;
	protected float m_fHoverRadius;
	protected bool m_bVisible;
	protected TNodeId m_iHeadBoneIndex = -1;
	protected Faction m_Faction;
	protected int m_iFactionColor;

	void GC_SpectatorMapCharacterSymbol(SCR_EditableEntityComponent entity)
	{
		m_Entity = entity;
		m_Dot.m_Vertices = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		m_Arrow.m_Vertices = {0, 0, 0, 0, 0, 0};
	}

	PolygonDrawCommand GetDotCommand()
	{
		return m_Dot;
	}

	PolygonDrawCommand GetArrowCommand()
	{
		return m_Arrow;
	}

	bool Update(SCR_MapEntity mapEntity, float displayScale, float viewportX, float viewportY, float viewportWidth, float viewportHeight)
	{
		if (!mapEntity || !m_Entity)
		{
			Hide();
			return false;
		}

		IEntity owner = m_Entity.GetOwner();
		if (!owner || owner.IsDeleted())
		{
			Hide();
			return false;
		}

		vector transform[4];
		owner.GetTransform(transform);
		vector position = transform[3];
		int screenX;
		int screenY;
		mapEntity.WorldToScreen(position[0], position[2], screenX, screenY, true);
		if (screenX < viewportX - VIEWPORT_MARGIN || screenX > viewportX + viewportWidth + VIEWPORT_MARGIN
			|| screenY < viewportY - VIEWPORT_MARGIN || screenY > viewportY + viewportHeight + VIEWPORT_MARGIN)
		{
			Hide();
			return false;
		}

		Faction faction = m_Entity.GetFaction();
		if (!faction)
		{
			Hide();
			return false;
		}
		if (faction != m_Faction)
		{
			m_Faction = faction;
			m_iFactionColor = faction.GetFactionColor().PackToInt();
		}

		Animation animation = owner.GetAnimation();
		if (!animation)
		{
			Hide();
			return false;
		}

		if (m_iHeadBoneIndex < 0)
			m_iHeadBoneIndex = animation.GetBoneIndex("Head");
		vector headBoneTransform[4];
		if (m_iHeadBoneIndex < 0 || !animation.GetBoneMatrix(m_iHeadBoneIndex, headBoneTransform))
		{
			Hide();
			return false;
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
			return false;
		}
		aimDirection.Normalize();
		vector facingPosition = position + aimDirection * 10.0;
		int facingScreenX;
		int facingScreenY;
		mapEntity.WorldToScreen(facingPosition[0], facingPosition[2], facingScreenX, facingScreenY, true);

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

		float perpendicularX = -directionY;
		float perpendicularY = directionX;
		float dotRadius = DOT_RADIUS * displayScale;
		float diagonalRadius = dotRadius * 0.7071;
		float arrowBaseDistance = dotRadius + ARROW_GAP * displayScale;
		float arrowTipDistance = arrowBaseDistance + ARROW_LENGTH * displayScale;
		float arrowHalfWidth = ARROW_HALF_WIDTH * displayScale;
		array<float> dotVertices = m_Dot.m_Vertices;
		dotVertices[0] = screenX + dotRadius;
		dotVertices[1] = screenY;
		dotVertices[2] = screenX + diagonalRadius;
		dotVertices[3] = screenY + diagonalRadius;
		dotVertices[4] = screenX;
		dotVertices[5] = screenY + dotRadius;
		dotVertices[6] = screenX - diagonalRadius;
		dotVertices[7] = screenY + diagonalRadius;
		dotVertices[8] = screenX - dotRadius;
		dotVertices[9] = screenY;
		dotVertices[10] = screenX - diagonalRadius;
		dotVertices[11] = screenY - diagonalRadius;
		dotVertices[12] = screenX;
		dotVertices[13] = screenY - dotRadius;
		dotVertices[14] = screenX + diagonalRadius;
		dotVertices[15] = screenY - diagonalRadius;

		array<float> arrowVertices = m_Arrow.m_Vertices;
		arrowVertices[0] = screenX + directionX * arrowTipDistance;
		arrowVertices[1] = screenY + directionY * arrowTipDistance;
		arrowVertices[2] = screenX + directionX * arrowBaseDistance + perpendicularX * arrowHalfWidth;
		arrowVertices[3] = screenY + directionY * arrowBaseDistance + perpendicularY * arrowHalfWidth;
		arrowVertices[4] = screenX + directionX * arrowBaseDistance - perpendicularX * arrowHalfWidth;
		arrowVertices[5] = screenY + directionY * arrowBaseDistance - perpendicularY * arrowHalfWidth;

		m_Dot.m_iColor = m_iFactionColor;
		m_Arrow.m_iColor = m_iFactionColor;
		m_iScreenX = screenX;
		m_iScreenY = screenY;
		m_fHoverRadius = dotRadius + 6.0;
		m_bVisible = true;
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

	protected void Hide()
	{
		m_bVisible = false;
	}
}

//! Client-local 2D spectator map presentation. The editable-entity registry is
//! used as the local list of streamed replication proxies; no entity, player,
//! AI, faction, or affiliation state is owned or replicated by this renderer.
class GC_SpectatorMapOverlay
{
	protected static const float CHARACTER_UPDATE_INTERVAL = 0.2;
	protected static const float MIN_ZOOM_SCALE = 2.0;
	protected static const float MAX_ZOOM_SCALE = 20.0;

	protected CanvasWidget m_Canvas;
	protected Widget m_wTooltip;
	protected TextWidget m_wTooltipText;
	protected WorkspaceWidget m_Workspace;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_EditableEntityCore m_EditableEntityCore;
	protected GC_SpectatorProjectileTracer m_ProjectileTracer;
	protected IEntity m_HoveredEntity;
	protected bool m_bTooltipVisible;
	protected float m_fCharacterUpdateAccumulator;
	protected ref map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol> m_mSymbols = new map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol>();
	protected ref array<ref GC_SpectatorMapCharacterSymbol> m_aVisibleSymbols = {};
	protected ref array<ref CanvasWidgetCommand> m_aFrameDrawCommands = {};

	bool Start(CanvasWidget canvas, Widget tooltip, TextWidget tooltipText)
	{
		if (!canvas || !tooltip || !tooltipText)
			return false;

		m_Canvas = canvas;
		m_wTooltip = tooltip;
		m_wTooltipText = tooltipText;
		m_Workspace = GetGame().GetWorkspace();
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		m_EditableEntityCore = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!m_Workspace || !m_MapEntity || !m_EditableEntityCore)
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

		m_aVisibleSymbols.Clear();
		m_aFrameDrawCommands.Clear();
		if (m_Canvas)
			m_Canvas.SetDrawCommands(m_aFrameDrawCommands);
		m_mSymbols.Clear();
		m_HoveredEntity = null;
		m_bTooltipVisible = false;
		m_Canvas = null;
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

		m_fCharacterUpdateAccumulator += timeSlice;
		if (force || m_fCharacterUpdateAccumulator >= CHARACTER_UPDATE_INTERVAL)
		{
			m_fCharacterUpdateAccumulator = 0.0;
			RefreshSymbols();
		}

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

	protected void RefreshSymbols()
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
			if (!symbol.Update(m_MapEntity, displayScale, viewportX, viewportY, viewportWidth, viewportHeight))
				continue;

			m_aVisibleSymbols.Insert(symbol);
			m_aFrameDrawCommands.Insert(symbol.GetDotCommand());
			m_aFrameDrawCommands.Insert(symbol.GetArrowCommand());
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
		m_fCharacterUpdateAccumulator = 0.0;
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

		GC_SpectatorMapCharacterSymbol symbol = new GC_SpectatorMapCharacterSymbol(entity);
		m_mSymbols.Insert(entity, symbol);
		m_fCharacterUpdateAccumulator = CHARACTER_UPDATE_INTERVAL;
	}

	protected void OnEntityUnregistered(SCR_EditableEntityComponent entity)
	{
		GC_SpectatorMapCharacterSymbol symbol;
		if (!entity || !m_mSymbols.Find(entity, symbol))
			return;

		m_mSymbols.Remove(entity);
		m_aVisibleSymbols.RemoveItem(symbol);
		m_fCharacterUpdateAccumulator = CHARACTER_UPDATE_INTERVAL;
	}
}

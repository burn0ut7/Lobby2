//! One 2D map symbol for a character proxy currently known to the local
//! editable-entity registry. Position, facing, and faction are always read
//! from the base-game entity instead of copied into Lobby2 state.
class GC_SpectatorMapCharacterSymbol
{
	protected static const float DOT_RADIUS = 5.6;
	protected static const float ARROW_GAP = 1.0;
	protected static const float ARROW_LENGTH = 3.0;
	protected static const float ARROW_HALF_WIDTH = 1.75;
	protected static const float MIN_ZOOM_SCALE = 0.8;
	protected static const float MAX_ZOOM_SCALE = 2.0;

	protected SCR_EditableEntityComponent m_Entity;
	protected ref PolygonDrawCommand m_Dot = new PolygonDrawCommand();
	protected ref PolygonDrawCommand m_Arrow = new PolygonDrawCommand();
	protected int m_iScreenX;
	protected int m_iScreenY;
	protected float m_fHoverRadius;
	protected bool m_bVisible;

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

	void Update(SCR_MapEntity mapEntity)
	{
		if (!mapEntity || !m_Entity)
		{
			Hide();
			return;
		}

		IEntity owner = m_Entity.GetOwner();
		Faction faction = m_Entity.GetFaction();
		if (!owner || owner.IsDeleted() || !faction)
		{
			Hide();
			return;
		}

		vector transform[4];
		owner.GetTransform(transform);
		vector position = transform[3];
		Animation animation = owner.GetAnimation();
		if (!animation)
		{
			Hide();
			return;
		}

		vector headBoneTransform[4];
		if (!animation.GetBoneMatrix(animation.GetBoneIndex("Head"), headBoneTransform))
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
		vector facingPosition = position + aimDirection * 10.0;
		int screenX;
		int screenY;
		int facingScreenX;
		int facingScreenY;
		mapEntity.WorldToScreen(position[0], position[2], screenX, screenY, true);
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
		// GetCurrentZoom is pixels per world meter. Square-root scaling keeps
		// symbols responsive to zoom without allowing extreme map zoom levels
		// to make them disappear or cover a large area of the map.
		float displayScale = Math.Clamp(Math.Sqrt(mapEntity.GetCurrentZoom()), MIN_ZOOM_SCALE, MAX_ZOOM_SCALE);
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

		int factionColor = faction.GetFactionColor().PackToInt();
		m_Dot.m_iColor = factionColor;
		m_Arrow.m_iColor = factionColor;
		m_iScreenX = screenX;
		m_iScreenY = screenY;
		m_fHoverRadius = dotRadius + 6.0;
		m_bVisible = true;
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
		for (int dotIndex; dotIndex < m_Dot.m_Vertices.Count(); dotIndex++)
			m_Dot.m_Vertices[dotIndex] = -100;
		for (int arrowIndex; arrowIndex < m_Arrow.m_Vertices.Count(); arrowIndex++)
			m_Arrow.m_Vertices[arrowIndex] = -100;
	}
}

//! Client-local 2D spectator map presentation. The editable-entity registry is
//! used as the local list of streamed replication proxies; no entity, player,
//! AI, faction, or affiliation state is owned or replicated by this renderer.
class GC_SpectatorMapOverlay
{
	protected CanvasWidget m_Canvas;
	protected Widget m_wTooltip;
	protected TextWidget m_wTooltipText;
	protected WorkspaceWidget m_Workspace;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_EditableEntityCore m_EditableEntityCore;
	protected GC_SpectatorProjectileTracer m_ProjectileTracer;
	protected IEntity m_HoveredEntity;
	protected ref map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol> m_mSymbols = new map<SCR_EditableEntityComponent, ref GC_SpectatorMapCharacterSymbol>();
	protected ref array<ref CanvasWidgetCommand> m_aSymbolDrawCommands = {};
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

		m_aSymbolDrawCommands.Clear();
		m_aFrameDrawCommands.Clear();
		if (m_Canvas)
			m_Canvas.SetDrawCommands(m_aFrameDrawCommands);
		m_mSymbols.Clear();
		m_HoveredEntity = null;
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

	void Update()
	{
		if (!m_Canvas || !m_MapEntity || !m_MapEntity.IsOpen())
			return;

		int cursorX;
		int cursorY;
		WidgetManager.GetMousePos(cursorX, cursorY);
		float closestDistanceSq = 1000000000.0;
		GC_SpectatorMapCharacterSymbol hoveredSymbol;
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorMapCharacterSymbol symbol : m_mSymbols)
		{
			symbol.Update(m_MapEntity);
			float cursorDistanceSq = symbol.GetCursorDistanceSq(cursorX, cursorY);
			if (cursorDistanceSq >= 0 && cursorDistanceSq < closestDistanceSq)
			{
				closestDistanceSq = cursorDistanceSq;
				hoveredSymbol = symbol;
			}
		}

		m_aFrameDrawCommands = {};
		foreach (CanvasWidgetCommand symbolCommand : m_aSymbolDrawCommands)
			m_aFrameDrawCommands.Insert(symbolCommand);
		if (m_ProjectileTracer)
			m_ProjectileTracer.AppendMapDrawCommands(m_MapEntity, m_aFrameDrawCommands);
		m_Canvas.SetDrawCommands(m_aFrameDrawCommands);
		m_HoveredEntity = null;
		string hoverLabel;
		if (hoveredSymbol)
		{
			m_HoveredEntity = hoveredSymbol.GetTargetEntity();
			hoverLabel = hoveredSymbol.GetHoverLabel();
		}
		if (hoverLabel.IsEmpty())
		{
			m_wTooltip.SetVisible(false);
			return;
		}

		m_wTooltipText.SetText(hoverLabel);
		float tooltipX = m_Workspace.DPIUnscale(cursorX) + 14.0;
		float tooltipY = m_Workspace.DPIUnscale(cursorY) + 14.0;
		if (cursorX > m_Workspace.GetWidth() * 0.75)
			tooltipX -= 240.0;
		if (cursorY > m_Workspace.GetHeight() * 0.85)
			tooltipY -= 50.0;
		FrameSlot.SetPos(m_wTooltip, tooltipX, tooltipY);
		m_wTooltip.SetVisible(true);
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
		m_HoveredEntity = null;
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
		m_aSymbolDrawCommands.Insert(symbol.GetDotCommand());
		m_aSymbolDrawCommands.Insert(symbol.GetArrowCommand());
	}

	protected void OnEntityUnregistered(SCR_EditableEntityComponent entity)
	{
		GC_SpectatorMapCharacterSymbol symbol;
		if (!entity || !m_mSymbols.Find(entity, symbol))
			return;

		m_mSymbols.Remove(entity);
		m_aSymbolDrawCommands.Clear();
		foreach (SCR_EditableEntityComponent remainingEntity, GC_SpectatorMapCharacterSymbol remainingSymbol : m_mSymbols)
		{
			m_aSymbolDrawCommands.Insert(remainingSymbol.GetDotCommand());
			m_aSymbolDrawCommands.Insert(remainingSymbol.GetArrowCommand());
		}
	}
}

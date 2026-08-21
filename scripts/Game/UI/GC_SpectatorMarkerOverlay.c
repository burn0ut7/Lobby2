//! Lobby2 presentation layered on top of the native editable-character widget.
//! The base component remains responsible for resolving the character role icon.
class GC_SpectatorCharacterEditableEntityUIComponent : SCR_CharacterEditableEntityUIComponent
{
	protected static const float MIN_ICON_DISTANCE = 0.0;
	protected static const float MAX_ICON_SIZE = 32.0;
	protected static const float MIN_ICON_SIZE = 4.0;

	protected ImageWidget m_wSmallBackground;
	protected ImageWidget m_wBackground;
	protected ImageWidget m_wOutline;
	protected ImageWidget m_wIcon;
	protected ImageWidget m_wSelectedIcon;
	protected ImageWidget m_wStatusIcon;
	protected ButtonWidget m_wButton;
	protected PanelWidget m_wLabel;
	protected RichTextWidget m_wLabelText;
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected string m_sFallbackName;
	protected bool m_bHovered;
	protected bool m_bStateInitialized;
	protected bool m_bDead;
	protected bool m_bUnconscious;
	protected int m_iFactionColor;
	protected int m_iOutlineColor;

	override void OnInit(SCR_EditableEntityComponent entity, SCR_UIInfo info, SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnInit(entity, info, slot);

		m_wSmallBackground = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerSmallBackground"));
		m_wBackground = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerBackground"));
		m_wOutline = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerOutline"));
		m_wIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerRoleIcon"));
		m_wSelectedIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerSelectedIcon"));
		m_wStatusIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerWoundedIcon"));
		m_wButton = ButtonWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerButton"));
		m_wLabel = PanelWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerLabel"));
		m_wLabelText = RichTextWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerLabelText"));
		if (m_wLabel)
			m_wLabel.SetOpacity(0.0);
		if (m_wSelectedIcon)
			m_wSelectedIcon.SetOpacity(0.0);

		if (!entity || !entity.GetOwner())
			return;

		// Players and AI both use the authoritative role icon supplied by the
		// editable character. The native player override is deliberately replaced
		// here because the spectator needs role identity for both entity states.
		if (info && m_wIcon)
			info.SetIconTo(m_wIcon);

		SCR_EditableEntityUIInfo editableInfo = SCR_EditableEntityUIInfo.Cast(info);
		if (editableInfo)
			m_sFallbackName = editableInfo.GetName();

		m_CharacterController = SCR_CharacterControllerComponent.Cast(entity.GetOwner().FindComponent(SCR_CharacterControllerComponent));
		RefreshFaction(entity.GetFaction());
		RefreshCharacterState();
	}

	void UpdateDistancePresentation(float distance, float maxDistance)
	{
		if (!m_wIcon)
			return;

		float distanceScale = 1.0 - ((distance - MIN_ICON_DISTANCE) / (maxDistance - MIN_ICON_DISTANCE));
		distanceScale = Math.Clamp(distanceScale, 0.0, 1.0);
		distanceScale = distanceScale * distanceScale * distanceScale;
		float iconSize = MIN_ICON_SIZE + (MAX_ICON_SIZE - MIN_ICON_SIZE) * distanceScale;
		float iconOffset = -iconSize * 0.5;

		FrameSlot.SetSize(m_wIcon, iconSize, iconSize);
		FrameSlot.SetPos(m_wIcon, iconOffset, iconOffset);
		if (m_wSelectedIcon)
		{
			FrameSlot.SetSize(m_wSelectedIcon, iconSize, iconSize);
			FrameSlot.SetPos(m_wSelectedIcon, iconOffset, iconOffset);

			float highlightOpacity = m_wSelectedIcon.GetOpacity();
			float highlightStep = GetGame().GetWorld().GetTimeSlice() * 5.0;
			if (m_bHovered)
				highlightOpacity += highlightStep;
			else
				highlightOpacity -= highlightStep;

			m_wSelectedIcon.SetOpacity(Math.Clamp(highlightOpacity, 0.0, 1.0));
		}

		if (m_wButton)
		{
			FrameSlot.SetSize(m_wButton, iconSize, iconSize);
			FrameSlot.SetPos(m_wButton, iconOffset, iconOffset);
		}

		if (m_wLabel)
		{
			float labelWidth;
			float labelHeight;
			m_wLabel.GetScreenSize(labelWidth, labelHeight);
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (workspace)
				FrameSlot.SetPos(m_wLabel, -workspace.DPIUnscale(labelWidth) * 0.5, -iconSize);
		}
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bHovered = true;
		bool hasLabel = RefreshHoverLabel();
		if (m_wLabel && hasLabel)
			m_wLabel.SetOpacity(1.0);

		GetWidget().SetZOrder(100);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bHovered = false;
		if (m_wLabel)
			m_wLabel.SetOpacity(0.0);

		GetWidget().SetZOrder(0);
		return true;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 1 || !m_Entity || !m_Entity.GetOwner())
			return false;

		GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForEntity(m_Entity.GetOwner()), GetWidget());
		return true;
	}

	override void OnRefresh(SCR_EditableEntityBaseSlotUIComponent slot)
	{
		if (m_Entity)
			RefreshFaction(m_Entity.GetFaction());

		RefreshCharacterState();
		if (m_bHovered)
			RefreshHoverLabel();
	}

	void RefreshCharacterState()
	{
		if (!m_CharacterController)
			return;

		bool dead = m_CharacterController.IsDead();
		bool unconscious = !dead && m_CharacterController.IsUnconscious();
		if (m_bStateInitialized && dead == m_bDead && unconscious == m_bUnconscious)
			return;

		m_bStateInitialized = true;
		m_bDead = dead;
		m_bUnconscious = unconscious;
		ApplyStatePresentation();
	}

	protected void RefreshFaction(Faction faction)
	{
		if (!faction)
			return;

		m_iFactionColor = faction.GetFactionColor().PackToInt();
		m_iOutlineColor = Color.WHITE;
		SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
		if (scriptedFaction)
			m_iOutlineColor = scriptedFaction.GetOutlineFactionColor().PackToInt();

		ApplyStatePresentation();
	}

	void RefreshAdminNamePresentation(int playerId)
	{
		if (!m_bHovered || !m_Entity || !m_Entity.GetOwner())
			return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager && playerManager.GetPlayerIdFromControlledEntity(m_Entity.GetOwner()) == playerId)
			RefreshHoverLabel();
	}

	protected bool RefreshHoverLabel()
	{
		if (!m_wLabelText || !m_Entity || !m_Entity.GetOwner())
			return false;

		string displayName = m_sFallbackName;
		int presentedPlayerId = -1;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(m_Entity.GetOwner());
			if (playerId > 0)
			{
				presentedPlayerId = playerId;
				string playerName = playerManager.GetPlayerName(playerId);
				if (!playerName.IsEmpty())
					displayName = playerName;
			}
		}

		m_wLabelText.SetText(displayName);
		GC_PlayerNamePresentation.Apply(m_wLabelText, presentedPlayerId);
		return !displayName.IsEmpty();
	}

	protected void ApplyStatePresentation()
	{
		if (!m_wSmallBackground || !m_wBackground || !m_wOutline || !m_wIcon || !m_wStatusIcon)
			return;

		m_wStatusIcon.SetVisible(m_bUnconscious);
		if (m_bDead)
		{
			int destroyedColor = UIColors.EDITOR_ICON_COLOR_DESTROYED.PackToInt();
			m_wSmallBackground.SetColorInt(destroyedColor);
			m_wBackground.SetColorInt(destroyedColor);
			m_wOutline.SetColorInt(destroyedColor);
			m_wIcon.SetColorInt(destroyedColor);
			GetWidget().SetOpacity(0.55);
			return;
		}

		m_wSmallBackground.SetColorInt(m_iFactionColor);
		m_wBackground.SetColorInt(m_iFactionColor);
		m_wOutline.SetColorInt(m_iOutlineColor);
		m_wIcon.SetColorInt(m_iOutlineColor);
		GetWidget().SetOpacity(1);
	}
}

//! Uses the base Game Master military-symbol hierarchy and applies the group's
//! authoritative faction color to the symbol root.
class GC_SpectatorGroupEditableEntityUIComponent : SCR_GroupEditableEntityUIComponent
{
	protected static const ResourceName GROUP_HOVER_LAYOUT = "{6A56000000000009}UI/layouts/Spectator/Lobby_Spectator_Group_Hover.layout";
	protected static const ResourceName GROUP_MEMBER_ROW_LAYOUT = "{6A5600000000000A}UI/layouts/Spectator/Lobby_Spectator_Group_Member_Row.layout";
	protected static const float GROUP_ICON_SIZE = 48.0;
	protected static const float CLOSE_FADE_START = 20.0;
	protected static const float CLOSE_FADE_END = 10.0;
	protected static const float HOVER_WIDTH = 220.0;
	protected static const float HOVER_HEADER_HEIGHT = 34.0;
	protected static const float HOVER_ROW_HEIGHT = 30.0;
	protected static const float HOVER_OUTER_PADDING = 4.0;
	protected static const float HOVER_ICON_GAP = 8.0;

	protected Widget m_wGroupOverlay;
	protected Widget m_wSlotRoot;
	protected Widget m_wMemberPopup;
	protected VerticalLayoutWidget m_wMemberList;
	protected SCR_AIGroup m_Group;
	protected float m_fPopupHeight;

	void GC_SpectatorGroupEditableEntityUIComponent()
	{
		m_Textures = {};
	}

	override void OnInit(SCR_EditableEntityComponent entity, SCR_UIInfo info, SCR_EditableEntityBaseSlotUIComponent slot)
	{
		m_wGroupOverlay = GetWidget();
		if (slot)
			m_wSlotRoot = slot.GetWidget();

		if (entity)
			m_Group = SCR_AIGroup.Cast(entity.GetOwner());

		RefreshGroupPresentation(entity);
		super.OnInit(entity, info, slot);
	}

	override void OnRefresh(SCR_EditableEntityBaseSlotUIComponent slot)
	{
		RefreshGroupPresentation(m_Entity);
		super.OnRefresh(slot);
	}

	void UpdateDistancePresentation(float distance, float maxDistance)
	{
		if (!m_wGroupOverlay)
			return;
		PositionMemberPopup();

		float opacity = 1.0;
		if (distance <= CLOSE_FADE_END)
			opacity = 0.0;
		else if (distance < CLOSE_FADE_START)
			opacity = (distance - CLOSE_FADE_END) / (CLOSE_FADE_START - CLOSE_FADE_END);

		bool isVisible = opacity > 0.0;
		m_wGroupOverlay.SetVisible(isVisible);
		m_wGroupOverlay.SetOpacity(opacity);
		if (!isVisible)
			RemoveMemberPopup();
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		CreateMemberPopup();
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		RemoveMemberPopup();
		return true;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 1 || !m_Group)
			return false;

		GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForGroup(m_Group), GetWidget());
		return true;
	}

	protected void CreateMemberPopup()
	{
		RemoveMemberPopup();
		if (!m_Group || !m_wSlotRoot)
			return;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		m_wMemberPopup = workspace.CreateWidgets(GROUP_HOVER_LAYOUT, m_wSlotRoot);
		if (!m_wMemberPopup)
			return;

		m_wMemberList = VerticalLayoutWidget.Cast(m_wMemberPopup.FindAnyWidget("LobbySpectatorGroupMembers"));
		if (!m_wMemberList)
		{
			RemoveMemberPopup();
			return;
		}

		TextWidget groupName = TextWidget.Cast(m_wMemberPopup.FindAnyWidget("LobbySpectatorGroupHeaderText"));
		if (groupName)
			groupName.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));

		Widget headerBackground = m_wMemberPopup.FindAnyWidget("LobbySpectatorGroupHeaderBackground");
		Faction faction = m_Group.GetFaction();
		if (headerBackground && faction)
			headerBackground.SetColor(faction.GetFactionColor());

		PlayerManager playerManager = GetGame().GetPlayerManager();
		int memberCount;
		if (playerManager)
		{
			array<int> playerIds = m_Group.GetPlayerIDs();
			foreach (int playerId : playerIds)
			{
				IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
				if (!playerEntity)
					continue;

				if (CreateMemberRow(workspace, playerEntity, playerManager.GetPlayerName(playerId), playerId))
					memberCount++;
			}
		}

		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity)
				continue;

			// Players are sourced from the group's replicated player ID list above,
			// so an agent backed by a player must not create a duplicate row.
			if (playerManager && playerManager.GetPlayerIdFromControlledEntity(controlledEntity) > 0)
				continue;

			if (CreateMemberRow(workspace, controlledEntity, string.Empty))
				memberCount++;
		}

		if (memberCount == 0)
		{
			RemoveMemberPopup();
			return;
		}

		m_fPopupHeight = HOVER_HEADER_HEIGHT + memberCount * HOVER_ROW_HEIGHT + HOVER_OUTER_PADDING * 2.0;
		FrameSlot.SetSize(m_wMemberPopup, HOVER_WIDTH, m_fPopupHeight);
		PositionMemberPopup();
		m_wMemberPopup.SetZOrder(100);
	}

	protected bool CreateMemberRow(WorkspaceWidget workspace, IEntity controlledEntity, string playerName, int playerId = -1)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.GetEditableEntity(controlledEntity);
		if (!editableEntity)
			return false;

		SCR_UIInfo memberInfo = editableEntity.GetInfo(controlledEntity);
		if (!memberInfo)
			return false;

		string displayName = playerName;
		if (displayName.IsEmpty())
		{
			SCR_EditableEntityUIInfo editableInfo = SCR_EditableEntityUIInfo.Cast(memberInfo);
			if (editableInfo)
				displayName = editableInfo.GetName();
		}

		if (displayName.IsEmpty())
			return false;

		Widget row = workspace.CreateWidgets(GROUP_MEMBER_ROW_LAYOUT, m_wMemberList);
		if (!row)
			return false;

		ImageWidget roleIcon = ImageWidget.Cast(row.FindAnyWidget("LobbySpectatorGroupMemberRoleIcon"));
		if (roleIcon)
			memberInfo.SetIconTo(roleIcon);

		RichTextWidget memberName = RichTextWidget.Cast(row.FindAnyWidget("LobbySpectatorGroupMemberName"));
		if (memberName)
		{
			memberName.SetText(displayName);
			GC_PlayerNamePresentation.Apply(memberName, playerId);
		}

		return true;
	}

	protected void PositionMemberPopup()
	{
		if (!m_wMemberPopup)
			return;

		FrameSlot.SetPos(m_wMemberPopup, -HOVER_WIDTH * 0.5, -GROUP_ICON_SIZE * 0.5 - m_fPopupHeight - HOVER_ICON_GAP);
	}

	protected void RemoveMemberPopup()
	{
		if (m_wMemberPopup)
			m_wMemberPopup.RemoveFromHierarchy();

		m_wMemberPopup = null;
		m_wMemberList = null;
		m_fPopupHeight = 0.0;
	}

	void RefreshAdminNamePresentation(int playerId)
	{
		if (!m_wMemberPopup)
			return;
		CreateMemberPopup();
	}

	protected void RefreshGroupPresentation(SCR_EditableEntityComponent entity)
	{
		if (!entity || !m_wGroupOverlay)
			return;

		Faction faction = entity.GetFaction();
		if (!faction)
			return;

		m_wGroupOverlay.SetColor(faction.GetFactionColor());
	}
}

//! Lightweight record for one native editable entity. A scene slot is only
//! allocated while the entity is inside the spectator overlay's draw distance.
class GC_SpectatorEditableEntityMarker
{
	protected static const float Z_ORDER_DISTANCE_PRECISION = 1000.0;

	protected SCR_EditableEntityComponent m_Entity;
	protected SCR_EditableEntitySceneSlotUIComponent m_Slot;
	protected GC_SpectatorCharacterEditableEntityUIComponent m_CharacterUI;
	protected GC_SpectatorGroupEditableEntityUIComponent m_GroupUI;
	protected EEditableEntityType m_eType;
	protected float m_fMaxDistance;
	protected float m_fMaxDistanceSq;

	void GC_SpectatorEditableEntityMarker(SCR_EditableEntityComponent entity, float maxDistance)
	{
		m_Entity = entity;
		m_eType = entity.GetEntityType();
		m_fMaxDistance = maxDistance;
		m_fMaxDistanceSq = maxDistance * maxDistance;
	}

	bool IsActive()
	{
		return m_Slot != null;
	}

	bool ShouldBeActive(vector cameraPosition)
	{
		if (!m_Entity || !m_Entity.GetOwner() || m_Entity.GetOwner().IsDeleted())
			return false;

		vector worldPosition;
		if (!m_Entity.GetPos(worldPosition))
			return false;

		float distanceSq = vector.DistanceSq(worldPosition, cameraPosition);
		if (distanceSq > m_fMaxDistanceSq)
			return false;

		return true;
	}

	bool Activate(WorkspaceWidget workspace, Widget parent, ResourceName slotLayout, ResourceName characterLayout, ResourceName groupLayout)
	{
		if (m_Slot)
			return true;

		if (!workspace || !parent || !m_Entity)
			return false;

		Widget slotRoot = workspace.CreateWidgets(slotLayout, parent);
		if (!slotRoot)
			return false;

		m_Slot = SCR_EditableEntitySceneSlotUIComponent.Cast(slotRoot.FindHandler(SCR_EditableEntitySceneSlotUIComponent));
		if (!m_Slot)
		{
			slotRoot.RemoveFromHierarchy();
			return false;
		}

		m_Slot.InitSlot(m_Entity);
		ResourceName entityLayout = characterLayout;
		if (m_eType == EEditableEntityType.GROUP)
			entityLayout = groupLayout;

		Widget entityRoot = m_Slot.CreateWidget(m_Entity, entityLayout);
		if (!entityRoot)
		{
			m_Slot.DeleteSlot();
			m_Slot = null;
			return false;
		}

		if (m_eType == EEditableEntityType.CHARACTER)
			m_CharacterUI = GC_SpectatorCharacterEditableEntityUIComponent.Cast(entityRoot.FindHandler(GC_SpectatorCharacterEditableEntityUIComponent));
		else if (m_eType == EEditableEntityType.GROUP)
			m_GroupUI = GC_SpectatorGroupEditableEntityUIComponent.Cast(entityRoot.FindHandler(GC_SpectatorGroupEditableEntityUIComponent));

		return true;
	}

	void Deactivate()
	{
		if (m_Slot)
			m_Slot.DeleteSlot();

		m_Slot = null;
		m_CharacterUI = null;
		m_GroupUI = null;
	}

	void Update(int screenWidth, int screenHeight, vector screenCenter, vector cameraPosition)
	{
		if (!m_Slot)
			return;

		m_Slot.UpdateSlot(screenWidth, screenHeight, screenCenter, cameraPosition);

		vector worldPosition;
		if (!m_Entity.GetPos(worldPosition))
			return;

		float distance = vector.Distance(worldPosition, cameraPosition);
		Widget slotWidget = m_Slot.GetWidget();
		if (slotWidget)
			slotWidget.SetZOrder(-distance * Z_ORDER_DISTANCE_PRECISION);

		if (m_CharacterUI)
		{
			m_CharacterUI.UpdateDistancePresentation(distance, m_fMaxDistance);
		}
		else if (m_GroupUI)
		{
			m_GroupUI.UpdateDistancePresentation(distance, m_fMaxDistance);
		}
	}

	void RefreshAdminNamePresentation(int playerId)
	{
		if (m_CharacterUI)
			m_CharacterUI.RefreshAdminNamePresentation(playerId);
		else if (m_GroupUI)
			m_GroupUI.RefreshAdminNamePresentation(playerId);
	}
}

//! Spectator-only adapter over the base editable-entity registry and scene UI.
//! It owns no player, AI, group, faction, or affiliation state of its own.
class GC_SpectatorMarkerOverlay
{
	protected static const ResourceName OVERLAY_LAYOUT = "{6A56000000000001}UI/layouts/Spectator/Lobby_Spectator_Marker_Overlay.layout";
	protected static const ResourceName CHARACTER_LAYOUT = "{6A56000000000002}UI/layouts/Spectator/Lobby_Spectator_Marker.layout";
	protected static const ResourceName SLOT_LAYOUT = "{6A56000000000004}UI/layouts/Spectator/Lobby_Spectator_Entity_Slot.layout";
	protected static const ResourceName GROUP_LAYOUT = "{6A56000000000005}UI/layouts/Spectator/Lobby_Spectator_Group_Marker.layout";
	protected static const int VISIBILITY_REFRESH_INTERVAL = 250;
	protected static const float MAX_MARKER_DISTANCE = 2500.0;

	protected Widget m_wRoot;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected SCR_EditableEntityCore m_EditableEntityCore;
	protected GC_LobbyManager m_LobbyManager;
	protected ref map<SCR_EditableEntityComponent, ref GC_SpectatorEditableEntityMarker> m_mMarkers = new map<SCR_EditableEntityComponent, ref GC_SpectatorEditableEntityMarker>();

	bool Start()
	{
		if (m_wRoot)
			return true;

		m_Workspace = GetGame().GetWorkspace();
		m_World = GetGame().GetWorld();
		m_EditableEntityCore = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_Workspace || !m_World || !m_EditableEntityCore)
			return false;

		m_wRoot = m_Workspace.CreateWidgets(OVERLAY_LAYOUT);
		if (!m_wRoot)
			return false;

		m_EditableEntityCore.Event_OnEntityRegistered.Insert(OnEntityRegistered);
		m_EditableEntityCore.Event_OnEntityUnregistered.Insert(OnEntityUnregistered);
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);

		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		m_EditableEntityCore.GetAllEntities(entities);
		foreach (SCR_EditableEntityComponent entity : entities)
			OnEntityRegistered(entity);

		RefreshVisibility();
		GetGame().GetCallqueue().CallLater(UpdatePositions, 0, true);
		GetGame().GetCallqueue().CallLater(RefreshVisibility, VISIBILITY_REFRESH_INTERVAL, true);
		return true;
	}

	void Destroy()
	{
		GetGame().GetCallqueue().Remove(UpdatePositions);
		GetGame().GetCallqueue().Remove(RefreshVisibility);

		if (m_EditableEntityCore)
		{
			m_EditableEntityCore.Event_OnEntityRegistered.Remove(OnEntityRegistered);
			m_EditableEntityCore.Event_OnEntityUnregistered.Remove(OnEntityUnregistered);
		}
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
			marker.Deactivate();

		m_mMarkers.Clear();
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_Workspace = null;
		m_World = null;
		m_EditableEntityCore = null;
		m_LobbyManager = null;
	}

	protected void OnEntityRegistered(SCR_EditableEntityComponent entity)
	{
		if (!entity || m_mMarkers.Contains(entity))
			return;

		EEditableEntityType entityType = entity.GetEntityType();
		if (entityType != EEditableEntityType.CHARACTER && entityType != EEditableEntityType.GROUP)
			return;

		IEntity owner = entity.GetOwner();
		if (!owner || owner.FindComponent(GC_LobbySpectatorBodyComponent))
			return;

		if (entityType == EEditableEntityType.CHARACTER && !SCR_EditableCharacterComponent.Cast(entity))
			return;

		if (entityType == EEditableEntityType.GROUP && !SCR_EditableGroupComponent.Cast(entity))
			return;

		m_mMarkers.Insert(entity, new GC_SpectatorEditableEntityMarker(entity, MAX_MARKER_DISTANCE));
	}

	protected void OnEntityUnregistered(SCR_EditableEntityComponent entity)
	{
		GC_SpectatorEditableEntityMarker marker;
		if (!m_mMarkers.Find(entity, marker))
			return;

		marker.Deactivate();
		m_mMarkers.Remove(entity);
	}

	protected void OnPlayerRoleChanged(int playerId)
	{
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
			marker.RefreshAdminNamePresentation(playerId);
	}

	protected void RefreshVisibility()
	{
		if (!m_wRoot || !m_World)
			return;

		vector cameraTransform[4];
		m_World.GetCurrentCamera(cameraTransform);
		vector cameraPosition = cameraTransform[3];

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
		{
			bool shouldBeActive = marker.ShouldBeActive(cameraPosition);
			if (shouldBeActive && !marker.IsActive())
				marker.Activate(m_Workspace, m_wRoot, SLOT_LAYOUT, CHARACTER_LAYOUT, GROUP_LAYOUT);
			else if (!shouldBeActive && marker.IsActive())
				marker.Deactivate();
		}
	}

	protected void UpdatePositions()
	{
		if (!m_wRoot || !m_Workspace || !m_World)
			return;

		vector cameraTransform[4];
		m_World.GetCurrentCamera(cameraTransform);
		vector cameraPosition = cameraTransform[3];
		int screenWidth = m_Workspace.DPIUnscale(m_Workspace.GetWidth());
		int screenHeight = m_Workspace.DPIUnscale(m_Workspace.GetHeight());
		vector screenCenter = Vector(screenWidth * 0.5, screenHeight * 0.5, 0);

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
			marker.Update(screenWidth, screenHeight, screenCenter, cameraPosition);
	}
}

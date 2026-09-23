//! Lobby2 presentation layered on top of the native editable-character widget.
//! The base component remains responsible for resolving the character role icon.
class GC_SpectatorCharacterEditableEntityUIComponent : SCR_CharacterEditableEntityUIComponent
{
	protected static const float MIN_ICON_DISTANCE = 0.0;
	protected static const float MAX_ICON_SIZE = 32.0;
	protected static const float MIN_ICON_SIZE = 4.0;
	protected static const float NAME_REVEAL_DISTANCE = 50.0;
	protected static const float NAME_REVEAL_TRANSITION_FRACTION = 0.1;

	protected ImageWidget m_wSmallBackground;
	protected ImageWidget m_wBackground;
	protected ImageWidget m_wOutline;
	protected ImageWidget m_wIcon;
	protected ImageWidget m_wSelectedIcon;
	protected ImageWidget m_wStatusIcon;
	protected ImageWidget m_wDisconnectedIcon;
	protected ButtonWidget m_wButton;
	protected PanelWidget m_wLabel;
	protected RichTextWidget m_wLabelText;
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected string m_sFallbackName;
	protected bool m_bHasLabel;
	protected bool m_bHovered;
	protected float m_fProximityNameOpacity;
	protected bool m_bStateInitialized;
	protected bool m_bDead;
	protected bool m_bUnconscious;
	protected bool m_bDisconnected;
	protected int m_iFactionColor;
	protected int m_iOutlineColor;
	protected float m_fLastPresentationDistance = -1.0;
	protected float m_fAppliedIconSize = -1.0;
	protected float m_fAppliedLabelOpacity = -1.0;
	protected bool m_bAppliedLabelVisible;
	protected bool m_bLabelVisibilityInitialized;
	protected float m_fCachedLabelWidth;
	protected bool m_bLabelLayoutDirty = true;

	override void OnInit(SCR_EditableEntityComponent entity, SCR_UIInfo info, SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnInit(entity, info, slot);

		m_wSmallBackground = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerSmallBackground"));
		m_wBackground = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerBackground"));
		m_wOutline = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerOutline"));
		m_wIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerRoleIcon"));
		m_wSelectedIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerSelectedIcon"));
		m_wStatusIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerWoundedIcon"));
		m_wDisconnectedIcon = ImageWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerDisconnectedIcon"));
		m_wButton = ButtonWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerButton"));
		m_wLabel = PanelWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerLabel"));
		m_wLabelText = RichTextWidget.Cast(GetWidget().FindAnyWidget("LobbySpectatorMarkerLabelText"));
		if (m_wLabel)
		{
			m_wLabel.SetOpacity(0.0);
			m_wLabel.SetVisible(false);
			m_fAppliedLabelOpacity = 0.0;
			m_bLabelVisibilityInitialized = true;
		}
		if (m_wSelectedIcon)
		{
			m_wSelectedIcon.SetOpacity(0.0);
			m_wSelectedIcon.SetVisible(false);
		}

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
		m_bHasLabel = RefreshLabel();
		RefreshFaction(entity.GetFaction());
		RefreshCharacterState();
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (lobbyManager)
			RefreshSlotPresentation(lobbyManager.GetSlotByEntity(entity.GetOwner()));
	}

	void UpdateDistancePresentation(float distance, float maxDistance)
	{
		if (!m_wIcon)
			return;

		bool distanceChanged = m_fLastPresentationDistance < 0.0 || Math.AbsFloat(distance - m_fLastPresentationDistance) >= 0.5;
		if (distanceChanged)
		{
			m_fLastPresentationDistance = distance;
			float distanceScale = 1.0 - ((distance - MIN_ICON_DISTANCE) / (maxDistance - MIN_ICON_DISTANCE));
			distanceScale = Math.Clamp(distanceScale, 0.0, 1.0);
			distanceScale = distanceScale * distanceScale * distanceScale;
			float iconSize = MIN_ICON_SIZE + (MAX_ICON_SIZE - MIN_ICON_SIZE) * distanceScale;
			if (m_fAppliedIconSize < 0.0 || Math.AbsFloat(iconSize - m_fAppliedIconSize) >= 0.25)
			{
				m_fAppliedIconSize = iconSize;
				float iconOffset = -iconSize * 0.5;
				FrameSlot.SetSize(m_wIcon, iconSize, iconSize);
				FrameSlot.SetPos(m_wIcon, iconOffset, iconOffset);
				if (m_wSelectedIcon)
				{
					FrameSlot.SetSize(m_wSelectedIcon, iconSize, iconSize);
					FrameSlot.SetPos(m_wSelectedIcon, iconOffset, iconOffset);
				}
				if (m_wButton)
				{
					FrameSlot.SetSize(m_wButton, iconSize, iconSize);
					FrameSlot.SetPos(m_wButton, iconOffset, iconOffset);
				}
			}
		}

		if (m_wSelectedIcon)
		{
			float highlightOpacity = m_wSelectedIcon.GetOpacity();
			if ((m_bHovered && highlightOpacity < 1.0) || (!m_bHovered && highlightOpacity > 0.0))
			{
				float highlightStep = GetGame().GetWorld().GetTimeSlice() * 5.0;
				if (m_bHovered)
					highlightOpacity += highlightStep;
				else
					highlightOpacity -= highlightStep;

				highlightOpacity = Math.Clamp(highlightOpacity, 0.0, 1.0);
				m_wSelectedIcon.SetOpacity(highlightOpacity);
				if (!m_bHovered && highlightOpacity <= 0.0)
					m_wSelectedIcon.SetVisible(false);
			}
		}

		if (m_wLabel && (distanceChanged || m_bLabelLayoutDirty))
		{
			m_fProximityNameOpacity = 0.0;
			float nameRevealTransitionDistance = NAME_REVEAL_DISTANCE * NAME_REVEAL_TRANSITION_FRACTION;
			float fullNameRevealDistance = NAME_REVEAL_DISTANCE - nameRevealTransitionDistance;
			if (distance < NAME_REVEAL_DISTANCE)
			{
				if (!m_bHasLabel)
					m_bHasLabel = RefreshLabel();

				if (m_bHasLabel)
				{
					if (distance <= fullNameRevealDistance)
						m_fProximityNameOpacity = 1.0;
					else
						m_fProximityNameOpacity = (NAME_REVEAL_DISTANCE - distance) / nameRevealTransitionDistance;

					m_fProximityNameOpacity = m_fProximityNameOpacity * m_fProximityNameOpacity * (3.0 - 2.0 * m_fProximityNameOpacity);
				}
			}

			float labelOpacity = m_fProximityNameOpacity;
			if (m_bHovered)
				labelOpacity = 1.0;
			bool labelVisible = labelOpacity > 0.0;
			if (!m_bLabelVisibilityInitialized || labelVisible != m_bAppliedLabelVisible)
			{
				m_wLabel.SetVisible(labelVisible);
				m_bAppliedLabelVisible = labelVisible;
				m_bLabelVisibilityInitialized = true;
			}
			if (Math.AbsFloat(labelOpacity - m_fAppliedLabelOpacity) >= 0.01)
			{
				m_wLabel.SetOpacity(labelOpacity);
				m_fAppliedLabelOpacity = labelOpacity;
			}

			if (m_bLabelLayoutDirty)
			{
				float labelHeight;
				m_wLabel.GetScreenSize(m_fCachedLabelWidth, labelHeight);
				if (m_fCachedLabelWidth > 0.0)
					m_bLabelLayoutDirty = false;
			}

			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (workspace && m_fCachedLabelWidth > 0.0)
				FrameSlot.SetPos(m_wLabel, -workspace.DPIUnscale(m_fCachedLabelWidth) * 0.5, -m_fAppliedIconSize);
		}
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bHovered = true;
		m_bHasLabel = RefreshLabel();
		if (m_wSelectedIcon)
			m_wSelectedIcon.SetVisible(true);
		if (m_wLabel && m_bHasLabel)
		{
			m_wLabel.SetVisible(true);
			m_wLabel.SetOpacity(1.0);
			m_fAppliedLabelOpacity = 1.0;
			m_bAppliedLabelVisible = true;
			m_bLabelVisibilityInitialized = true;
		}

		GetWidget().SetZOrder(100);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bHovered = false;
		if (m_wLabel)
		{
			bool labelVisible = m_fProximityNameOpacity > 0.0;
			m_wLabel.SetVisible(labelVisible);
			m_wLabel.SetOpacity(m_fProximityNameOpacity);
			m_fAppliedLabelOpacity = m_fProximityNameOpacity;
			m_bAppliedLabelVisible = labelVisible;
			m_bLabelVisibilityInitialized = true;
		}

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
			m_bHasLabel = RefreshLabel();
		else if (m_fProximityNameOpacity > 0.0)
			m_bHasLabel = RefreshLabel();
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
		if (!m_Entity || !m_Entity.GetOwner())
			return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager && playerManager.GetPlayerIdFromControlledEntity(m_Entity.GetOwner()) == playerId)
			m_bHasLabel = RefreshLabel();
	}

	void RefreshSlotPresentation(GC_Slot slot)
	{
		if (!slot || !m_Entity || slot.GetEntity() != m_Entity.GetOwner())
			return;

		m_bDisconnected = slot.GetState() == GC_SlotState.DISCONNECTED;
		m_bHasLabel = RefreshLabel();
		ApplyStatePresentation();
	}

	protected bool RefreshLabel()
	{
		if (!m_wLabelText || !m_Entity || !m_Entity.GetOwner())
			return false;

		string displayName = m_sFallbackName;
		int presentedPlayerId = -1;
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		GC_Slot slot;
		if (lobbyManager)
			slot = lobbyManager.GetSlotByEntity(m_Entity.GetOwner());
		if (slot && !slot.GetCachedPlayerName().IsEmpty())
		{
			displayName = slot.GetCachedPlayerName();
			presentedPlayerId = slot.GetPlayerId();
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if ((!slot || slot.GetCachedPlayerName().IsEmpty()) && playerManager)
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
		m_bLabelLayoutDirty = true;
		return !displayName.IsEmpty();
	}

	protected void ApplyStatePresentation()
	{
		if (!m_wSmallBackground || !m_wBackground || !m_wOutline || !m_wIcon || !m_wStatusIcon || !m_wDisconnectedIcon)
			return;

		m_wStatusIcon.SetVisible(m_bUnconscious && !m_bDisconnected);
		m_wDisconnectedIcon.SetVisible(m_bDisconnected);
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
	protected static const float CLOSE_FADE_START = 100.0;
	protected static const float CLOSE_FADE_TRANSITION_FRACTION = 0.1;
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
	protected float m_fLastPresentationDistance = -1.0;
	protected float m_fAppliedOpacity = -1.0;
	protected bool m_bAppliedVisibility;
	protected bool m_bPresentationInitialized;

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
		m_fLastPresentationDistance = -1.0;
		RefreshGroupPresentation(m_Entity);
		super.OnRefresh(slot);
	}

	void UpdateDistancePresentation(float distance, float maxDistance)
	{
		if (!m_wGroupOverlay)
			return;
		if (m_fLastPresentationDistance >= 0.0 && Math.AbsFloat(distance - m_fLastPresentationDistance) < 1.0)
			return;
		m_fLastPresentationDistance = distance;

		float opacity = 1.0;
		float closeFadeTransitionDistance = CLOSE_FADE_START * CLOSE_FADE_TRANSITION_FRACTION;
		float closeFadeEnd = CLOSE_FADE_START - closeFadeTransitionDistance;
		if (distance <= closeFadeEnd)
			opacity = 0.0;
		else if (distance < CLOSE_FADE_START)
			opacity = (distance - closeFadeEnd) / closeFadeTransitionDistance;

		bool isVisible = m_Group && m_Group.GetAgentsCount() > 0 && opacity > 0.0;
		if (!m_bPresentationInitialized || isVisible != m_bAppliedVisibility)
		{
			m_wGroupOverlay.SetVisible(isVisible);
			m_bAppliedVisibility = isVisible;
			m_bPresentationInitialized = true;
		}
		if (Math.AbsFloat(opacity - m_fAppliedOpacity) >= 0.01)
		{
			m_wGroupOverlay.SetOpacity(opacity);
			m_fAppliedOpacity = opacity;
		}
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
			headerBackground.SetColor(GC_LobbyUIColors.FactionBackground(faction));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		int memberCount;
		if (playerManager)
		{
			array<int> playerIds = m_Group.GetPlayerIDs();
			foreach (int playerId : playerIds)
			{
				IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
				if (!playerEntity)
					continue;

				string playerName;
				GC_Slot playerSlot;
				if (lobbyManager)
					playerSlot = lobbyManager.GetSlotByPlayerId(playerId);
				if (playerSlot)
					playerName = playerSlot.GetCachedPlayerName();
				if (playerName.IsEmpty())
					playerName = playerManager.GetPlayerName(playerId);

				if (CreateMemberRow(workspace, playerEntity, playerName, playerId))
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

			string playerName;
			int playerId = -1;
			if (lobbyManager)
			{
				GC_Slot slot = lobbyManager.GetSlotByEntity(controlledEntity);
				if (slot)
				{
					playerName = slot.GetCachedPlayerName();
					playerId = slot.GetPlayerId();
				}
			}

			if (CreateMemberRow(workspace, controlledEntity, playerName, playerId))
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

	void RefreshSlotPresentation(GC_Slot slot)
	{
		if (m_wMemberPopup && slot && slot.GetGroup() == m_Group)
			CreateMemberPopup();
	}

	protected void RefreshGroupPresentation(SCR_EditableEntityComponent entity)
	{
		if (!entity || !m_wGroupOverlay)
			return;
		if (!m_Group || m_Group.GetAgentsCount() == 0)
		{
			m_wGroupOverlay.SetVisible(false);
			m_bAppliedVisibility = false;
			m_bPresentationInitialized = true;
			RemoveMemberPopup();
			return;
		}

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
	protected static const float VIEWPORT_ACTIVATION_MARGIN = 160.0;
	protected static const float VIEWPORT_DEACTIVATION_MARGIN = 240.0;

	protected SCR_EditableEntityComponent m_Entity;
	protected SCR_EditableEntitySceneSlotUIComponent m_Slot;
	protected Widget m_wSlotWidget;
	protected GC_SpectatorCharacterEditableEntityUIComponent m_CharacterUI;
	protected GC_SpectatorGroupEditableEntityUIComponent m_GroupUI;
	protected EEditableEntityType m_eType;
	protected float m_fMaxDistance;
	protected float m_fMaxDistanceSq;
	protected float m_fVisibilityDistance;
	protected float m_fAppliedZDistance = -1.0;
	protected bool m_bProjectionVisibilityInitialized;
	protected bool m_bProjectionVisible;
	protected bool m_bScreenPositionInitialized;
	protected vector m_vAppliedScreenPosition;

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

	bool ShouldBeActive(vector cameraPosition, WorkspaceWidget workspace, BaseWorld world, int screenWidth, int screenHeight, out vector screenPosition)
	{
		screenPosition = vector.Zero;
		if (!m_Entity || !m_Entity.GetOwner() || m_Entity.GetOwner().IsDeleted() || !workspace || !world)
			return false;

		vector worldPosition;
		if (!m_Entity.GetPos(worldPosition))
			return false;

		float distanceSq = vector.DistanceSq(worldPosition, cameraPosition);
		if (distanceSq > m_fMaxDistanceSq)
			return false;

		screenPosition = workspace.ProjWorldToScreen(worldPosition, world);
		if (screenPosition[2] <= 0.0)
			return false;

		float viewportMargin = VIEWPORT_ACTIVATION_MARGIN;
		if (m_Slot)
			viewportMargin = VIEWPORT_DEACTIVATION_MARGIN;
		if (screenPosition[0] < -viewportMargin || screenPosition[0] > screenWidth + viewportMargin)
			return false;
		if (screenPosition[1] < -viewportMargin || screenPosition[1] > screenHeight + viewportMargin)
			return false;

		m_fVisibilityDistance = vector.Distance(worldPosition, cameraPosition);
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
		m_wSlotWidget = slotRoot;
		ResourceName entityLayout = characterLayout;
		if (m_eType == EEditableEntityType.GROUP)
			entityLayout = groupLayout;

		Widget entityRoot = m_Slot.CreateWidget(m_Entity, entityLayout);
		if (!entityRoot)
		{
			m_Slot.DeleteSlot();
			m_Slot = null;
			m_wSlotWidget = null;
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
		m_wSlotWidget = null;
		m_CharacterUI = null;
		m_GroupUI = null;
		m_fAppliedZDistance = -1.0;
		m_bProjectionVisibilityInitialized = false;
		m_bProjectionVisible = false;
		m_bScreenPositionInitialized = false;
	}

	void RefreshPresentation()
	{
		if (!m_Slot || !m_wSlotWidget)
			return;

		if (m_fAppliedZDistance < 0.0 || Math.AbsFloat(m_fVisibilityDistance - m_fAppliedZDistance) >= 1.0)
		{
			m_wSlotWidget.SetZOrder(-m_fVisibilityDistance);
			m_fAppliedZDistance = m_fVisibilityDistance;
		}

		if (m_CharacterUI)
			m_CharacterUI.UpdateDistancePresentation(m_fVisibilityDistance, m_fMaxDistance);
		else if (m_GroupUI)
			m_GroupUI.UpdateDistancePresentation(m_fVisibilityDistance, m_fMaxDistance);
	}

	void Update(vector screenPosition)
	{
		if (!m_Slot || !m_wSlotWidget)
			return;

		if (!m_bProjectionVisibilityInitialized || !m_bProjectionVisible)
		{
			m_wSlotWidget.SetVisible(true);
			m_bProjectionVisible = true;
			m_bProjectionVisibilityInitialized = true;
		}

		if (!m_bScreenPositionInitialized
			|| screenPosition[0] != m_vAppliedScreenPosition[0]
			|| screenPosition[1] != m_vAppliedScreenPosition[1])
		{
			FrameSlot.SetPos(m_wSlotWidget, screenPosition[0], screenPosition[1]);
			m_vAppliedScreenPosition = screenPosition;
			m_bScreenPositionInitialized = true;
		}
	}

	void RefreshAdminNamePresentation(int playerId)
	{
		if (m_CharacterUI)
			m_CharacterUI.RefreshAdminNamePresentation(playerId);
		else if (m_GroupUI)
			m_GroupUI.RefreshAdminNamePresentation(playerId);
	}

	void RefreshSlotPresentation(GC_Slot slot)
	{
		if (m_CharacterUI)
			m_CharacterUI.RefreshSlotPresentation(slot);
		else if (m_GroupUI)
			m_GroupUI.RefreshSlotPresentation(slot);
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
	protected static const float MAX_MARKER_DISTANCE = 2500.0;

	protected Widget m_wRoot;
	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected SCR_EditableEntityCore m_EditableEntityCore;
	protected GC_LobbyManager m_LobbyManager;
	protected bool m_bInterfaceVisible = true;
	protected bool m_bProjectileTracersEnabled;
	protected bool m_bKilledFromEnabled;
	protected ref GC_SpectatorProjectileTracer m_ProjectileTracer;
	protected ref GC_SpectatorKilledFromVisualizer m_KilledFromVisualizer;
	protected ref map<SCR_EditableEntityComponent, ref GC_SpectatorEditableEntityMarker> m_mMarkers = new map<SCR_EditableEntityComponent, ref GC_SpectatorEditableEntityMarker>();
	protected ref set<SCR_EditableEntityComponent> m_sVehicles = new set<SCR_EditableEntityComponent>();
	protected ref map<SCR_EditableEntityComponent, ref array<IEntity>> m_mCharacterShooters = new map<SCR_EditableEntityComponent, ref array<IEntity>>();
	protected ref map<SCR_EditableEntityComponent, ref array<IEntity>> m_mVehicleShooters = new map<SCR_EditableEntityComponent, ref array<IEntity>>();
	protected ref map<SCR_EditableEntityComponent, CharacterWeaponManagerComponent> m_mCharacterWeaponManagers = new map<SCR_EditableEntityComponent, CharacterWeaponManagerComponent>();

	bool Start(float projectileTraceDuration, bool projectileTracersEnabled, GC_KilledFromSnapshot killedFromSnapshot, bool killedFromEnabled)
	{
		if (m_wRoot)
			return true;

		m_Workspace = GetGame().GetWorkspace();
		m_World = GetGame().GetWorld();
		m_EditableEntityCore = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		m_LobbyManager = GC_LobbyManager.GetManager();
		m_bProjectileTracersEnabled = projectileTracersEnabled;
		m_bKilledFromEnabled = killedFromEnabled;
		if (!m_Workspace || !m_World || !m_EditableEntityCore)
			return false;

		m_wRoot = m_Workspace.CreateWidgets(OVERLAY_LAYOUT);
		if (!m_wRoot)
			return false;

		CanvasWidget tracerCanvas = CanvasWidget.Cast(m_wRoot.FindAnyWidget("LobbySpectatorProjectileTracer"));
		m_ProjectileTracer = new GC_SpectatorProjectileTracer();
		if (!m_ProjectileTracer.Start(tracerCanvas, m_Workspace, m_World, projectileTraceDuration, projectileTracersEnabled))
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			m_ProjectileTracer = null;
			return false;
		}

		CanvasWidget killedFromCanvas = CanvasWidget.Cast(m_wRoot.FindAnyWidget("LobbySpectatorKilledFromTracer"));
		m_KilledFromVisualizer = new GC_SpectatorKilledFromVisualizer();
		if (!m_KilledFromVisualizer.Start(killedFromCanvas, m_wRoot, m_Workspace, m_World, killedFromSnapshot, killedFromEnabled))
		{
			m_ProjectileTracer.Destroy();
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			m_ProjectileTracer = null;
			m_KilledFromVisualizer = null;
			return false;
		}

		m_EditableEntityCore.Event_OnEntityRegistered.Insert(OnEntityRegistered);
		m_EditableEntityCore.Event_OnEntityUnregistered.Insert(OnEntityUnregistered);
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);
			m_LobbyManager.m_OnSlotPresentationChanged.Insert(OnSlotPresentationChanged);
		}

		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		m_EditableEntityCore.GetAllEntities(entities);
		foreach (SCR_EditableEntityComponent entity : entities)
			OnEntityRegistered(entity);

		Update();
		return true;
	}

	void Destroy()
	{
		if (m_EditableEntityCore)
		{
			m_EditableEntityCore.Event_OnEntityRegistered.Remove(OnEntityRegistered);
			m_EditableEntityCore.Event_OnEntityUnregistered.Remove(OnEntityUnregistered);
		}
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);
			m_LobbyManager.m_OnSlotPresentationChanged.Remove(OnSlotPresentationChanged);
		}
		ClearProjectileShooters();
		if (m_ProjectileTracer)
			m_ProjectileTracer.Destroy();
		if (m_KilledFromVisualizer)
			m_KilledFromVisualizer.Destroy();

		m_mCharacterShooters.Clear();
		m_mVehicleShooters.Clear();
		m_mCharacterWeaponManagers.Clear();
		m_sVehicles.Clear();
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
		m_bProjectileTracersEnabled = false;
		m_bKilledFromEnabled = false;
		m_ProjectileTracer = null;
		m_KilledFromVisualizer = null;
	}

	void SetInterfaceVisible(bool visible)
	{
		if (m_wRoot)
			m_wRoot.SetVisible(visible);
		if (visible == m_bInterfaceVisible)
			return;

		m_bInterfaceVisible = visible;
		ApplyProjectileTracerState();
		if (m_KilledFromVisualizer)
			m_KilledFromVisualizer.SetEnabled(m_bKilledFromEnabled && m_bInterfaceVisible);
		if (visible)
		{
			Update();
			return;
		}

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
			marker.Deactivate();
	}

	void SetProjectileTracersEnabled(bool enabled)
	{
		m_bProjectileTracersEnabled = enabled;
		ApplyProjectileTracerState();
	}

	GC_SpectatorProjectileTracer GetProjectileTracer()
	{
		return m_ProjectileTracer;
	}

	void SetKilledFromSnapshot(GC_KilledFromSnapshot snapshot)
	{
		if (m_KilledFromVisualizer)
			m_KilledFromVisualizer.SetSnapshot(snapshot);
	}

	void SetKilledFromEnabled(bool enabled)
	{
		m_bKilledFromEnabled = enabled;
		if (m_KilledFromVisualizer)
			m_KilledFromVisualizer.SetEnabled(m_bKilledFromEnabled && m_bInterfaceVisible);
	}

	protected void ApplyProjectileTracerState()
	{
		if (!m_ProjectileTracer)
			return;

		bool shouldRun = m_bProjectileTracersEnabled && m_bInterfaceVisible;
		if (m_ProjectileTracer.IsEnabled() == shouldRun)
			return;

		if (!shouldRun)
		{
			ClearProjectileShooters();
			m_ProjectileTracer.SetEnabled(false);
			return;
		}

		m_ProjectileTracer.SetEnabled(true);
		RefreshProjectileShooters();
	}

	protected void OnEntityRegistered(SCR_EditableEntityComponent entity)
	{
		if (!entity || m_mMarkers.Contains(entity))
			return;

		EEditableEntityType entityType = entity.GetEntityType();
		IEntity owner = entity.GetOwner();
		if (entityType == EEditableEntityType.VEHICLE)
		{
			m_sVehicles.Insert(entity);
			if (m_ProjectileTracer && m_ProjectileTracer.IsEnabled())
				RefreshVehicleShooters(entity);
			return;
		}

		if (entityType != EEditableEntityType.CHARACTER && entityType != EEditableEntityType.GROUP)
			return;

		if (!owner || owner.FindComponent(GC_LobbySpectatorBodyComponent))
			return;

		if (entityType == EEditableEntityType.CHARACTER && !SCR_EditableCharacterComponent.Cast(entity))
			return;
		if (entityType == EEditableEntityType.CHARACTER)
		{
			if (m_ProjectileTracer && m_ProjectileTracer.IsEnabled())
				RefreshCharacterShooters(entity);
		}

		if (entityType == EEditableEntityType.GROUP)
		{
			if (!SCR_EditableGroupComponent.Cast(entity))
				return;
		}

		m_mMarkers.Insert(entity, new GC_SpectatorEditableEntityMarker(entity, MAX_MARKER_DISTANCE));
	}

	protected void OnEntityUnregistered(SCR_EditableEntityComponent entity)
	{
		if (!entity)
			return;

		EEditableEntityType entityType = entity.GetEntityType();
		if (entityType == EEditableEntityType.VEHICLE)
		{
			array<IEntity> vehicleShooters;
			if (m_mVehicleShooters.Find(entity, vehicleShooters) && m_ProjectileTracer)
				foreach (IEntity shooter : vehicleShooters)
					m_ProjectileTracer.UntrackShooter(shooter);
			m_mVehicleShooters.Remove(entity);
			m_sVehicles.RemoveItem(entity);
			return;
		}

		GC_SpectatorEditableEntityMarker marker;
		if (entityType == EEditableEntityType.CHARACTER)
		{
			CharacterWeaponManagerComponent weaponManager;
			if (m_mCharacterWeaponManagers.Find(entity, weaponManager) && weaponManager)
				weaponManager.m_OnWeaponChangeCompleteInvoker.Remove(OnCharacterWeaponChanged);
			m_mCharacterWeaponManagers.Remove(entity);

			array<IEntity> characterShooters;
			if (m_mCharacterShooters.Find(entity, characterShooters) && m_ProjectileTracer)
				foreach (IEntity shooter : characterShooters)
					m_ProjectileTracer.UntrackShooter(shooter);
			m_mCharacterShooters.Remove(entity);
		}
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

	protected void OnSlotPresentationChanged(GC_Slot slot)
	{
		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
			marker.RefreshSlotPresentation(slot);
	}

	void Update()
	{
		if (!m_wRoot || !m_Workspace || !m_World || !m_bInterfaceVisible)
			return;

		vector cameraTransform[4];
		m_World.GetCurrentCamera(cameraTransform);
		vector cameraPosition = cameraTransform[3];
		int screenWidth = m_Workspace.DPIUnscale(m_Workspace.GetWidth());
		int screenHeight = m_Workspace.DPIUnscale(m_Workspace.GetHeight());

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
		{
			vector screenPosition;
			bool shouldBeActive = marker.ShouldBeActive(cameraPosition, m_Workspace, m_World, screenWidth, screenHeight, screenPosition);
			if (shouldBeActive && !marker.IsActive())
				marker.Activate(m_Workspace, m_wRoot, SLOT_LAYOUT, CHARACTER_LAYOUT, GROUP_LAYOUT);
			else if (!shouldBeActive && marker.IsActive())
				marker.Deactivate();

			if (shouldBeActive && marker.IsActive())
			{
				marker.Update(screenPosition);
				marker.RefreshPresentation();
			}
		}
	}

	protected void RefreshProjectileShooters()
	{
		if (!m_ProjectileTracer || !m_ProjectileTracer.IsEnabled())
			return;

		foreach (SCR_EditableEntityComponent entity, GC_SpectatorEditableEntityMarker marker : m_mMarkers)
		{
			if (entity.GetEntityType() == EEditableEntityType.CHARACTER)
				RefreshCharacterShooters(entity);
		}
		foreach (SCR_EditableEntityComponent vehicle : m_sVehicles)
			RefreshVehicleShooters(vehicle);
	}

	protected void ClearProjectileShooters()
	{
		foreach (SCR_EditableEntityComponent entity, CharacterWeaponManagerComponent weaponManager : m_mCharacterWeaponManagers)
		{
			if (weaponManager)
				weaponManager.m_OnWeaponChangeCompleteInvoker.Remove(OnCharacterWeaponChanged);
		}
		m_mCharacterWeaponManagers.Clear();

		if (m_ProjectileTracer)
		{
			foreach (SCR_EditableEntityComponent entity, array<IEntity> characterShooters : m_mCharacterShooters)
			{
				foreach (IEntity shooter : characterShooters)
					m_ProjectileTracer.UntrackShooter(shooter);
			}
			foreach (SCR_EditableEntityComponent vehicle, array<IEntity> vehicleShooters : m_mVehicleShooters)
			{
				foreach (IEntity shooter : vehicleShooters)
					m_ProjectileTracer.UntrackShooter(shooter);
			}
		}
		m_mCharacterShooters.Clear();
		m_mVehicleShooters.Clear();
	}

	protected void OnCharacterWeaponChanged(BaseWeaponComponent newWeaponSlot)
	{
		if (!m_ProjectileTracer || !m_ProjectileTracer.IsEnabled())
			return;

		foreach (SCR_EditableEntityComponent entity, CharacterWeaponManagerComponent weaponManager : m_mCharacterWeaponManagers)
			RefreshCharacterShooters(entity);
	}

	protected void RefreshCharacterShooters(SCR_EditableEntityComponent characterEntity)
	{
		if (!characterEntity || !m_ProjectileTracer)
			return;

		IEntity character = characterEntity.GetOwner();
		if (!character)
			return;

		CharacterWeaponManagerComponent weaponManager = CharacterWeaponManagerComponent.Cast(character.FindComponent(CharacterWeaponManagerComponent));
		if (weaponManager && !m_mCharacterWeaponManagers.Contains(characterEntity))
		{
			weaponManager.m_OnWeaponChangeCompleteInvoker.Insert(OnCharacterWeaponChanged);
			m_mCharacterWeaponManagers.Insert(characterEntity, weaponManager);
		}

		array<IEntity> trackedShooters;
		if (!m_mCharacterShooters.Find(characterEntity, trackedShooters))
		{
			trackedShooters = {};
			m_mCharacterShooters.Insert(characterEntity, trackedShooters);
		}

		array<IEntity> currentShooters = {};
		CollectCharacterShooterSources(character, currentShooters);
		foreach (IEntity shooter : currentShooters)
		{
			if (!trackedShooters.Contains(shooter) && m_ProjectileTracer.TrackShooter(shooter, shooter == character, shooter == character))
				trackedShooters.Insert(shooter);
		}

		for (int shooterIndex = trackedShooters.Count() - 1; shooterIndex >= 0; shooterIndex--)
		{
			IEntity shooter = trackedShooters[shooterIndex];
			if (currentShooters.Contains(shooter))
				continue;

			m_ProjectileTracer.UntrackShooter(shooter);
			trackedShooters.RemoveOrdered(shooterIndex);
		}
	}

	protected void CollectCharacterShooterSources(IEntity character, array<IEntity> output)
	{
		if (!character || !output)
			return;

		output.Insert(character);
		CharacterWeaponManagerComponent weaponManager = CharacterWeaponManagerComponent.Cast(character.FindComponent(CharacterWeaponManagerComponent));
		if (!weaponManager)
			return;

		array<WeaponSlotComponent> weaponSlots = {};
		weaponManager.GetWeaponsSlots(weaponSlots);
		foreach (WeaponSlotComponent weaponSlot : weaponSlots)
		{
			if (!weaponSlot)
				continue;

			IEntity weaponEntity = weaponSlot.GetWeaponEntity();
			if (weaponEntity && weaponEntity.FindComponent(SCR_MuzzleEffectComponent) && !output.Contains(weaponEntity))
				output.Insert(weaponEntity);
		}
	}

	protected void RefreshVehicleShooters(SCR_EditableEntityComponent vehicle)
	{
		if (!vehicle || !m_ProjectileTracer)
			return;

		IEntity vehicleOwner = vehicle.GetOwner();
		if (!vehicleOwner)
			return;

		array<IEntity> trackedShooters;
		if (!m_mVehicleShooters.Find(vehicle, trackedShooters))
		{
			trackedShooters = {};
			m_mVehicleShooters.Insert(vehicle, trackedShooters);
		}

		array<IEntity> hierarchy = {vehicleOwner};
		SCR_EntityHelper.GetHierarchyEntityList(vehicleOwner, hierarchy);
		array<IEntity> currentShooters = {};
		foreach (IEntity hierarchyEntity : hierarchy)
		{
			array<Managed> turretComponents = {};
			hierarchyEntity.FindComponents(TurretControllerComponent, turretComponents);
			foreach (Managed component : turretComponents)
			{
				TurretControllerComponent turret = TurretControllerComponent.Cast(component);
				if (!turret)
					continue;

				BaseWeaponManagerComponent weaponManager = turret.GetWeaponManager();
				if (!weaponManager)
					continue;

				array<BaseWeaponComponent> weapons = {};
				weaponManager.GetWeapons(weapons);
				foreach (BaseWeaponComponent weapon : weapons)
				{
					if (!weapon)
						continue;

					IEntity shooter = weapon.GetOwner();
					if (!shooter || currentShooters.Contains(shooter))
						continue;

					currentShooters.Insert(shooter);
					if (!trackedShooters.Contains(shooter) && m_ProjectileTracer.TrackShooter(shooter, false, true))
						trackedShooters.Insert(shooter);
				}
			}
		}

		for (int shooterIndex = trackedShooters.Count() - 1; shooterIndex >= 0; shooterIndex--)
		{
			IEntity shooter = trackedShooters[shooterIndex];
			if (currentShooters.Contains(shooter))
				continue;

			m_ProjectileTracer.UntrackShooter(shooter);
			trackedShooters.RemoveOrdered(shooterIndex);
		}
	}

}

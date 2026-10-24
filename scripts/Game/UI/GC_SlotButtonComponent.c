class GC_SlotButtonComponent : SCR_ButtonBaseComponent
{
	ref ScriptInvoker<GC_Slot> m_OnSlotHovered = new ScriptInvoker<GC_Slot>();
	ref ScriptInvoker<GC_Slot> m_OnSlotHoverEnded = new ScriptInvoker<GC_Slot>();

	protected GC_Slot m_Slot;
	protected bool m_bSlotInteractable = true;
	protected bool m_bSlotHovered;
	protected bool m_bClickHoverGrace;
	protected ResourceName m_RoleInfoPrefab;
	protected ref SCR_UIInfo m_RoleInfo;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		// Sound playback is kept behind the slot interaction check.
		SetHoverSound(string.Empty);
		SetClickedSound(string.Empty);
		SetToggleable(false);
		m_bUseColorization = false;
		UpdateSlotVisuals();
	}

	override void HandlerDeattached(Widget w)
	{
		CancelClickHoverGrace();

		if (m_Slot)
			m_OnSlotHoverEnded.Invoke(m_Slot);

		if (m_Slot)
			m_Slot.m_OnChanged.Remove(OnSlotChanged);

		m_Slot = null;
		super.HandlerDeattached(w);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 1)
		{
			GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForSlot(m_Slot), m_wRoot);
			return true;
		}

		if (button != 0 || !CanInteract())
			return true;

		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController || !m_Slot)
			return true;

		PlaySound(SCR_SoundEvent.CLICK);
		playerController.RequestLobbySlot(m_Slot.GetID());

		// A mouse click may transfer focus and emit a transient leave. Keep the
		// slot visibly hovered while that input sequence settles.
		CancelClickHoverGrace();
		m_bSlotHovered = true;
		UpdateSlotVisuals();
		m_bClickHoverGrace = true;
		GetGame().GetCallqueue().CallLater(EndClickHoverGrace, 1);

		// The slot owns its click behavior. Consuming the event here avoids the
		// base button's temporary clicked-color transition while the server
		// processes the slot request.
		return true;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		CancelClickHoverGrace();

		if (m_Slot)
			m_OnSlotHovered.Invoke(m_Slot);

		if (!CanInteract())
			return true;

		m_bSlotHovered = true;
		PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
		UpdateSlotVisuals();
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		// A click can emit a leave during the same input sequence even though
		// the pointer never left the slot. Ignore only that one-frame event.
		if (m_bClickHoverGrace)
			return true;

		if (m_Slot)
			m_OnSlotHoverEnded.Invoke(m_Slot);

		m_bSlotHovered = false;
		UpdateSlotVisuals();

		return true;
	}

	protected void EndClickHoverGrace()
	{
		m_bClickHoverGrace = false;
	}

	protected void CancelClickHoverGrace()
	{
		GetGame().GetCallqueue().Remove(EndClickHoverGrace);
		m_bClickHoverGrace = false;
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		if (!CanInteract())
			return true;

		PlaySound(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
		return true;
	}

	override bool OnFocusLost(Widget w, int x, int y)
	{
		return true;
	}

	//! Binds the authoritative lobby slot used for interaction checks.
	void SetSlot(GC_Slot slot)
	{
		if (m_Slot)
			m_Slot.m_OnChanged.Remove(OnSlotChanged);

		m_Slot = slot;
		if (m_Slot)
			m_Slot.m_OnChanged.Insert(OnSlotChanged);

		UpdateSlotVisuals();
	}

	GC_Slot GetSlot()
	{
		return m_Slot;
	}

	//! Enables or disables local interaction without changing authoritative slot state.
	void SetSlotInteractable(bool interactable)
	{
		m_bSlotInteractable = interactable;
		if (!interactable)
			m_bSlotHovered = false;

		UpdateSlotVisuals();
	}

	bool IsSlotInteractable()
	{
		return CanInteract();
	}

	protected bool CanInteract()
	{
		if (!m_bSlotInteractable)
			return false;

		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		PlayerController playerController = GetGame().GetPlayerController();
		return lobbyManager && playerController && lobbyManager.CanPlayerRequestSlot(playerController.GetPlayerId(), m_Slot);
	}

	//! Re-evaluate shared eligibility without replacing this button or its focus.
	void RefreshInteractionPresentation()
	{
		UpdateSlotVisuals();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSlotVisuals()
	{
		if (!m_wRoot)
			return;

		UpdateRoleVisuals();

		ImageWidget background = ImageWidget.Cast(m_wRoot.FindAnyWidget("Background"));
		if (background)
		{
			if (!CanInteract())
				background.SetColor(Color.FromRGBA(1, 1, 1, 100));
			else if (m_bSlotHovered)
				background.SetColor(Color.FromRGBA(36, 42, 45, 210));
			else
				background.SetColor(Color.FromRGBA(1, 1, 1, 140));
		}

		ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("SelectionAccent"));
		if (accent)
		{
			UpdateFactionAccent(accent);

			if (!CanInteract())
				accent.SetOpacity(0.35);
			else if (m_bSlotHovered)
				accent.SetOpacity(1);
			else
				accent.SetOpacity(0.85);
		}

		string statusImage;
		string playerName;
		if (m_Slot)
		{
			if (m_Slot.IsKIA())
				statusImage = "death";
			else if (m_Slot.GetPlayerId() >= 0)
				statusImage = "player";
			else if (m_Slot.IsAwaitingInitialJoin())
				statusImage = "admin";
			else if (m_Slot.GetState() == GC_SlotState.DISCONNECTED)
				statusImage = "disconnection";
			else if (m_Slot.GetState() == GC_SlotState.RESERVED)
				statusImage = "admin";
			else if (m_Slot.CanAdminLock() && m_Slot.IsAdminLocked())
				statusImage = "server-locked";

			if (!statusImage.IsEmpty())
				playerName = m_Slot.GetCachedPlayerName();
		}

		GC_LobbyManager lotteryManager = GC_LobbyManager.GetManager();
		if (m_Slot && !m_Slot.HasClaim() && lotteryManager && lotteryManager.GetLotteryForSlot(m_Slot.GetID()))
		{
			statusImage = "gameMechanics";
			playerName = "Lottery open";
		}

		Widget statusSection = m_wRoot.FindAnyWidget("StatusSection");
		if (statusSection)
			statusSection.SetVisible(!statusImage.IsEmpty());

		ImageWidget statusIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("StatusIcon"));
		if (statusIcon)
		{
			statusIcon.SetVisible(!statusImage.IsEmpty());
			if (!statusImage.IsEmpty())
				statusIcon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", statusImage);
		}

		TextWidget statusText = TextWidget.Cast(m_wRoot.FindAnyWidget("StatusText"));
		if (statusText)
		{
			statusText.SetText(playerName);
			statusText.SetVisible(!playerName.IsEmpty());
			RefreshAdminNamePresentation();
		}
	}

	//------------------------------------------------------------------------------------------------
	void RefreshAdminNamePresentation()
	{
		if (!m_wRoot)
			return;

		TextWidget statusText = TextWidget.Cast(m_wRoot.FindAnyWidget("StatusText"));
		if (!statusText)
			return;

		int playerId = -1;
		if (m_Slot)
			playerId = m_Slot.GetPlayerId();
		bool absent = m_Slot && (m_Slot.IsKIA() || m_Slot.IsAwaitingInitialJoin()
			|| m_Slot.GetState() == GC_SlotState.DISCONNECTED
			|| (m_Slot.GetState() == GC_SlotState.RESERVED && playerId <= 0));
		// Presence takes priority over admin highlighting, including role refreshes.
		if (absent)
			statusText.SetColor(Color.FromRGBA(170, 170, 170, 255));
		else
			GC_PlayerNamePresentation.Apply(statusText, playerId);
	}

	protected void UpdateFactionAccent(ImageWidget accent)
	{
		if (!accent || !m_Slot)
			return;

		SCR_AIGroup group = m_Slot.GetGroup();
		if (!group)
			return;

		Faction faction = group.GetFaction();
		if (!faction)
			return;

		accent.SetColor(faction.GetFactionColor());
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRoleVisuals()
	{
		TextWidget roleName = TextWidget.Cast(m_wRoot.FindAnyWidget("RoleName"));
		ImageWidget roleIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleIcon"));
		if (!roleName && !roleIcon)
			return;

		SCR_UIInfo roleInfo = GetRoleInfo();
		if (roleName)
		{
			string label;
			if (roleInfo)
				label = WidgetManager.Translate(roleInfo.GetName());
			roleName.SetText(label);
		}

		if (!roleIcon)
			return;

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		SCR_AIGroup group;
		if (m_Slot)
			group = m_Slot.GetGroup();
		if (manager && group && group.GetFaction() && manager.GetFactionCommanderSlot(group.GetFaction().GetFactionKey()) == m_Slot)
		{
			roleIcon.LoadImageFromSet(0, "{EE0B4CDEA86FC24A}UI/Textures/DeployMenu/LoadoutIcons/LoadoutIcons.imageset", "EditableEntity_Character_SquadLeaderALT");
			roleIcon.SetOpacity(1);
		}
		else if (roleInfo && roleInfo.SetIconTo(roleIcon))
			roleIcon.SetOpacity(1);
		else
			roleIcon.SetOpacity(0);
	}

	protected SCR_UIInfo GetRoleInfo()
	{
		if (!m_Slot)
			return null;

		ResourceName prefab = m_Slot.GetAuthoredPrefab();
		if (prefab == m_RoleInfoPrefab)
			return m_RoleInfo;

		m_RoleInfoPrefab = prefab;
		m_RoleInfo = null;

		m_RoleInfo = GC_Slot.GetPrefabRoleInfo(prefab);
		return m_RoleInfo;
	}


	protected void OnSlotChanged(GC_Slot slot)
	{
		if (slot == m_Slot)
			UpdateSlotVisuals();
	}
}

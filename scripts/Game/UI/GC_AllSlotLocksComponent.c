//! Lobby-header admin controls. Slot locks and faction rules remain server-owned.
class GC_AllSlotLocksComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_Controller;
	protected SCR_ButtonBaseComponent m_Button;
	protected ImageWidget m_Icon;
	protected SCR_ButtonTextComponent m_RatioButton;
	protected ImageWidget m_RatioIcon;
	protected TextWidget m_Status;
	protected int m_iPending = -1;
	protected bool m_bLockRequested;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		if (SCR_Global.IsEditMode())
			return;
		m_Manager = GC_LobbyManager.GetManager();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Button = SCR_ButtonBaseComponent.Cast(w.FindAnyWidget("AllSlotLocksButton").FindHandler(SCR_ButtonBaseComponent));
		m_Icon = ImageWidget.Cast(w.FindAnyWidget("AllSlotLocksImage"));
		m_RatioButton = SCR_ButtonTextComponent.GetButtonText("FactionRatioButton", w);
		m_RatioIcon = ImageWidget.Cast(w.FindAnyWidget("FactionRatioImage"));
		m_Status = TextWidget.Cast(w.FindAnyWidget("AllSlotLocksStatus"));
		m_Button.m_OnClicked.Insert(OnClicked);
		m_RatioButton.m_OnClicked.Insert(OnClicked);
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Insert(OnSlotChanged);
			m_Manager.m_OnRegistryReset.Insert(OnRegistryChanged);
			m_Manager.m_OnFactionTopologyChanged.Insert(OnTopologyChanged);
			m_Manager.m_OnPlayerRoleChanged.Insert(OnRegistryChanged);
			m_Manager.m_OnFactionRulesChanged.Insert(OnRegistryChanged);
			m_Manager.m_OnLobbyPhaseChanged.Insert(OnPhaseChanged);
		}
		if (m_Controller)
		{
			m_Controller.m_OnAdminGroupLockResult.Insert(OnResult);
			m_Controller.m_OnAdminFactionRulesResult.Insert(OnResult);
		}
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Manager && m_Manager == GC_LobbyManager.GetManager() && m_Controller
			&& SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		bool access = HasAccess();
		m_Button.GetRootWidget().SetVisible(access);
		m_Button.SetEnabled(access && m_iPending < 0 && !m_Manager.GetSlots().IsEmpty());
		m_RatioButton.GetRootWidget().SetVisible(access);
		m_RatioButton.SetEnabled(access && m_iPending < 0 && m_Manager.GetLobbyPhase() != SCR_EGameModeState.POSTGAME);
		m_Status.SetVisible(access);
		if (!access)
			return;

		bool enforceRatio = m_Manager.IsFactionBalanceEnforced();
		m_RatioButton.SetToggled(enforceRatio, false, false);
		if (enforceRatio)
			m_RatioIcon.SetColor(Color.White);
		else
			m_RatioIcon.SetColor(Color.Red);
		if (m_iPending >= 0)
		{
			m_Status.SetText("Waiting for server...");
			return;
		}

		m_bLockRequested = true;
		foreach (GC_Slot slot : m_Manager.GetSlots())
		{
			if (slot.IsAdminLocked())
			{
				m_bLockRequested = false;
				break;
			}
		}
		if (m_bLockRequested)
			m_Icon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", "server-unlocked");
		else
			m_Icon.LoadImageFromSet(0, "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", "server-locked");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnClicked(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || m_iPending >= 0 || !button.GetRootWidget().IsEnabled())
			return;
		// Send the displayed direction, never a server-side toggle that can invert on retry.
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		m_Status.SetText("");
		Refresh();
		if (button == m_RatioButton)
		{
			// Preserve faction-lock policy; the displayed revision prevents stale overwrites.
			m_Controller.RequestAdminFactionRules(m_Manager.GetFactionRulesRevision(),
				m_Manager.GetFactionLockOverride(), !m_Manager.IsFactionBalanceEnforced(), m_iPending);
		}
		else
			m_Controller.RequestAdminGroupLock(RplId.Invalid(), m_bLockRequested, m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(int requestId, string outcome)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		m_Status.SetText("");
		if (outcome != "applied")
			m_Status.SetText(GC_AdminActionResultText.Get(outcome));
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlotChanged(GC_Slot slot)
	{
		// Bulk updates emit several slot events in one tick; refresh the aggregate once.
		GetGame().GetCallqueue().Remove(Refresh);
		GetGame().GetCallqueue().CallLater(Refresh, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRegistryChanged(int value)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTopologyChanged(Faction faction)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPhaseChanged(SCR_EGameModeState phase)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Refresh);
		if (m_Button)
			m_Button.m_OnClicked.Remove(OnClicked);
		if (m_RatioButton)
			m_RatioButton.m_OnClicked.Remove(OnClicked);
		if (m_Controller)
		{
			m_Controller.m_OnAdminGroupLockResult.Remove(OnResult);
			m_Controller.m_OnAdminFactionRulesResult.Remove(OnResult);
		}
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlotChanged);
			m_Manager.m_OnRegistryReset.Remove(OnRegistryChanged);
			m_Manager.m_OnFactionTopologyChanged.Remove(OnTopologyChanged);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnRegistryChanged);
			m_Manager.m_OnFactionRulesChanged.Remove(OnRegistryChanged);
			m_Manager.m_OnLobbyPhaseChanged.Remove(OnPhaseChanged);
		}
		super.HandlerDeattached(w);
	}
}

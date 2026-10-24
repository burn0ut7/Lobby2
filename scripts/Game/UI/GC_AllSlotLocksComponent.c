//! One lobby-header padlock. Slot records remain the availability authority.
class GC_AllSlotLocksComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyManager m_Manager;
	protected GC_LobbyPlayerController m_Controller;
	protected SCR_ButtonBaseComponent m_Button;
	protected ImageWidget m_Icon;
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
		m_Status = TextWidget.Cast(w.FindAnyWidget("AllSlotLocksStatus"));
		m_Button.m_OnClicked.Insert(OnClicked);
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Insert(OnSlotChanged);
			m_Manager.m_OnRegistryReset.Insert(OnRegistryChanged);
			m_Manager.m_OnFactionTopologyChanged.Insert(OnTopologyChanged);
			m_Manager.m_OnPlayerRoleChanged.Insert(OnRegistryChanged);
		}
		if (m_Controller)
			m_Controller.m_OnAdminGroupLockResult.Insert(OnResult);
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
		m_Status.SetVisible(access);
		if (!access)
			return;

		m_Button.SetEnabled(m_iPending < 0 && !m_Manager.GetSlots().IsEmpty());
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
		if (!HasAccess() || m_iPending >= 0 || m_Manager.GetSlots().IsEmpty())
			return;
		// Send the displayed direction, never a server-side toggle that can invert on retry.
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		m_Status.SetText("");
		Refresh();
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
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Refresh);
		if (m_Button)
			m_Button.m_OnClicked.Remove(OnClicked);
		if (m_Controller)
			m_Controller.m_OnAdminGroupLockResult.Remove(OnResult);
		if (m_Manager)
		{
			m_Manager.m_OnSlotPresentationChanged.Remove(OnSlotChanged);
			m_Manager.m_OnRegistryReset.Remove(OnRegistryChanged);
			m_Manager.m_OnFactionTopologyChanged.Remove(OnTopologyChanged);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnRegistryChanged);
		}
		super.HandlerDeattached(w);
	}
}

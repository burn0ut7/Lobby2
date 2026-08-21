//! Right-click target for the header of one slotting group card.
class GC_GroupHeaderContextComponent : SCR_ScriptedWidgetComponent
{
	protected SCR_AIGroup m_Group;

	void SetGroup(SCR_AIGroup group)
	{
		m_Group = group;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 1 || !m_Group)
			return false;

		GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForGroup(m_Group), m_wRoot);
		return true;
	}
}

class GC_GroupWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName SLOT_LAYOUT = "{6A24000000000001}UI/layouts/Groups/Lobby_Slot.layout";
	ref ScriptInvoker<GC_Slot> m_OnSlotHovered = new ScriptInvoker<GC_Slot>();
	ref ScriptInvoker<GC_Slot> m_OnSlotHoverEnded = new ScriptInvoker<GC_Slot>();

	protected Widget m_wSlots;
	protected TextWidget m_wSquadName;
	protected TextWidget m_wPlayerCount;
	protected GC_GroupHeaderContextComponent m_GroupHeaderContext;
	protected GC_LobbyManager m_LobbyManager;
	protected SCR_AIGroup m_Group;
	protected ref array<Widget> m_aSlotWidgets = {};

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		m_wSlots = m_wRoot.FindAnyWidget("Slots");
		m_wSquadName = TextWidget.Cast(m_wRoot.FindAnyWidget("SquadName"));
		m_wPlayerCount = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerCount"));
		Widget groupHeader = m_wRoot.FindAnyWidget("SquadHeader");
		if (groupHeader)
			m_GroupHeaderContext = GC_GroupHeaderContextComponent.Cast(groupHeader.FindHandler(GC_GroupHeaderContextComponent));
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_wSlots || !m_LobbyManager)
			return;

		m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnFactionUpdated);
		m_LobbyManager.m_OnFactionPresentationChanged.Insert(OnFactionUpdated);
		ClearSlotWidgets();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnFactionUpdated);
			m_LobbyManager.m_OnFactionPresentationChanged.Remove(OnFactionUpdated);
		}

		ClearSlotWidgets();
		m_Group = null;
		m_LobbyManager = null;
		m_wPlayerCount = null;
		m_wSquadName = null;
		m_GroupHeaderContext = null;
		m_wSlots = null;
		super.HandlerDeattached(w);
	}

	//! Binds this presentation to one authoritative base-game group.
	void SetGroup(SCR_AIGroup group)
	{
		if (m_Group == group)
		{
			Refresh();
			return;
		}

		m_Group = group;
		if (m_GroupHeaderContext)
			m_GroupHeaderContext.SetGroup(group);
		Refresh();
	}

	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}

	//! Rebuilds slot rows from the lobby manager's derived slot index.
	void Refresh()
	{
		if (!m_LobbyManager || !m_wSlots)
			return;

		UpdateGroupName();
		UpdateFactionAccent();
		ClearSlotWidgets();

		int slotCount;
		if (m_Group)
		{
			array<ref GC_Slot> slots = m_LobbyManager.GetSlotsForGroup(m_Group);
			slotCount = slots.Count();

			foreach (GC_Slot slot : slots)
			{
				if (!slot)
					continue;

				Widget slotWidget = GetGame().GetWorkspace().CreateWidgets(SLOT_LAYOUT, m_wSlots);
				if (!slotWidget)
					continue;

				GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(slotWidget.FindHandler(GC_SlotButtonComponent));
				if (slotButton)
				{
					slotButton.m_OnSlotHovered.Insert(OnSlotHovered);
					slotButton.m_OnSlotHoverEnded.Insert(OnSlotHoverEnded);
					slotButton.SetSlot(slot);
				}

				m_aSlotWidgets.Insert(slotWidget);
			}
		}

		UpdatePlayerCount(slotCount);
	}

	protected void UpdateGroupName()
	{
		if (!m_wSquadName)
			return;

		m_wSquadName.SetText(GC_GroupCallsignHelper.GetDisplayName(m_Group));
	}

	void RefreshAdminNamePresentation(int playerId = -1)
	{
		foreach (Widget slotWidget : m_aSlotWidgets)
		{
			if (!slotWidget)
				continue;

			GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(slotWidget.FindHandler(GC_SlotButtonComponent));
			if (slotButton && (playerId <= 0 || (slotButton.GetSlot() && slotButton.GetSlot().GetPlayerId() == playerId)))
				slotButton.RefreshAdminNamePresentation();
		}
	}

	protected void UpdatePlayerCount(int slotCount = -1)
	{
		if (!m_wPlayerCount)
			return;

		int playerCount;
		if (m_Group)
		{
			playerCount = m_LobbyManager.GetGroupPlayerCount(m_Group);
			if (slotCount < 0)
				slotCount = m_LobbyManager.GetSlotsForGroup(m_Group).Count();
		}

		if (slotCount < 0)
			slotCount = 0;

		m_wPlayerCount.SetTextFormat("%1 / %2", playerCount, slotCount);
	}

	protected bool HasCurrentSlotTopology()
	{
		if (!m_Group || !m_LobbyManager)
			return m_aSlotWidgets.IsEmpty();

		array<ref GC_Slot> slots = m_LobbyManager.GetSlotsForGroup(m_Group);
		if (slots.Count() != m_aSlotWidgets.Count())
			return false;

		for (int i = 0; i < slots.Count(); i++)
		{
			Widget slotWidget = m_aSlotWidgets[i];
			GC_SlotButtonComponent slotButton;
			if (slotWidget)
				slotButton = GC_SlotButtonComponent.Cast(slotWidget.FindHandler(GC_SlotButtonComponent));

			if (!slotButton || slotButton.GetSlot() != slots[i])
				return false;
		}

		return true;
	}

	protected void UpdateFactionAccent()
	{
		if (!m_Group)
			return;

		ImageWidget accent = ImageWidget.Cast(m_wRoot.FindAnyWidget("SquadAccent"));
		if (!accent)
			return;

		Faction faction = m_Group.GetFaction();
		if (!faction)
			return;

		accent.SetColor(faction.GetFactionColor());
	}

	protected void OnFactionUpdated(Faction faction)
	{
		if (!m_Group)
			return;

		Faction groupFaction = m_Group.GetFaction();
		if (!faction || (groupFaction && groupFaction.GetFactionKey() == faction.GetFactionKey()))
		{
			if (!HasCurrentSlotTopology())
			{
				Refresh();
				return;
			}

			// Existing slot buttons subscribe directly to their GC_Slot. Keep
			// them alive so hover/pressed presentation is not reset by a routine
			// occupancy update.
			UpdateFactionAccent();
			UpdatePlayerCount();
		}
	}

	protected void ClearSlotWidgets()
	{
		if (m_aSlotWidgets.IsEmpty() && m_wSlots)
		{
			Widget previewSlot = m_wSlots.GetChildren();
			while (previewSlot)
			{
				Widget nextSlot = previewSlot.GetSibling();
				previewSlot.RemoveFromHierarchy();
				previewSlot = nextSlot;
			}
		}
		else
		{
			foreach (Widget slotWidget : m_aSlotWidgets)
			{
				if (slotWidget)
				{
					GC_SlotButtonComponent slotButton = GC_SlotButtonComponent.Cast(slotWidget.FindHandler(GC_SlotButtonComponent));
					if (slotButton)
					{
						slotButton.m_OnSlotHovered.Remove(OnSlotHovered);
						slotButton.m_OnSlotHoverEnded.Remove(OnSlotHoverEnded);
					}

					slotWidget.RemoveFromHierarchy();
				}
			}
		}

		m_aSlotWidgets.Clear();
	}

	protected void OnSlotHovered(GC_Slot slot)
	{
		m_OnSlotHovered.Invoke(slot);
	}

	protected void OnSlotHoverEnded(GC_Slot slot)
	{
		m_OnSlotHoverEnded.Invoke(slot);
	}
}

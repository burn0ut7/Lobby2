class GC_FactionWidgetComponent : SCR_ScriptedWidgetComponent
{
	protected FactionKey m_FactionKey;
	protected GC_LobbyManager m_LobbyManager;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (SCR_Global.IsEditMode())
			return;

		m_LobbyManager = GC_LobbyManager.GetManager();
		if (!m_LobbyManager)
			return;

		m_LobbyManager.m_OnFactionRulesChanged.Insert(OnBalanceChanged);
		m_LobbyManager.m_OnPlayerConnected.Insert(OnBalanceChanged);
		m_LobbyManager.m_OnPlayerDisconnected.Insert(OnBalanceChanged);
		m_LobbyManager.m_OnRegistryReset.Insert(OnBalanceChanged);
		m_LobbyManager.m_OnFactionPresentationChanged.Insert(OnFactionUpdated);
		m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnFactionUpdated);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_LobbyManager)
		{
			m_LobbyManager.m_OnFactionRulesChanged.Remove(OnBalanceChanged);
			m_LobbyManager.m_OnPlayerConnected.Remove(OnBalanceChanged);
			m_LobbyManager.m_OnPlayerDisconnected.Remove(OnBalanceChanged);
			m_LobbyManager.m_OnRegistryReset.Remove(OnBalanceChanged);
			m_LobbyManager.m_OnFactionPresentationChanged.Remove(OnFactionUpdated);
			m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnFactionUpdated);
		}

		GetGame().GetCallqueue().Remove(Refresh);
		m_LobbyManager = null;
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns the faction represented by this runtime-created card.
	void SetFactionKey(FactionKey factionKey)
	{
		if (m_FactionKey == factionKey)
			return;

		m_FactionKey = factionKey;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionUpdated(Faction faction)
	{
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBalanceChanged(int value)
	{
		// Native disconnect cleanup can finish after its event is dispatched.
		GetGame().GetCallqueue().Remove(Refresh);
		GetGame().GetCallqueue().CallLater(Refresh, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_LobbyManager || m_LobbyManager != GC_LobbyManager.GetManager())
			return;

		TextWidget text = TextWidget.Cast(m_wRoot.FindAnyWidget("Text"));
		TextWidget counter = TextWidget.Cast(m_wRoot.FindAnyWidget("Counter"));
		ImageWidget flag = ImageWidget.Cast(m_wRoot.FindAnyWidget("Flag"));
		Faction faction = m_LobbyManager.GetRegisteredFaction(m_FactionKey);
		if (!faction)
		{
			if (text)
				text.SetText("");

			if (counter)
				counter.SetText("");

			if (flag)
				flag.SetOpacity(0);

			return;
		}

		UIInfo factionUIInfo = faction.GetUIInfo();
		if (text)
		{
			if (factionUIInfo)
				text.SetText(WidgetManager.Translate(factionUIInfo.GetName()));
			else
				text.SetText(WidgetManager.Translate(faction.GetFactionName()));
		}

		if (counter)
		{
			int currentCount = m_LobbyManager.GetFactionPlayerCount(m_FactionKey);
			int availableCount = m_LobbyManager.GetFactionSlotCount(m_FactionKey, false);
			if (availableCount == 0)
				counter.SetText("Locked");
			else
				counter.SetTextFormat("%1 / %2", currentCount, availableCount);
		}

		if (flag)
		{
			if (factionUIInfo && factionUIInfo.GetIconPath() != string.Empty)
			{
				flag.LoadImageTexture(0, factionUIInfo.GetIconPath());
				flag.SetOpacity(1);
			}
			else
			{
				flag.SetOpacity(0);
			}
		}

		ImageWidget color = ImageWidget.Cast(m_wRoot.FindAnyWidget("Color"));
		if (color)
		{
			Color factionColor = faction.GetFactionColor();
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (scrFaction)
				factionColor = scrFaction.GetOutlineFactionColor();

			color.SetColor(factionColor);
		}
	}
}

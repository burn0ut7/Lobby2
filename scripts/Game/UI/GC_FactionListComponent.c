class GC_FactionListComponent : SCR_ScriptedWidgetComponent
{
    protected static const ResourceName FACTION_CARD_LAYOUT = "{BF292F3C1D6C5FB5}UI/layouts/Factions/Lobby_Faction.layout";

    protected Widget m_wContent;
    protected GC_LobbyManager m_LobbyManager;
    protected ref map<FactionKey, Widget> m_mFactionCards = new map<FactionKey, Widget>();
    protected FactionKey m_SelectedFactionKey;

    ref ScriptInvoker<FactionKey> m_OnFactionSelected = new ScriptInvoker<FactionKey>();

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        m_wContent = m_wRoot.FindAnyWidget("FactionContent");
        m_LobbyManager = GC_LobbyManager.GetManager();
        if (!m_wContent || !m_LobbyManager)
            return;

        m_LobbyManager.m_OnFactionTopologyChanged.Insert(OnFactionUpdated);
		m_LobbyManager.m_OnRegistryReset.Insert(OnRegistryReset);
        Refresh();
    }

    override void HandlerDeattached(Widget w)
    {
        if (m_LobbyManager)
        {
            m_LobbyManager.m_OnFactionTopologyChanged.Remove(OnFactionUpdated);
			m_LobbyManager.m_OnRegistryReset.Remove(OnRegistryReset);
		}

        ClearCards();
        m_LobbyManager = null;
        m_wContent = null;
        super.HandlerDeattached(w);
    }

    //! Rebuilds the presentation from the lobby manager's authoritative faction index.
    void Refresh()
    {
        if (!m_LobbyManager || !m_wContent)
            return;

        array<Faction> factions = m_LobbyManager.GetRegisteredFactions();
        FactionKey previousSelection = m_SelectedFactionKey;

        ClearCards();

        bool selectionStillExists;
        foreach (Faction faction : factions)
        {
            if (faction && faction.GetFactionKey() == previousSelection)
                selectionStillExists = true;
        }

        m_SelectedFactionKey = string.Empty;
        foreach (Faction faction : factions)
        {
            if (!faction)
                continue;

			FactionKey factionKey = faction.GetFactionKey();
			EnsureFactionCard(factionKey);

            if (selectionStillExists && factionKey == previousSelection)
                m_SelectedFactionKey = factionKey;
        }

        UpdateSelectionVisuals();
    }

    FactionKey GetSelectedFactionKey()
    {
        return m_SelectedFactionKey;
    }

    //! Selects a faction and optionally notifies the owning menu of the user action.
    void SetSelectedFaction(FactionKey factionKey, bool invokeAction = true)
    {
        if (!FindFaction(factionKey))
            return;

        if (m_SelectedFactionKey == factionKey)
            return;

        m_SelectedFactionKey = factionKey;
        UpdateSelectionVisuals();

        if (invokeAction)
            m_OnFactionSelected.Invoke(factionKey);
    }

    protected void OnFactionUpdated(Faction faction)
    {
		if (!faction)
		{
			ReconcileFactionCards();
			return;
		}

		FactionKey factionKey = faction.GetFactionKey();
		if (m_LobbyManager.GetRegisteredFaction(factionKey))
			EnsureFactionCard(factionKey);
		else
			RemoveFactionCard(factionKey);
		UpdateSelectionVisuals();
    }

	protected void ReconcileFactionCards()
	{
		array<FactionKey> desiredKeys = {};
		foreach (Faction faction : m_LobbyManager.GetRegisteredFactions())
		{
			if (!faction)
				continue;
			FactionKey factionKey = faction.GetFactionKey();
			desiredKeys.Insert(factionKey);
			EnsureFactionCard(factionKey);
		}

		array<FactionKey> removedKeys = {};
		foreach (FactionKey existingKey, Widget card : m_mFactionCards)
		{
			if (!desiredKeys.Contains(existingKey))
				removedKeys.Insert(existingKey);
		}
		foreach (FactionKey removedKey : removedKeys)
			RemoveFactionCard(removedKey);
		UpdateSelectionVisuals();
	}

	protected void OnRegistryReset(int revision)
	{
		Refresh();
	}

    protected void OnFactionButtonClicked(SCR_ButtonBaseComponent baseButton)
    {
        GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(baseButton);
        if (!button)
            return;

        SetSelectedFaction(button.GetFactionKey());
    }

    protected bool FindFaction(FactionKey factionKey)
    {
        if (!m_LobbyManager)
            return false;

        return m_LobbyManager.GetRegisteredFaction(factionKey) != null;
    }

    protected void UpdateSelectionVisuals()
    {
		foreach (FactionKey factionKey, Widget card : m_mFactionCards)
        {
            if (!card)
                continue;

            GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(card.FindHandler(GC_FactionButtonComponent));
            if (button)
                button.SetFactionSelected(button.GetFactionKey() == m_SelectedFactionKey);
        }
    }

	protected void EnsureFactionCard(FactionKey factionKey)
	{
		if (factionKey.IsEmpty() || m_mFactionCards.Contains(factionKey))
			return;

		Widget card = GetGame().GetWorkspace().CreateWidgets(FACTION_CARD_LAYOUT, m_wContent);
		if (!card)
			return;

		GC_FactionWidgetComponent factionWidget = GC_FactionWidgetComponent.Cast(card.FindHandler(GC_FactionWidgetComponent));
		if (factionWidget)
			factionWidget.SetFactionKey(factionKey);

		GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(card.FindHandler(GC_FactionButtonComponent));
		if (button)
		{
			button.SetFactionKey(factionKey);
			button.m_OnClicked.Insert(OnFactionButtonClicked);
		}

		m_mFactionCards.Set(factionKey, card);
	}

	protected void RemoveFactionCard(FactionKey factionKey)
	{
		Widget card;
		if (!m_mFactionCards.Find(factionKey, card))
			return;

		GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(card.FindHandler(GC_FactionButtonComponent));
		if (button)
			button.m_OnClicked.Remove(OnFactionButtonClicked);
		if (card)
			card.RemoveFromHierarchy();
		m_mFactionCards.Remove(factionKey);

		if (m_SelectedFactionKey == factionKey)
		{
			m_SelectedFactionKey = string.Empty;
			m_OnFactionSelected.Invoke(m_SelectedFactionKey);
		}
	}

    protected void ClearCards()
    {
		foreach (FactionKey factionKey, Widget card : m_mFactionCards)
        {
            if (!card)
                continue;

            GC_FactionButtonComponent button = GC_FactionButtonComponent.Cast(card.FindHandler(GC_FactionButtonComponent));
            if (button)
                button.m_OnClicked.Remove(OnFactionButtonClicked);

            card.RemoveFromHierarchy();
        }

		m_mFactionCards.Clear();
    }
}

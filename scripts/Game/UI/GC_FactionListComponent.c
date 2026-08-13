class GC_FactionListComponent : SCR_ScriptedWidgetComponent
{
    protected static const ResourceName FACTION_CARD_LAYOUT = "{BF292F3C1D6C5FB5}UI/layouts/Factions/Lobby_Faction.layout";

    protected Widget m_wContent;
    protected GC_LobbyManager m_LobbyManager;
    protected ref array<Widget> m_aFactionCards = {};
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

        m_LobbyManager.m_OnFactionUpdated.Insert(OnFactionUpdated);
        Refresh();
    }

    override void HandlerDeattached(Widget w)
    {
        if (m_LobbyManager)
            m_LobbyManager.m_OnFactionUpdated.Remove(OnFactionUpdated);

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

            Widget card = GetGame().GetWorkspace().CreateWidgets(FACTION_CARD_LAYOUT, m_wContent);
            if (!card)
                continue;

            FactionKey factionKey = faction.GetFactionKey();
            GC_FactionWidgetComponent factionWidget = GC_FactionWidgetComponent.Cast(card.FindHandler(GC_FactionWidgetComponent));
            if (factionWidget)
                factionWidget.SetFactionKey(factionKey);

            Widget buttonRoot = card.FindAnyWidget("Button");
            GC_FactionButtonComponent button;
            if (buttonRoot)
                button = GC_FactionButtonComponent.Cast(buttonRoot.FindHandler(GC_FactionButtonComponent));
            if (button)
            {
                button.SetFactionKey(factionKey);
                button.m_OnClicked.Insert(OnFactionButtonClicked);
            }

            m_aFactionCards.Insert(card);

            if ((selectionStillExists && factionKey == previousSelection) || (!selectionStillExists && m_SelectedFactionKey.IsEmpty()))
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
        foreach (Widget card : m_aFactionCards)
        {
            if (!card)
                continue;

            Widget buttonRoot = card.FindAnyWidget("Button");
            GC_FactionButtonComponent button;
            if (buttonRoot)
                button = GC_FactionButtonComponent.Cast(buttonRoot.FindHandler(GC_FactionButtonComponent));
            if (button)
                button.SetFactionSelected(button.GetFactionKey() == m_SelectedFactionKey);
        }
    }

    protected void ClearCards()
    {
        foreach (Widget card : m_aFactionCards)
        {
            if (!card)
                continue;

            Widget buttonRoot = card.FindAnyWidget("Button");
            GC_FactionButtonComponent button;
            if (buttonRoot)
                button = GC_FactionButtonComponent.Cast(buttonRoot.FindHandler(GC_FactionButtonComponent));
            if (button)
                button.m_OnClicked.Remove(OnFactionButtonClicked);

            card.RemoveFromHierarchy();
        }

        m_aFactionCards.Clear();
    }
}

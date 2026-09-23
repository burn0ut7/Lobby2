class GC_FactionWidgetComponent : SCR_ScriptedWidgetComponent
{
    protected FactionKey m_FactionKey;
    protected GC_LobbyManager m_LobbyManager;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        m_LobbyManager = GC_LobbyManager.GetManager();
        if (!m_LobbyManager)
            return;

        m_LobbyManager.m_OnFactionPresentationChanged.Insert(OnFactionUpdated);
        Refresh();
    }

    override void HandlerDeattached(Widget w)
    {
        if (m_LobbyManager)
            m_LobbyManager.m_OnFactionPresentationChanged.Remove(OnFactionUpdated);

        m_LobbyManager = null;
        super.HandlerDeattached(w);
    }

    //! Assigns the faction represented by this runtime-created card.
    void SetFactionKey(FactionKey factionKey)
    {
        if (m_FactionKey == factionKey)
            return;

        m_FactionKey = factionKey;
        Refresh();
    }

    FactionKey GetFactionKey()
    {
        return m_FactionKey;
    }

    protected void OnFactionUpdated(Faction faction)
    {
        if (!faction || faction.GetFactionKey() != m_FactionKey)
            return;

        Refresh();
    }

    protected void Refresh()
    {
        if (!m_LobbyManager)
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
            int totalCount = m_LobbyManager.GetFactionSlotCount(m_FactionKey);
            counter.SetTextFormat("%1 / %2", currentCount, totalCount);
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
            color.SetColor(GC_LobbyUIColors.FactionBackground(faction));

        Widget accent = m_wRoot.FindAnyWidget("FactionAccent");
        if (accent)
            accent.SetColor(faction.GetFactionColor());
    }
}

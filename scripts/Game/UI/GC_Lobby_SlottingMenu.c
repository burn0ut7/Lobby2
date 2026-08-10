modded enum ChimeraMenuPreset
{
    GC_Lobby_SlottingMenu
};

class GC_Lobby_SlottingMenu : ChimeraMenuBase
{
    protected Widget m_wRoot;
    protected ref array<SCR_ButtonTextComponent> m_aHeaderButtons = {};

    override void OnMenuOpen()
    {
        PrintFormat("GC | OnMenuOpen");

        m_wRoot = GetRootWidget();

        ItemPreviewWidget render = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("Player"));

        ChimeraWorld world = GetGame().GetWorld();
        ItemPreviewManagerEntity manager = world.GetItemPreviewManager();

        manager.SetPreviewItemFromPrefab(render, "{9C2A994C94376FFE}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Ammo_random.et");

        RegisterHeaderButton("Mission Preview Button");
        RegisterHeaderButton("Lobby Button");
        RegisterHeaderButton("Briefing Button");
        RegisterHeaderButton("Game Button");
        RegisterHeaderButton("Debrief Button");

        // This menu opens in the lobby state.
        SetActiveHeaderButton(1);
        SetHeaderInputEnabled(CanChangeLobbyState());
    }

    protected void RegisterHeaderButton(string buttonName)
    {
        SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(buttonName, m_wRoot);
        if (!button)
        {
            PrintFormat("GC | Header button not found: %1", buttonName);
            return;
        }

        m_aHeaderButtons.Insert(button);
        button.m_OnClicked.Insert(OnHeaderButtonClicked);
        button.m_OnFocus.Insert(OnFocusHeaderButton);
    }

    protected bool CanChangeLobbyState()
    {
        // This controls presentation/input only. The server must validate the
        // same permission before applying any replicated game-state change.
        return SCR_Global.IsAdmin();
    }

    protected void SetHeaderInputEnabled(bool enabled)
    {
        for (int i = 0; i < m_aHeaderButtons.Count(); i++)
        {
            Widget buttonRoot = m_aHeaderButtons[i].GetRootWidget();
            if (buttonRoot)
                buttonRoot.SetEnabled(enabled);
        }
    }

    protected void SetActiveHeaderButton(int activeIndex)
    {
        for (int i = 0; i < m_aHeaderButtons.Count(); i++)
        {
            SCR_ButtonTextComponent button = m_aHeaderButtons[i];
            bool isActive = i == activeIndex;

            // Keep the standard button toggle state in sync.
            button.SetToggled(isActive, true, false, false);

            // Keep SCR_ButtonEffectColor/SCR_ButtonEffectOpacity in sync too.
            Widget buttonRoot = button.GetRootWidget();
            if (!buttonRoot)
                continue;

            SCR_ModularButtonComponent modularButton =
                SCR_ModularButtonComponent.Cast(buttonRoot.FindHandler(SCR_ModularButtonComponent));

            if (modularButton)
                modularButton.SetToggled(isActive, false, false);
        }
    }

    protected void OnHeaderButtonClicked(SCR_ButtonTextComponent button)
    {
        if (!CanChangeLobbyState())
        {
            SetHeaderInputEnabled(false);
            PrintFormat("GC | Header input rejected: local player is not an admin");
            return;
        }

        int buttonIndex = -1;
        for (int i = 0; i < m_aHeaderButtons.Count(); i++)
        {
            if (m_aHeaderButtons[i] == button)
            {
                buttonIndex = i;
                break;
            }
        }

        if (buttonIndex < 0)
            return;

        SetActiveHeaderButton(buttonIndex);

        switch (buttonIndex)
        {
            case 0:
                MissionPreview(button);
                break;
            case 1:
                LobbyState(button);
                break;
            case 2:
                BriefingState(button);
                break;
            case 3:
                GameState(button);
                break;
            case 4:
                DebriefState(button);
                break;
        }
    }

    protected void MissionPreview(SCR_ButtonTextComponent button)
    {
        bool isMissionPreviewToggled = button.IsToggled();
        PrintFormat("GC | MissionPreview Clicked: %1", isMissionPreviewToggled);

        // Prototype: find a child widget and customize its visual state.
        Widget buttonRoot = button.GetRootWidget();
        if (!buttonRoot)
        {
            PrintFormat("GC | MissionPreview: button root not found");
            return;
        }

        TextWidget text = TextWidget.Cast(buttonRoot.FindAnyWidget("Text"));
        if (!text)
        {
            PrintFormat("GC | MissionPreview: Text widget not found");
            return;
        }

        text.SetColor(Color.FromRGBA(255, 0, 0, 255));
    }

    protected void LobbyState(SCR_ButtonTextComponent button)
    {
        PrintFormat("GC | Lobby state selected");
    }

    protected void BriefingState(SCR_ButtonTextComponent button)
    {
        PrintFormat("GC | Briefing state selected");
    }

    protected void GameState(SCR_ButtonTextComponent button)
    {
        PrintFormat("GC | Game state selected");
    }

    protected void DebriefState(SCR_ButtonTextComponent button)
    {
        PrintFormat("GC | Debrief state selected");
    }

    protected void OnFocusHeaderButton(Widget widget)
    {
        if (!widget)
            return;

        PrintFormat("GC | OnFocusHeaderButton: %1", widget);

        Widget search = widget.FindAnyWidget("Border");
        if (search)
            search.SetOpacity(0.5);

        search = widget.FindAnyWidget("LineHighlight");
        if (search)
            search.SetOpacity(0.1);

        search = widget.FindAnyWidget("LineHighlight2");
        if (search)
            search.SetOpacity(1);

        ImageWidget image = ImageWidget.Cast(widget.FindAnyWidget("Image0"));
        if (image)
            image.SetColor(Color.FromRGBA(255, 152, 50, 255));

        TextWidget text = TextWidget.Cast(widget.FindAnyWidget("Text"));
        if (text)
            text.SetColor(Color.FromRGBA(255, 152, 50, 255));
    }
}

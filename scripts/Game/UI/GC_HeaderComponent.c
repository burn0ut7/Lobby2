class GC_HeaderComponent : SCR_ScriptedWidgetComponent
{
    ref ScriptInvoker<GC_HeaderComponent> m_OnStateChanged = new ScriptInvoker<GC_HeaderComponent>();

    protected ref array<GC_HeaderButtonComponent> m_aButtons = {};
    protected int m_iActiveState = -1;
    protected bool m_bCanChangeState;
    protected bool m_bInitialized;
    protected int m_iAdminCheckAttempts;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        GetGame().GetCallqueue().CallLater(InitializeHeader, 0);
    }

    protected void InitializeHeader()
    {
        if (m_bInitialized)
            return;

        m_bInitialized = true;

        RegisterButton("Mission Preview Button");
        RegisterButton("Lobby Button");
        RegisterButton("Briefing Button");
        RegisterButton("Game Button");
        RegisterButton("Debrief Button");

        // Mission Preview is the default header state.
        SetActiveState(0);
        SetCanChangeState(false);
        RefreshAdminPermission();
        PrintFormat("GC | Header initialized: buttons=%1", m_aButtons.Count());
    }

    protected void RefreshAdminPermission()
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
        {
            // Layout previews and early menu construction do not have a local player yet.
            // Retry briefly so a real game session can apply the permission when it becomes available.
            if (m_iAdminCheckAttempts++ < 20)
                GetGame().GetCallqueue().CallLater(RefreshAdminPermission, 250);

            return;
        }

        m_iAdminCheckAttempts = 0;
        SetCanChangeState(SCR_Global.IsAdmin());
    }

    protected void RegisterButton(string buttonName)
    {
        Widget buttonRoot = m_wRoot.FindAnyWidget(buttonName);
        if (!buttonRoot)
        {
            PrintFormat("GC | Header button not found: %1", buttonName);
            return;
        }

        GC_HeaderButtonComponent button =
            GC_HeaderButtonComponent.Cast(buttonRoot.FindHandler(GC_HeaderButtonComponent));
        if (!button)
        {
            PrintFormat("GC | Header component missing on button: %1", buttonName);
            return;
        }

        m_aButtons.Insert(button);
        button.m_OnClicked.Insert(OnButtonClicked);
    }

    void SetCanChangeState(bool canChange)
    {
        m_bCanChangeState = canChange;

        for (int i = 0; i < m_aButtons.Count(); i++)
            m_aButtons[i].SetHeaderEnabled(canChange);
    }

    bool CanChangeState()
    {
        return m_bCanChangeState;
    }

    void SetActiveState(int state)
    {
        m_iActiveState = state;

        for (int i = 0; i < m_aButtons.Count(); i++)
        {
            bool selected = m_aButtons[i].GetHeaderState() == m_iActiveState;
            m_aButtons[i].SetHeaderSelected(selected);
            m_aButtons[i].SetHeaderLocked(selected);

            // The selected button is a state indicator, not a hover target.
            if (selected)
                m_aButtons[i].SetHeaderHovered(false);
        }
    }

    int GetActiveState()
    {
        return m_iActiveState;
    }

    protected void OnButtonClicked(SCR_ButtonBaseComponent baseButton)
    {
        GC_HeaderButtonComponent button = GC_HeaderButtonComponent.Cast(baseButton);
        if (!button)
        {
            PrintFormat("GC | Header click received from an unexpected button component");
            return;
        }

        PrintFormat("GC | Header button clicked: state=%1 active=%2", button.GetHeaderState(), m_iActiveState);

        if (!m_bCanChangeState)
        {
            RefreshAdminPermission();
            PrintFormat("GC | Header input rejected: local player is not an admin");
            return;
        }

        // The active state is sticky. Select another button to change state;
        // clicking the active button again does nothing.
        if (m_iActiveState == button.GetHeaderState())
            return;

        SetActiveState(button.GetHeaderState());

        m_OnStateChanged.Invoke(this);
    }

}

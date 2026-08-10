modded enum ChimeraMenuPreset
{
	GC_Lobby_SlottingMenu
};

class GC_Lobby_SlottingMenu : ChimeraMenuBase
{
    protected Widget m_wRoot;

    override void OnMenuOpen()
    {
        PrintFormat("GC | OnMenuOpen");

        m_wRoot = GetRootWidget();
        
        ItemPreviewWidget render = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("Player"));
        
        ChimeraWorld world = GetGame().GetWorld();
        ItemPreviewManagerEntity manager = world.GetItemPreviewManager();

		manager.SetPreviewItemFromPrefab(render, "{9C2A994C94376FFE}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Ammo_random.et");
    
    


        SCR_ButtonTextComponent comp;
		comp = SCR_ButtonTextComponent.GetButtonText("Mission Preview Button", m_wRoot);
		if (comp)
		{
			GetGame().GetWorkspace().SetFocusedWidget(comp.GetRootWidget());
            comp.m_OnFocus.Insert(OnFocusMission);
			comp.m_OnClicked.Insert(MissionPreview);
		}
    
    }

    protected void MissionPreview(SCR_ButtonTextComponent button)
    {
        bool isMissionPreviewToggled = button.IsToggled();
        PrintFormat("GC | MissionPreview Clicked: %1", isMissionPreviewToggled);

        // Prototype: find a widget by name and customize its visual state.
        // Search from the clicked button so "Background" means this button's
        // child, not a similarly named widget elsewhere in the menu.
        Widget buttonRoot = button.GetRootWidget();
        if (!buttonRoot)
        {
            PrintFormat("GC | MissionPreview: button root not found");
            return;
        }
        Widget background = buttonRoot.FindAnyWidget("Text");
        background.SetColor(Color.FromRGBA(255, 0, 0, 255));
        if (!background)
        {
            PrintFormat("GC | MissionPreview: Background widget not found");
            return;
        }

        //SOUND_FE_BUTTON_CONFIRM
    }

    protected void OnFocusMission(Widget widget)
    {
        ButtonWidget button = ButtonWidget.Cast(widget);
        //if (button.)


        PrintFormat("GC | OnFocusMission: %1", widget);
        Widget search;

        search = widget.FindAnyWidget("Border");
        if (search)
        {
            search.SetOpacity(0.5);
        }
        
        search = widget.FindAnyWidget("LineHighlight");
        if (search)
        {
            //search.SetColor(Color.FromRGBA(255, 152, 50, 255));
            search.SetOpacity(0.1);
        }
    
        search = widget.FindAnyWidget("LineHighlight2");
        if (search)
        {
            search.SetOpacity(1);
        }

        ImageWidget img = ImageWidget.Cast(widget.FindAnyWidget("Image0"));
        if (img)
        {
            img.SetColor(Color.FromRGBA(255, 152, 50, 255));
        }
        

        TextWidget text = TextWidget.Cast(widget.FindAnyWidget("Text"));
        if (img)
        {
            text.SetColor(Color.FromRGBA(255, 152, 50, 255));
        }
        
        SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
    }
    
}

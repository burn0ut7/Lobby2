#ifdef WORKBENCH
//! Native combo list with the game's character-role glyphs and the source's custom icon.
class GC_LoadoutIconSelector : SCR_ComboBoxComponent
{
	protected static const ResourceName ROLE_ICONS = "{EE0B4CDEA86FC24A}UI/Textures/DeployMenu/LoadoutIcons/LoadoutIcons.imageset";

	//------------------------------------------------------------------------------------------------
	override void OpenList()
	{
		super.OpenList();
		// Open upward without reversing the visible A-Z order.
		if (m_wContent)
			m_wContent.SetFillOrigin(VerticalFillOrigin.TOP);
		ChimeraMenuBase menu = ChimeraMenuBase.GetOwnerMenu(m_wRoot);
		if (m_wElementsRoot && menu)
		{
			m_wElementsRoot.SetZOrder(menu.GetRootWidget().GetZOrder() + 1);
			FrameSlot.SetAlignment(m_wElementsRoot, 0, 1);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsExpanded()
	{
		return m_bOpened;
	}

	//------------------------------------------------------------------------------------------------
	void Populate(GC_LoadoutMetadata source)
	{
		ClearAll();
		// Names from the installed LoadoutIcons imageset; preserve its serialized spelling.
		array<string> roles = {"Ammo_Bearer", "Anti_Tank", "Crew", "Custom", "Dead", "Driver", "Empty", "Engineer", "Grenadier", "Helicopter_Crew", "Helicopter_Pilot", "Leader_Squad", "Leader_Team", "Machinegunner", "Medic", "Mortar", "Officer", "Player", "Radio_Operator", "Radio_Scout", "Rifleman", "Sapper", "Scout", "Sharpshooter", "Special_Forces", "Spotter", "SquadLeaderALT", "Unnarmed"};
		int selected = -1;
		array<string> labels = {};
		map<string, string> roleByLabel = new map<string, string>();
		bool knownIcon;
		string currentLabel = WidgetManager.Translate("#GC-Loadout_CurrentIcon");
		foreach (string role : roles)
		{
			if (source && source.m_sIcon == ROLE_ICONS && source.m_sIconName == "EditableEntity_Character_" + role)
				knownIcon = true;
			string label = role;
			label.Replace("_", " ");
			if (role == "Leader_Squad")
				label = "Squad leader";
			else if (role == "Leader_Team")
				label = "Team leader";
			else if (role == "SquadLeaderALT")
				label = "Squad leader (alternate)";
			else if (role == "Unnarmed")
				label = "Unarmed";
			labels.Insert(label);
			roleByLabel.Insert(label, role);
		}
		if (source && !knownIcon)
			labels.Insert(currentLabel);
		labels.Sort();
		foreach (string label : labels)
		{
			GC_LoadoutMetadata option = new GC_LoadoutMetadata();
			option.m_sIcon = ROLE_ICONS;
			option.m_sIconName = "EditableEntity_Character_" + roleByLabel.Get(label);
			if (source && !knownIcon && label == currentLabel)
				option = source;
			int index = AddItem(label, false, option);
			if (source && source.m_sIcon == option.m_sIcon && source.m_sIconName == option.m_sIconName)
				selected = index;
		}
		SetCurrentItem(selected, false, false, false);
	}

	//------------------------------------------------------------------------------------------------
	GC_LoadoutMetadata GetSelectedIcon()
	{
		int index = GetCurrentIndex();
		if (index < 0 || index >= m_aElementData.Count())
			return null;
		return GC_LoadoutMetadata.Cast(m_aElementData[index]);
	}

	//------------------------------------------------------------------------------------------------
	static void DrawIcon(ImageWidget image, GC_LoadoutMetadata icon)
	{
		bool visible = icon && !icon.m_sIcon.IsEmpty();
		image.SetVisible(visible);
		if (!visible)
			return;
		if (!icon.m_sIconName.IsEmpty())
			image.LoadImageFromSet(0, icon.m_sIcon, icon.m_sIconName);
		else
			image.LoadImageTexture(0, icon.m_sIcon);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnCreateElement(Widget elementWidget, int index)
	{
		super.OnCreateElement(elementWidget, index);
		DrawIcon(ImageWidget.Cast(elementWidget.FindAnyWidget("RoleIcon")), GC_LoadoutMetadata.Cast(m_aElementData[index]));
		AlignableSlot.SetPadding(elementWidget.FindAnyWidget("Text"), 64, 4, 16, 4);
	}
}
#endif

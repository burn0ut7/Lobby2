//! Modal native menu; file authoring is compiled only for Workbench.
class GC_LoadoutFilesMenu : ChimeraMenuBase
{
#ifdef WORKBENCH
	protected IEntity m_Owner;
	protected EditBoxWidget m_Faction;
	protected EditBoxWidget m_Group;
	protected EditBoxWidget m_Name;
	protected GC_LoadoutIconSelector m_Icon;
	protected TextWidget m_Status;
	protected ItemPreviewWidget m_Preview;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected ResourceName m_sSelected;
	protected ResourceName m_sOverwriteTarget;
	protected SCR_ConfigurableDialogUi m_OverwriteDialog;
	protected bool m_bCurrentGear = true;
	protected ref array<ResourceName> m_aResources = {};
	protected ref array<SCR_ButtonBaseComponent> m_aButtons = {};
	protected ref array<SCR_ButtonBaseComponent> m_aRows = {};

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Owner = GetGame().GetPlayerController().GetControlledEntity();
		if (!GC_WorkbenchItemBrowser.IsActive(m_Owner))
		{
			Close();
			return;
		}
		Widget root = GetRootWidget();
		m_Faction = EditBoxWidget.Cast(root.FindAnyWidget("Faction"));
		m_Group = EditBoxWidget.Cast(root.FindAnyWidget("Group"));
		m_Name = EditBoxWidget.Cast(root.FindAnyWidget("Name"));
		m_Icon = GC_LoadoutIconSelector.Cast(root.FindAnyWidget("RoleSelector").FindHandler(GC_LoadoutIconSelector));
		m_Icon.m_OnChanged.Insert(OnIconChanged);
		m_Status = TextWidget.Cast(root.FindAnyWidget("Status"));
		m_Preview = ItemPreviewWidget.Cast(root.FindAnyWidget("LoadoutPreview"));
		ChimeraWorld world = GetGame().GetWorld();
		if (world)
			m_PreviewManager = world.GetItemPreviewManager();
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(m_Owner.FindComponent(FactionAffiliationComponent));
		if (affiliation && affiliation.GetAffiliatedFaction())
			m_Faction.SetText(affiliation.GetAffiliatedFaction().GetFactionKey());
		m_Group.SetPlaceholderText("My Team");
		m_Name.SetPlaceholderText("Base");
		array<string> actions = {"Close", "Load", "Save", "Inherit", "Duplicate", "Current"};
		foreach (string action : actions)
		{
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(action, root);
			button.SetText("#GC-Loadout_" + action);
			button.m_OnClicked.Insert(OnAction);
			m_aButtons.Insert(button);
		}
		m_sSelected = GC_WorkbenchItemBrowser.GetEditingPrefab();
		RefreshList();
		SetFields(m_sSelected);
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	protected string Label(ResourceName resource)
	{
		return FilePath.StripExtension(FilePath.StripPath(resource.GetPath()));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFields(ResourceName resource)
	{
		GC_LoadoutMetadata metadata = GC_LoadoutMetadata.Read(resource);
		m_Icon.Populate(metadata);
		OnIconChanged(m_Icon, m_Icon.GetCurrentIndex());
		if (metadata)
			m_Faction.SetText(metadata.m_sFaction);
		if (!resource.GetPath().StartsWith("Prefabs/Loadouts/"))
			return;
		array<string> parts = {};
		string relativePath = resource.GetPath();
		relativePath.Replace("Prefabs/Loadouts/", "");
		FilePath.StripExtension(relativePath).Split("/", parts, true);
		if (parts.Count() != 3)
			return;
		m_Group.SetText(parts[1]);
		m_Name.SetText(parts[2]);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnIconChanged(SCR_ComboBoxComponent combo, int index)
	{
		GC_LoadoutIconSelector.DrawIcon(ImageWidget.Cast(GetRootWidget().FindAnyWidget("SelectedIcon")), m_Icon.GetSelectedIcon());
	}

	//------------------------------------------------------------------------------------------------
	protected GC_LoadoutMetadata GetMetadata()
	{
		GC_LoadoutMetadata icon = m_Icon.GetSelectedIcon();
		if (!icon)
			return null;
		GC_LoadoutMetadata metadata = new GC_LoadoutMetadata();
		metadata.m_sName = m_Name.GetText();
		metadata.m_sFaction = m_Faction.GetText();
		metadata.m_sIcon = icon.m_sIcon;
		metadata.m_sIconName = icon.m_sIconName;
		return metadata;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshList()
	{
		foreach (SCR_ButtonBaseComponent row : m_aRows)
		{
			row.m_OnClicked.Remove(OnSelect);
			row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
		m_aResources.Clear();
		// Authoring reads source files, including files created before the paused
		// Play-time resource database has incorporated them.
		string directory;
		if (Workbench.GetAbsolutePath("$Lobby2:Prefabs/Loadouts/", directory, false))
			FileIO.FindFiles(OnFile, directory, ".et");
		array<string> names = {};
		map<string, ResourceName> resources = new map<string, ResourceName>();
		foreach (ResourceName resource : m_aResources)
		{
			string key = Label(resource) + " " + resource;
			key.ToLower();
			names.Insert(key);
			resources.Insert(key, resource);
		}
		names.Sort();
		m_aResources.Clear();
		foreach (string key : names)
			m_aResources.Insert(resources.Get(key));
		foreach (ResourceName resource : m_aResources)
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", GetRootWidget().FindAnyWidget("SavedRows"));
			SCR_ButtonTextComponent row = SCR_ButtonTextComponent.Cast(widget.FindHandler(SCR_ButtonTextComponent));
			row.SetText(Label(resource));
			row.SetToggleable(true);
			SetSelection(row, !m_bCurrentGear && resource == m_sSelected);
			SetRoleIcon(widget, resource);
			row.m_OnClicked.Insert(OnSelect);
			m_aRows.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetSelection(SCR_ButtonBaseComponent button, bool selected)
	{
		button.SetToggled(selected, false, false);
		Widget edge = button.GetRootWidget().FindAnyWidget("SelectedEdge");
		if (edge)
			edge.SetVisible(selected);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRoleIcon(Widget widget, ResourceName prefab)
	{
		ImageWidget icon = ImageWidget.Cast(widget.FindAnyWidget("RoleIcon"));
		if (!icon)
			return;
		SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(prefab);
		bool visible = info && info.SetIconTo(icon);
		icon.SetVisible(visible);
		Widget label = widget.FindAnyWidget("Text");
		if (label && visible)
			AlignableSlot.SetPadding(label, 64, 4, 16, 4);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFile(string filename)
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile meta = manager.GetMetaFile(filename);
		if (!meta)
			return;
		ResourceName resource = meta.GetResourceID();
		if (!resource.IsEmpty() && !resource.GetPath().Contains("/Weapons/"))
			m_aResources.Insert(resource);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelect(SCR_ButtonBaseComponent button)
	{
		int index = m_aRows.Find(button);
		if (index < 0)
			return;
		m_sSelected = m_aResources[index];
		m_bCurrentGear = false;
		SetFields(m_sSelected);
		m_Status.SetText("");
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		foreach (int i, SCR_ButtonBaseComponent row : m_aRows)
			SetSelection(row, i == index);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!GC_WorkbenchItemBrowser.IsActive(m_Owner))
		{
			Close();
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshPreview()
	{
		TextWidget message = TextWidget.Cast(GetRootWidget().FindAnyWidget("PreviewStatus"));
		message.SetText("#GC-Loadout_PreviewUnavailable");
		if (!m_PreviewManager || !m_Preview)
			return;

		// The native preview manager owns this entity; never possess or delete it.
		m_PreviewManager.SetPreviewItem(m_Preview, null);
		IEntity character = m_Owner;
		if (!m_bCurrentGear)
			character = m_PreviewManager.ResolvePreviewEntityForPrefab(m_sSelected);
		if (!ChimeraCharacter.Cast(character))
			return;
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		PreviewRenderAttributes attributes;
		if (storage && storage.GetAttributes())
			attributes = PreviewRenderAttributes.Cast(storage.GetAttributes().FindAttribute(SCR_CharacterInventoryPreviewAttributes));
		if (m_bCurrentGear)
			m_PreviewManager.SetPreviewItem(m_Preview, m_Owner, attributes, true);
		else
			m_PreviewManager.SetPreviewItemFromPrefab(m_Preview, m_sSelected, attributes, true);
		message.SetText("");
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshActions()
	{
		Widget root = GetRootWidget();
		SCR_ButtonTextComponent current = SCR_ButtonTextComponent.GetButtonText("Current", root);
		current.SetToggleable(true);
		SetSelection(current, m_bCurrentGear);
		SetRoleIcon(current.GetRootWidget(), GC_WorkbenchItemBrowser.GetEditingPrefab());
		SCR_ButtonTextComponent.GetButtonText("Save", root).SetEnabled(true);
		SCR_ButtonTextComponent.GetButtonText("Load", root).SetEnabled(!m_bCurrentGear);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshRelationship()
	{
		TextWidget badge = TextWidget.Cast(GetRootWidget().FindAnyWidget("Parent"));
		badge.SetText("");
		Resource resource = Resource.Load(m_sSelected);
		if (resource && resource.IsValid())
		{
			BaseContainer source = resource.GetResource().ToEntitySource();
			if (source && source.GetAncestor())
			{
				badge.SetText(WidgetManager.Translate("#GC-Loadout_Inherits") + " " + Label(source.GetAncestor().GetResourceName()));
				badge.SetColor(Color.White);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (m_OverwriteDialog)
			return;
		string action = button.GetRootWidget().GetName();
		if (action == "Close")
		{
			Close();
			return;
		}
		if (!GC_WorkbenchItemBrowser.IsActive(m_Owner))
			return;
		if (action == "Current")
		{
			m_bCurrentGear = true;
			m_sSelected = GC_WorkbenchItemBrowser.GetEditingPrefab();
			m_Name.SetText("");
			SetFields(m_sSelected);
			m_Status.SetText("");
			foreach (SCR_ButtonBaseComponent row : m_aRows)
				SetSelection(row, false);
			RefreshRelationship();
			RefreshPreview();
			RefreshActions();
			return;
		}
		if (action == "Load" && m_bCurrentGear)
			return;
		m_Status.SetColor(Color.White);
		if (action == "Load")
		{
			if (m_sSelected.IsEmpty() || !GC_WorkbenchItemBrowser.LoadPrefab(m_sSelected))
				m_Status.SetText("#GC-Loadout_LoadFailed");
			else
				Close();
			return;
		}
		string path = GC_LoadoutPrefab.Destination(m_Faction.GetText(), m_Group.GetText(), m_Name.GetText());
		if (path.IsEmpty())
		{
			m_Status.SetText("#GC-Loadout_InvalidName");
			return;
		}
		GC_LoadoutMetadata metadata = GetMetadata();
		if (!metadata || !metadata.IsValid())
		{
			m_Status.SetText("#GC-Loadout_InvalidMetadata");
			return;
		}
		ResourceName target;
		bool created;
		if (FileIO.FileExists(path))
		{
			if (action != "Save")
			{
				m_Status.SetText("#GC-Loadout_NameExists");
				return;
			}
			ResourceManager manager = Workbench.GetModule(ResourceManager);
			MetaFile meta = manager.GetMetaFile(path);
			if (meta)
				target = meta.GetResourceID();
			if (target.IsEmpty())
			{
				m_Status.SetText("#GC-Loadout_CreateFailed");
				return;
			}
			m_sOverwriteTarget = target;
			m_OverwriteDialog = SCR_ConfigurableDialogUi.CreateFromPreset("{DC391DA320AE5000}Configs/UI/GC_LoadoutDialogs.conf", "overwrite");
			if (m_OverwriteDialog)
			{
				// Native configurable dialogs place their layout inside a proxy menu.
				m_OverwriteDialog.GetRootWidget().GetParent().SetZOrder(GetRootWidget().GetZOrder() + 1);
				m_OverwriteDialog.SetTitle(WidgetManager.Translate("#GC-Loadout_Overwrite") + " " + Label(target) + "?");
				m_OverwriteDialog.m_OnConfirm.Insert(OnOverwrite);
				m_OverwriteDialog.m_OnCloseStart.Insert(OnOverwriteClosed);
			}
			return;
		}
		else
		{
			ResourceName parent = m_sSelected;
			target = GC_LoadoutPrefab.Create(parent, path, action == "Duplicate");
			created = !target.IsEmpty();
		}
		FinishSave(target, created, m_bCurrentGear || action == "Save");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (m_Icon && m_Icon.IsExpanded())
		{
			m_Icon.CloseList();
			return;
		}
		if (!SCR_ConfigurableDialogUi.GetCurrentDialog())
			Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOverwrite(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog != m_OverwriteDialog || !GC_WorkbenchItemBrowser.IsActive(m_Owner))
			return;
		ResourceName target = m_sOverwriteTarget;
		ClearOverwrite();
		FinishSave(target, false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOverwriteClosed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog == m_OverwriteDialog)
			ClearOverwrite();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearOverwrite()
	{
		if (m_OverwriteDialog)
		{
			m_OverwriteDialog.m_OnConfirm.Remove(OnOverwrite);
			m_OverwriteDialog.m_OnCloseStart.Remove(OnOverwriteClosed);
		}
		m_OverwriteDialog = null;
		m_sOverwriteTarget = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void FinishSave(ResourceName target, bool created, bool writeGear)
	{
		if (target.IsEmpty())
		{
			m_Status.SetText("#GC-Loadout_CreateFailed");
			return;
		}
		GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
		GC_LoadoutMetadata metadata = GetMetadata();
		if (!metadata || !writer.Open(target) || !writer.Save(m_Owner, metadata, writeGear))
		{
			if (created)
				GC_WorkbenchItemBrowser.SetEditingPrefab(target);
			m_sSelected = target;
			RefreshList();
			RefreshRelationship();
			m_Status.SetText("Save incomplete. Your current gear is retained; retry Save. " + writer.GetError());
			return;
		}
		GC_WorkbenchItemBrowser.SetEditingPrefab(target);
		if (!GC_WorkbenchItemBrowser.LoadPrefab(target))
		{
			m_sSelected = target;
			RefreshList();
			SetFields(target);
			RefreshRelationship();
			RefreshPreview();
			m_Status.SetText("#GC-Loadout_SavedLoadFailed");
			return;
		}
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		SCR_ConfigurableDialogUi dialog = m_OverwriteDialog;
		ClearOverwrite();
		if (dialog)
			dialog.Close();
		if (m_PreviewManager && m_Preview)
			m_PreviewManager.SetPreviewItem(m_Preview, null);
		m_Preview = null;
		m_PreviewManager = null;
		if (m_Icon)
		{
			m_Icon.m_OnChanged.Remove(OnIconChanged);
			m_Icon.CloseList();
		}
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		foreach (SCR_ButtonBaseComponent button : m_aButtons)
			button.m_OnClicked.Remove(OnAction);
		foreach (SCR_ButtonBaseComponent row : m_aRows)
			row.m_OnClicked.Remove(OnSelect);
		m_aButtons.Clear();
		m_aRows.Clear();
		super.OnMenuClose();
	}
#else
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Close();
	}
#endif
}

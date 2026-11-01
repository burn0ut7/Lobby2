//! Modal native menu; file authoring is compiled only for Workbench.
class GC_LoadoutFilesMenu : ChimeraMenuBase
{
#ifdef WORKBENCH
	protected IEntity m_Owner;
	protected GC_LoadoutScopeSelector m_Faction;
	protected GC_LoadoutScopeSelector m_Group;
	protected string m_sScopeFaction;
	protected bool m_bScopeFactionChosen;
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
	protected ref array<ResourceName> m_aRowResources = {};
	protected ref array<SCR_ButtonBaseComponent> m_aVariantRows = {};
	protected ref array<ResourceName> m_aVariantOwners = {};
	protected ref array<int> m_aVariantIndices = {};
	protected ref GC_LoadoutVariants m_Variants;
	protected ResourceName m_sVariantOwner;
	protected int m_iVariant = -1;
	protected bool m_bVariantsDirty;
	protected SCR_ComboBoxComponent m_VariantPicker;
	protected ref array<ResourceName> m_aVariantChoices = {};
	protected SCR_ConfigurableDialogUi m_DeleteDialog;
	protected bool m_bVariantSaveFailed;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Owner = GetGame().GetPlayerController().GetControlledEntity();
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
		{
			Close();
			return;
		}
		Widget root = GetRootWidget();
		m_Faction = GC_LoadoutScopeSelector.Cast(root.FindAnyWidget("Faction").FindHandler(GC_LoadoutScopeSelector));
		m_Group = GC_LoadoutScopeSelector.Cast(root.FindAnyWidget("Group").FindHandler(GC_LoadoutScopeSelector));
		m_Faction.m_OnFilterChanged.Insert(OnFactionFilter);
		m_Group.m_OnFilterChanged.Insert(ApplyScopeFilter);
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
		m_VariantPicker = SCR_ComboBoxComponent.Cast(root.FindAnyWidget("VariantPrefab").FindHandler(SCR_ComboBoxComponent));
		m_VariantPicker.m_OnChanged.Insert(OnVariantPrefabChanged);
		array<string> actions = {"Close", "Load", "Save", "Inherit", "Duplicate", "Current", "AddVariant", "DeleteVariant", "VariantLoad", "VariantSave"};
		foreach (string action : actions)
		{
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(action, root);
			button.SetText("#GC-Loadout_" + action);
			if (action == "VariantLoad")
				button.SetText("#GC-Loadout_Load");
			else if (action == "VariantSave")
				button.SetText("#GC-Loadout_Save");
			button.m_OnClicked.Insert(OnAction);
			m_aButtons.Insert(button);
		}
		m_sSelected = GC_PrefabAuthoringSession.GetEditingPrefab();
		RefreshList();
		SetFields(m_sSelected);
		RefreshTeamChoices();
		ApplyScopeFilter();
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	protected string Label(ResourceName resource)
	{
		if (resource.IsEmpty())
			return WidgetManager.Translate("#GC-Loadout_Current");
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
		RefreshTeamChoices();
		ApplyScopeFilter();
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
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
		{
			variantRow.m_OnClicked.Remove(OnVariantSelect);
			variantRow.GetRootWidget().RemoveFromHierarchy();
		}
		m_aVariantRows.Clear();
		m_aVariantOwners.Clear();
		m_aVariantIndices.Clear();
		foreach (SCR_ButtonBaseComponent row : m_aRows)
		{
			row.m_OnClicked.Remove(OnSelect);
			row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
		m_aRowResources.Clear();
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
		map<ResourceName, ref GC_LoadoutVariants> lists = new map<ResourceName, ref GC_LoadoutVariants>();
		set<ResourceName> referenced = new set<ResourceName>();
		foreach (ResourceName resource : m_aResources)
		{
			GC_LoadoutVariants variants = GC_LoadoutVariants.Load(resource);
			if (m_Variants && resource == m_sVariantOwner)
				variants = m_Variants;
			lists.Insert(resource, variants);
			if (variants)
			{
				foreach (SCR_EditableEntityVariant entry : variants.m_aEntries)
					referenced.Insert(entry.m_sVariantPrefab);
			}
		}
		foreach (ResourceName resource : m_aResources)
		{
			GC_LoadoutVariants variants = lists.Get(resource);
			// Concrete choices already have a home under their pool. Pools remain
			// roots, so shared references and even cycles cannot hide every root.
			if (referenced.Contains(resource) && variants && variants.m_aEntries.IsEmpty())
				continue;
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", GetRootWidget().FindAnyWidget("SavedRows"));
			SCR_ButtonTextComponent row = SCR_ButtonTextComponent.Cast(widget.FindHandler(SCR_ButtonTextComponent));
			row.SetText(Label(resource));
			row.SetToggleable(true);
			SetSelection(row, !m_bCurrentGear && m_iVariant < 0 && resource == m_sSelected);
			SetRoleIcon(widget, resource);
			row.m_OnClicked.Insert(OnSelect);
			m_aRows.Insert(row);
			m_aRowResources.Insert(resource);
			if (!variants)
				continue;
			foreach (int index, SCR_EditableEntityVariant entry : variants.m_aEntries)
			{
				Widget child = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", GetRootWidget().FindAnyWidget("SavedRows"));
				AlignableSlot.SetPadding(child, 28, 0, 0, 2);
				SCR_ButtonTextComponent variantRow = SCR_ButtonTextComponent.Cast(child.FindHandler(SCR_ButtonTextComponent));
				string number = (index + 1).ToString();
				if (index < 9)
					number = "0" + number;
				variantRow.SetText(number);
				variantRow.SetToggleable(true);
				SetSelection(variantRow, resource == m_sVariantOwner && index == m_iVariant);
				variantRow.m_OnClicked.Insert(OnVariantSelect);
				m_aVariantRows.Insert(variantRow);
				m_aVariantOwners.Insert(resource);
				m_aVariantIndices.Insert(index);
			}
		}
		RefreshScopeChoices();
	}

	//------------------------------------------------------------------------------------------------
	protected bool ScopeParts(ResourceName resource, out string faction, out string team)
	{
		string path = resource.GetPath();
		if (!path.StartsWith("Prefabs/Loadouts/"))
			return false;
		array<string> parts = {};
		path.Split("/", parts, true);
		if (parts.Count() != 5)
			return false;
		faction = parts[2];
		team = parts[3];
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddScopeChoice(array<string> values, string value)
	{
		if (value.IsEmpty())
			return;
		string folded = value;
		folded.ToLower();
		foreach (string existing : values)
		{
			existing.ToLower();
			if (existing == folded)
				return;
		}
		values.Insert(value);
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesScope(string value, GC_LoadoutScopeSelector selector)
	{
		string query = selector.GetText();
		query.ToLower();
		value.ToLower();
		if (query.IsEmpty())
			return true;
		if (selector.HasChoice())
			return value == query;
		return value.Contains(query);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshScopeChoices()
	{
		array<string> values = {};
		array<Faction> factions = {};
		if (GetGame().GetFactionManager())
			GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction faction : factions)
			AddScopeChoice(values, faction.GetFactionKey());
		foreach (ResourceName resource : m_aResources)
		{
			string factionName, team;
			if (ScopeParts(resource, factionName, team))
				AddScopeChoice(values, factionName);
		}
		values.Sort();
		m_Faction.Populate(values, "#GC-Loadout_AllFactions");
		RefreshTeamChoices();
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshTeamChoices()
	{
		m_sScopeFaction = m_Faction.GetText();
		m_bScopeFactionChosen = m_Faction.HasChoice();
		array<string> values = {};
		foreach (ResourceName resource : m_aResources)
		{
			string faction, team;
			if (ScopeParts(resource, faction, team) && MatchesScope(faction, m_Faction))
				AddScopeChoice(values, team);
		}
		values.Sort();
		m_Group.Populate(values, "#GC-Loadout_AllTeams");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionFilter()
	{
		GetGame().GetCallqueue().Remove(ApplyFactionFilter);
		GetGame().GetCallqueue().CallLater(ApplyFactionFilter, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyFactionFilter()
	{
		// Focus commits can report the same edit again while the team field is typing.
		if (m_sScopeFaction == m_Faction.GetText() && m_bScopeFactionChosen == m_Faction.HasChoice())
		{
			ApplyScopeFilter();
			return;
		}
		m_Group.CloseList();
		m_Group.SetText("");
		RefreshTeamChoices();
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesResourceScope(ResourceName resource)
	{
		string faction, team;
		return ScopeParts(resource, faction, team) && MatchesScope(faction, m_Faction) && MatchesScope(team, m_Group);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyScopeFilter()
	{
		foreach (int index, SCR_ButtonBaseComponent row : m_aRows)
			row.GetRootWidget().SetVisible(MatchesResourceScope(m_aRowResources[index]));
		foreach (int index, SCR_ButtonBaseComponent row : m_aVariantRows)
			row.GetRootWidget().SetVisible(MatchesResourceScope(m_aVariantOwners[index]));
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanChangeVariantOwner(ResourceName target)
	{
		if (m_bVariantsDirty && target != m_sVariantOwner)
		{
			m_Status.SetText("#GC-Loadout_PendingVariants");
			return false;
		}
		if (target != m_sVariantOwner)
		{
			m_Variants = null;
			m_sVariantOwner = target;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ReadVariants()
	{
		if (!CanChangeVariantOwner(m_sSelected))
			return false;
		if (!m_Variants)
			m_Variants = GC_LoadoutVariants.Load(m_sSelected);
		return m_Variants != null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnVariantSelect(SCR_ButtonBaseComponent button)
	{
		if (m_DeleteDialog || m_OverwriteDialog)
			return;
		int index = m_aVariantRows.Find(button);
		if (index < 0 || !CanChangeVariantOwner(m_aVariantOwners[index]))
			return;
		m_sSelected = m_aVariantOwners[index];
		if (!ReadVariants())
			return;
		m_iVariant = m_aVariantIndices[index];
		m_bCurrentGear = false;
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVariantSelection()
	{
		RefreshActions();
		RefreshPreview();
		foreach (SCR_ButtonBaseComponent row : m_aRows)
			SetSelection(row, false);
		foreach (int i, SCR_ButtonBaseComponent row : m_aVariantRows)
			SetSelection(row, m_aVariantOwners[i] == m_sVariantOwner && m_aVariantIndices[i] == m_iVariant);
		if (m_bVariantsDirty)
			m_Status.SetText("#GC-Loadout_PendingVariants");
		else
			m_Status.SetText("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AddVariantChoice(ResourceName prefab)
	{
		if (prefab.IsEmpty() || prefab == m_sSelected || m_aVariantChoices.Contains(prefab) || !GC_LoadoutVariants.Load(prefab))
			return;
		m_aVariantChoices.Insert(prefab);
		m_VariantPicker.AddItem(Label(prefab));
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVariantControls()
	{
		Widget root = GetRootWidget();
		bool variantSelected = m_iVariant >= 0;
		root.FindAnyWidget("PrefabFields").SetVisible(!variantSelected);
		root.FindAnyWidget("PrefabFields").SetEnabled(!m_bVariantsDirty);
		root.FindAnyWidget("VariantFields").SetVisible(variantSelected);
		root.FindAnyWidget("FileActions").SetVisible(!variantSelected);
		root.FindAnyWidget("VariantActions").SetVisible(variantSelected);
		root.FindAnyWidget("VariantSave").SetVisible(m_bVariantsDirty);
		bool writable = m_sSelected.GetPath().StartsWith("Prefabs/Loadouts/");
		SCR_ButtonTextComponent.GetButtonText("AddVariant", root).SetEnabled(writable);
		SCR_ButtonTextComponent.GetButtonText("DeleteVariant", root).SetEnabled(writable && variantSelected);
		SCR_ButtonTextComponent.GetButtonText("Load", root).SetEnabled(!m_bCurrentGear && !m_bVariantsDirty);
		SCR_ButtonTextComponent.GetButtonText("VariantLoad", root).SetEnabled(!m_bVariantsDirty);
		root.FindAnyWidget("Inherit").SetVisible(!m_bVariantsDirty);
		root.FindAnyWidget("Duplicate").SetVisible(!m_bVariantsDirty);
		root.FindAnyWidget("AddVariant").SetVisible(!m_bVariantsDirty);
		if (!variantSelected)
			return;
		TextWidget.Cast(root.FindAnyWidget("VariantParent")).SetText(WidgetManager.Translate("#GC-Loadout_VariantOf") + " " + Label(m_sSelected));
		m_VariantPicker.ClearAll();
		m_aVariantChoices.Clear();
		m_aVariantChoices.Insert(string.Empty);
		m_VariantPicker.AddItem("#GC-Loadout_Current");
		foreach (ResourceName prefab : m_aResources)
			AddVariantChoice(prefab);
		foreach (SCR_EditableEntityVariant entry : m_Variants.m_aEntries)
			AddVariantChoice(entry.m_sVariantPrefab);
		SCR_EditableEntityVariant selected = m_Variants.m_aEntries[m_iVariant];
		m_VariantPicker.SetCurrentItem(m_aVariantChoices.Find(selected.m_sVariantPrefab), false, false, false);
		m_VariantPicker.SetEnabled(writable);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnVariantPrefabChanged(SCR_ComboBoxComponent combo, int index)
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner) || m_iVariant < 0 || index < 0 || index >= m_aVariantChoices.Count())
			return;
		m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab = m_aVariantChoices[index];
		m_bVariantsDirty = true;
		RefreshList();
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HandleVariantAction(string action)
	{
		if (action == "Save" && (m_iVariant >= 0 || m_bVariantsDirty))
		{
			GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
			if (m_bVariantSaveFailed)
			{
				m_Status.SetText("#GC-Loadout_VariantsFailed");
				return true;
			}
			if (!m_Variants.IsValid(m_sSelected, true))
			{
				m_Status.SetText("#GC-Loadout_InvalidVariants");
				return true;
			}
			foreach (int index, SCR_EditableEntityVariant entry : m_Variants.m_aEntries)
			{
				if (!entry.m_sVariantPrefab.IsEmpty())
					continue;
				ResourceName created;
				if (!GC_LoadoutPrefab.CreateGearVariant(m_Owner, m_sSelected, index + 1, created))
				{
					m_bVariantSaveFailed = true;
					m_Status.SetText(WidgetManager.Translate("#GC-Loadout_VariantsFailed") + " " + created.GetPath());
					return true;
				}
				entry.m_sVariantPrefab = created;
			}
			if (!writer.Open(m_sSelected) || !writer.SaveVariants(m_Owner, m_Variants))
				m_Status.SetText("#GC-Loadout_VariantsFailed");
			else
			{
				m_bVariantsDirty = false;
				RefreshList();
				RefreshActions();
				m_Status.SetText("#GC-Loadout_VariantsSaved");
			}
			return true;
		}
		if (action == "DeleteVariant")
		{
			if (m_iVariant < 0)
				return true;
			m_DeleteDialog = SCR_ConfigurableDialogUi.CreateFromPreset("{DC391DA320AE5000}Configs/UI/GC_LoadoutDialogs.conf", "deleteVariant");
			if (m_DeleteDialog)
			{
				m_DeleteDialog.GetRootWidget().GetParent().SetZOrder(GetRootWidget().GetZOrder() + 1);
				string title = WidgetManager.Translate("#GC-Loadout_DeleteVariant") + " " + Label(m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab);
				m_DeleteDialog.SetTitle(title + " · " + Label(m_sSelected) + "?");
				m_DeleteDialog.m_OnConfirm.Insert(OnDeleteVariantConfirmed);
				m_DeleteDialog.m_OnCloseStart.Insert(OnDeleteVariantClosed);
			}
			return true;
		}
		if (action == "AddVariant")
		{
			if (!m_sSelected.GetPath().StartsWith("Prefabs/Loadouts/") || !ReadVariants())
				return true;
			SCR_EditableEntityVariant entry = new SCR_EditableEntityVariant();
			entry.m_bEnabled = true;
			entry.m_iRandomizerWeight = 1;
			m_iVariant = m_Variants.m_aEntries.Insert(entry);
			m_bVariantsDirty = true;
			m_bCurrentGear = false;
			RefreshList();
			RefreshVariantSelection();
			return true;
		}
		if (m_bVariantsDirty && (action == "Load" || action == "Inherit" || action == "Duplicate"))
		{
			m_Status.SetText("#GC-Loadout_PendingVariants");
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeleteVariantConfirmed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog != m_DeleteDialog || !GC_PrefabAuthoringSession.IsAvailable(m_Owner) || m_iVariant < 0)
			return;
		m_Variants.m_aEntries.Remove(m_iVariant);
		m_iVariant = Math.Min(m_iVariant, m_Variants.m_aEntries.Count() - 1);
		m_bVariantsDirty = true;
		RefreshList();
		SetFields(m_sSelected);
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeleteVariantClosed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog == m_DeleteDialog)
		{
			dialog.m_OnConfirm.Remove(OnDeleteVariantConfirmed);
			dialog.m_OnCloseStart.Remove(OnDeleteVariantClosed);
			m_DeleteDialog = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearDeleteDialog()
	{
		SCR_ConfigurableDialogUi dialog = m_DeleteDialog;
		if (!dialog)
			return;
		OnDeleteVariantClosed(dialog);
		dialog.Close();
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
		if (!CanChangeVariantOwner(m_aRowResources[index]))
			return;
		m_iVariant = -1;
		m_sSelected = m_aRowResources[index];
		m_bCurrentGear = false;
		SetFields(m_sSelected);
		m_Status.SetText("");
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		foreach (int i, SCR_ButtonBaseComponent row : m_aRows)
			SetSelection(row, i == index);
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
			SetSelection(variantRow, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
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
		ResourceName previewPrefab = m_sSelected;
		if (m_iVariant >= 0)
			previewPrefab = m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab;
		bool currentPreview = m_bCurrentGear || previewPrefab.IsEmpty();
		if (!currentPreview)
			character = m_PreviewManager.ResolvePreviewEntityForPrefab(previewPrefab);
		if (!ChimeraCharacter.Cast(character))
			return;
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		PreviewRenderAttributes attributes;
		if (storage && storage.GetAttributes())
			attributes = PreviewRenderAttributes.Cast(storage.GetAttributes().FindAttribute(SCR_CharacterInventoryPreviewAttributes));
		if (currentPreview)
			m_PreviewManager.SetPreviewItem(m_Preview, m_Owner, attributes, true);
		else
			m_PreviewManager.SetPreviewItemFromPrefab(m_Preview, previewPrefab, attributes, true);
		message.SetText("");
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshActions()
	{
		Widget root = GetRootWidget();
		SCR_ButtonTextComponent current = SCR_ButtonTextComponent.GetButtonText("Current", root);
		current.SetToggleable(true);
		SetSelection(current, m_bCurrentGear);
		SetRoleIcon(current.GetRootWidget(), GC_PrefabAuthoringSession.GetEditingPrefab());
		SCR_ButtonTextComponent.GetButtonText("Save", root).SetEnabled(true);
		SCR_ButtonTextComponent.GetButtonText("Load", root).SetEnabled(!m_bCurrentGear);
		RefreshVariantControls();
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
		if (m_OverwriteDialog || m_DeleteDialog)
			return;
		string action = button.GetRootWidget().GetName();
		if (action == "VariantLoad")
			action = "Load";
		else if (action == "VariantSave")
			action = "Save";
		if (action == "Close")
		{
			Close();
			return;
		}
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
			return;
		if (HandleVariantAction(action))
			return;
		if (action == "Current")
		{
			if (!CanChangeVariantOwner(GC_PrefabAuthoringSession.GetEditingPrefab()))
				return;
			m_iVariant = -1;
			m_bCurrentGear = true;
			m_sSelected = GC_PrefabAuthoringSession.GetEditingPrefab();
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
			ResourceName targetPrefab = m_sSelected;
			if (m_iVariant >= 0)
				targetPrefab = m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab;
			if (targetPrefab.IsEmpty() || !GC_PrefabAuthoringSession.LoadPrefab(targetPrefab))
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
		if (m_Faction && m_Faction.IsOpened())
		{
			m_Faction.CloseList();
			return;
		}
		if (m_Group && m_Group.IsOpened())
		{
			m_Group.CloseList();
			return;
		}
		if (m_VariantPicker && m_VariantPicker.IsOpened())
		{
			m_VariantPicker.CloseList();
			return;
		}
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
		if (dialog != m_OverwriteDialog || !GC_PrefabAuthoringSession.IsAvailable(m_Owner))
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
				GC_PrefabAuthoringSession.SetEditingPrefab(target);
			m_sSelected = target;
			RefreshList();
			RefreshRelationship();
			m_Status.SetText("Save incomplete. Your current gear is retained; retry Save. " + writer.GetError());
			return;
		}
		// The current character already contains the gear just written. Saving
		// must not respawn it or restart the local Workbench session.
		if (!writeGear && !GC_PrefabAuthoringSession.LoadPrefab(target))
		{
			m_sSelected = target;
			RefreshList();
			SetFields(target);
			RefreshRelationship();
			RefreshPreview();
			m_Status.SetText("#GC-Loadout_SavedLoadFailed");
			return;
		}
		GC_PrefabAuthoringSession.SetEditingPrefab(target);
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(ApplyFactionFilter);
		if (m_Faction)
		{
			m_Faction.m_OnFilterChanged.Remove(OnFactionFilter);
			m_Faction.CloseList();
		}
		if (m_Group)
		{
			m_Group.m_OnFilterChanged.Remove(ApplyScopeFilter);
			m_Group.CloseList();
		}
		ClearDeleteDialog();
		if (m_VariantPicker)
		{
			m_VariantPicker.m_OnChanged.Remove(OnVariantPrefabChanged);
			m_VariantPicker.CloseList();
		}
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
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
			variantRow.m_OnClicked.Remove(OnVariantSelect);
		m_aVariantRows.Clear();
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

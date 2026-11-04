#ifdef WORKBENCH
//! Creates a standard header; no world scans, shared overrides or new file format.
[WorkbenchPluginAttribute(name: "Create Mission Header", description: "Create a GC mission .conf beside the active saved mission world. Existing headers are never overwritten.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF044)]
class GC_CreateMissionHeaderPlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction() || !api.GetWorld())
		{
			Workbench.Dialog("Create Mission Header", "Open your mission world in edit mode first.");
			return;
		}
		int subscene = api.GetCurrentSubScene();
		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (terrain && terrain.GetSubScene() == subscene)
		{
			Workbench.Dialog("Create Mission Header", "Select your mission subscene, not the base terrain.");
			return;
		}
		string world = api.GetWorld().GetSubSceneName(subscene);
		string absoluteWorld;
		if (world.IsEmpty() || !Workbench.GetAbsolutePath(world, absoluteWorld) || !FileIO.FileExists(absoluteWorld))
		{
			Workbench.Dialog("Create Mission Header", "Save your mission world into your addon first.");
			return;
		}
		string output = FilePath.StripExtension(absoluteWorld) + "_header.conf";
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile worldMeta;
		if (manager)
			worldMeta = manager.GetMetaFile(absoluteWorld);
		if (!worldMeta)
		{
			Workbench.Dialog("Create Mission Header", "The mission world is not registered yet. Save it and try again.");
			return;
		}
		ResourceName worldResource = worldMeta.GetResourceID();
		if (FileIO.FileExists(output) || FileIO.FileExists(output + ".meta"))
		{
			Workbench.Dialog("Create Mission Header", "A header already exists here. Open it in the Resource Browser to edit it.\n" + output);
			return;
		}
		GC_MissionHeaderForm form = new GC_MissionHeaderForm();
		form.m_sName = FilePath.StripExtension(FilePath.StripPath(absoluteWorld));
		form.m_sName.Replace("_", " ");
		Resource resource = BaseContainerTools.CreateContainer("GC_MissionHeader");
		if (!resource || !resource.IsValid())
			return;
		BaseContainer header = resource.GetResource().ToBaseContainer();
		form.m_Advanced = new GC_HeaderAdvancedSettings();
		form.m_Advanced.Read(header);
		Resource catalogDefaults = BaseContainerTools.CreateContainer("GC_CatalogMissionMetadata");
		if (!catalogDefaults || !catalogDefaults.IsValid())
			return;
		form.m_GC_Catalog = GC_CatalogMissionMetadata.Cast(BaseContainerTools.CreateInstanceFromContainer(catalogDefaults.GetResource().ToBaseContainer()));
		string description = "World: " + world + "\nCreates: " + FilePath.StripPath(output);
		while (Workbench.ScriptDialog("Create Mission Header", description, form))
		{
			form.m_sName.TrimInPlace();
			if (form.m_sName.IsEmpty() || form.m_iPlayerCount < 1)
			{
				Workbench.Dialog("Create Mission Header", "Enter a mission name and a player count of at least 1.");
				continue;
			}
			string error;
			if (!form.Create(header, worldResource, output, error))
			{
				Workbench.Dialog("Create Mission Header", error);
				return;
			}
			if (!manager || !manager.RegisterResourceFile(output, true))
			{
				Workbench.Dialog("Create Mission Header", "The file was saved, but resource registration failed. Check the Resource Browser before using it.\n" + output);
				return;
			}
			Workbench.Dialog("Mission Header Created", "Open this file in the Resource Browser to edit it.\n" + output);
			return;
		}
	}
}

[BaseContainerProps()]
class GC_MissionHeaderForm
{
	[Attribute("", UIWidgets.EditBox)]
	string m_sName;
	[Attribute("", UIWidgets.EditBox)]
	string m_sAuthor;
	[Attribute("", UIWidgets.EditBox)]
	string m_sDescription;
	[Attribute("1", UIWidgets.EditBox, "Maximum players for the mission.")]
	int m_iPlayerCount = 1;
	[Attribute("", UIWidgets.Object, "Mission categories and author-provided information.", "noDetails")]
	ref GC_CatalogMissionMetadata m_GC_Catalog;
	[Attribute("", UIWidgets.Object, "Optional technical settings. Other settings retain their normal game defaults.", "noDetails")]
	ref GC_HeaderAdvancedSettings m_Advanced;

	//------------------------------------------------------------------------------------------------
	bool Create(BaseContainer header, ResourceName world, string output, out string error)
	{
		m_sName.TrimInPlace();
		if (FileIO.FileExists(output) || FileIO.FileExists(output + ".meta"))
		{
			error = "Nothing was replaced: this header already exists.\n" + output;
			return false;
		}
		if (!header || !m_GC_Catalog || !m_Advanced || m_sName.IsEmpty() || m_iPlayerCount < 1 || world.IsEmpty())
		{
			error = "Enter a name, valid player count, GC Catalog and Advanced settings.";
			return false;
		}
		Resource catalog = BaseContainerTools.CreateContainerFromInstance(m_GC_Catalog);
		if (!catalog || !catalog.IsValid()
			|| !header.Set("World", world)
			|| !header.Set("m_sName", m_sName)
			|| !header.Set("m_sAuthor", m_sAuthor)
			|| !header.Set("m_sDescription", m_sDescription)
			|| !header.Set("m_iPlayerCount", m_iPlayerCount)
			|| !header.SetObject("m_GC_Catalog", catalog.GetResource().ToBaseContainer())
			|| !m_Advanced.Apply(header))
		{
			error = "The mission header could not be prepared. No file was saved.";
			return false;
		}
		if (!BaseContainerTools.SaveContainer(header, "", output))
		{
			error = "The header could not be saved. Check the destination and console before trying again.\n" + output;
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Create header")]
	bool CreateHeader()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	bool Cancel()
	{
		return false;
	}
}

[BaseContainerProps()]
class GC_HeaderAdvancedSettings
{
	[Attribute("Sandbox", UIWidgets.EditBox)]
	string m_sGameMode;
	[Attribute("", UIWidgets.ResourceNamePicker, params: "conf")]
	ResourceName m_sBriefingConfig;
	[Attribute("1", UIWidgets.CheckBox)]
	bool m_bShowInMenu;
	[Attribute("1", UIWidgets.EditBox)]
	float m_fXpMultiplier;
	[Attribute("10", UIWidgets.EditBox)]
	int m_iMarkersPerPlayer;
	[Attribute("0", UIWidgets.CheckBox)]
	bool m_bAllowArmavision;

	//------------------------------------------------------------------------------------------------
	void Read(BaseContainer header)
	{
		header.Get("m_sGameMode", m_sGameMode);
		header.Get("m_sBriefingConfig", m_sBriefingConfig);
		header.Get("m_bShowInScenarioMenu", m_bShowInMenu);
		header.Get("m_fXpMultiplier", m_fXpMultiplier);
		header.Get("m_iMapMarkerLimitPerPlayer", m_iMarkersPerPlayer);
		header.Get("m_bIsArmavisionAllowedInMP", m_bAllowArmavision);
	}

	//------------------------------------------------------------------------------------------------
	bool Apply(BaseContainer header)
	{
		return header.Set("m_sGameMode", m_sGameMode)
			&& header.Set("m_sBriefingConfig", m_sBriefingConfig)
			&& header.Set("m_bShowInScenarioMenu", m_bShowInMenu)
			&& header.Set("m_fXpMultiplier", m_fXpMultiplier)
			&& header.Set("m_iMapMarkerLimitPerPlayer", m_iMarkersPerPlayer)
			&& header.Set("m_bIsArmavisionAllowedInMP", m_bAllowArmavision);
	}
}
#endif

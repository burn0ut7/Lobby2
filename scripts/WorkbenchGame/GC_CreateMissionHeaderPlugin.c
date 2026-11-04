#ifdef WORKBENCH
//! One explicit authoring action for a mission header and its saved-world snapshot.
[WorkbenchPluginAttribute(name: "Create Mission Data", description: "Save the active mission, create its header if needed, and refresh world metadata.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF044)]
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
			Workbench.Dialog("Create Mission Data", "Open your mission world in edit mode first.");
			return;
		}
		int subscene = api.GetCurrentSubScene();
		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (terrain && terrain.GetSubScene() == subscene)
		{
			Workbench.Dialog("Create Mission Data", "Select your mission subscene, not the base terrain.");
			return;
		}
		string world = api.GetWorld().GetSubSceneName(subscene);
		string absoluteWorld;
		if (world.IsEmpty() || !Workbench.GetAbsolutePath(world, absoluteWorld))
		{
			Workbench.Dialog("Create Mission Data", "Select a saved mission subscene in your addon.\n" + world);
			return;
		}
		string output = FilePath.StripExtension(world) + "_header.conf";
		string absoluteOutput;
		if (!Workbench.GetAbsolutePath(output, absoluteOutput, false))
		{
			Workbench.Dialog("Create Mission Data", "Could not resolve the mission header destination.");
			return;
		}
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile worldMeta;
		if (manager)
			worldMeta = manager.GetMetaFile(absoluteWorld);
		if (!worldMeta)
		{
			Workbench.Dialog("Create Mission Data", "The mission world is not registered yet. Save it and try again.");
			return;
		}
		ResourceName worldResource = worldMeta.GetResourceID();
		string foundPath;
		bool existing = Workbench.GetAbsolutePath(output, foundPath);
		if (!existing && Workbench.GetAbsolutePath(output + ".meta", foundPath))
		{
			Workbench.Dialog("Create Mission Data", "A resource registration exists without its header. Restore the missing file first.\n" + output);
			return;
		}
		GC_MissionHeaderForm form;
		Resource resource;
		if (!existing)
		{
			resource = BaseContainerTools.CreateContainer("GC_MissionHeader");
			if (!resource || !resource.IsValid())
			{
				Workbench.Dialog("Create Mission Data", "Workbench could not prepare the mission header.");
				return;
			}
			form = Prompt(resource.GetResource().ToBaseContainer(), absoluteWorld);
			if (!form)
				return;
		}

		if (!editor.Save())
		{
			Workbench.Dialog("Create Mission Data", "World save failed. Mission data was not exported.");
			return;
		}
		WBProgressDialog progress = new WBProgressDialog("Collecting mission data", editor);
		GC_MissionMetadataExport exporter = new GC_MissionMetadataExport();
		bool collected = exporter.Collect(api, subscene, progress);
		progress = null;
		if (!collected)
		{
			Workbench.Dialog("Create Mission Data", "World saved. Metadata collection failed: " + exporter.GetError());
			return;
		}
		if (form && !form.m_bMetadataOnly)
		{
			string error;
			if (!form.Create(resource.GetResource().ToBaseContainer(), worldResource, output, error))
			{
				Workbench.Dialog("Create Mission Data", "World saved. " + error);
				return;
			}
		}
		if (!form || !form.m_bMetadataOnly)
		{
			if (!manager.GetMetaFile(absoluteOutput) && !manager.RegisterResourceFile(absoluteOutput, true))
			{
					Workbench.Dialog("Create Mission Data", "World and header saved. Header registration failed. Run Create Mission Data again to retry.");
				return;
			}
		}
		if (!exporter.Save(world + ".metadata.json", world))
		{
			Workbench.Dialog("Create Mission Data", "World saved. Metadata export failed. Check write access to the mission folder and the Workbench console before retrying.");
			return;
		}
		Workbench.Dialog("Mission Data Saved", "Mission saved and metadata refreshed for " + exporter.GetEntityCount().ToString() + " entities.");
	}

	//------------------------------------------------------------------------------------------------
	protected GC_MissionHeaderForm Prompt(BaseContainer header, string absoluteWorld)
	{
		GC_MissionHeaderForm form = new GC_MissionHeaderForm();
		form.m_sName = FilePath.StripExtension(FilePath.StripPath(absoluteWorld));
		form.m_sName.Replace("_", " ");
		form.m_Advanced = new GC_HeaderAdvancedSettings();
		form.m_Advanced.Read(header);
		Resource catalogDefaults = BaseContainerTools.CreateContainer("GC_CatalogMissionMetadata");
		if (!catalogDefaults || !catalogDefaults.IsValid())
			return null;
		form.m_GC_Catalog = GC_CatalogMissionMetadata.Cast(BaseContainerTools.CreateInstanceFromContainer(catalogDefaults.GetResource().ToBaseContainer()));
		string description = "World: " + FilePath.StripPath(absoluteWorld) + "\nAlready have a header elsewhere? Choose Refresh metadata only.";
		while (Workbench.ScriptDialog("Create Mission Data", description, form))
		{
			if (form.m_bMetadataOnly)
				return form;
			form.m_sName.TrimInPlace();
			if (form.m_sName.IsEmpty() || form.m_iPlayerCount < 1)
			{
				Workbench.Dialog("Create Mission Data", "Enter a mission name and a player count of at least 1.");
				continue;
			}
			return form;
		}
		return null;
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
	bool m_bMetadataOnly;

	//------------------------------------------------------------------------------------------------
	bool Create(BaseContainer header, ResourceName world, string output, out string error)
	{
		m_sName.TrimInPlace();
		string foundPath;
		if (Workbench.GetAbsolutePath(output, foundPath) || Workbench.GetAbsolutePath(output + ".meta", foundPath))
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
			error = "Header write failed. Check the mission folder and Workbench script-write permissions before retrying.\n" + output;
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Create mission data")]
	bool CreateHeader()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Refresh metadata only")]
	bool RefreshOnly()
	{
		m_bMetadataOnly = true;
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

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
		int mountEnd = world.IndexOf(":");
		if (mountEnd < 2 || world.Substring(0, 1) != "$")
		{
			Workbench.Dialog("Create Mission Data", "Select a mission world in a local addon.");
			return;
		}
		string configs = world.Substring(0, mountEnd + 1) + "Configs";
		string missions = configs + "/Missions";
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
		GC_MissionHeaderForm form = Prompt(absoluteWorld);
		if (!form)
			return;
		string authorDir = missions + "/" + form.m_sAuthor;
		string output = authorDir + "/" + HeaderFilename(form.m_sName);
		string absoluteOutput;
		string foundPath;
		Resource resource;
		if (!form.m_bMetadataOnly)
		{
			if (Workbench.GetAbsolutePath(output, foundPath) || Workbench.GetAbsolutePath(output + ".meta", foundPath))
			{
				Workbench.Dialog("Create Mission Data", "This header already exists. Edit it in Resource Manager or choose Refresh metadata only.\n" + output);
				return;
			}
			if (!EnsureDirectory(configs) || !EnsureDirectory(missions) || !EnsureDirectory(authorDir) || !Workbench.GetAbsolutePath(output, absoluteOutput, false))
			{
				Workbench.Dialog("Create Mission Data", "Could not prepare the header folder.\n" + authorDir);
				return;
			}
			resource = BaseContainerTools.CreateContainer("GC_MissionHeader");
			if (!resource || !resource.IsValid())
			{
				Workbench.Dialog("Create Mission Data", "Workbench could not prepare the mission header.");
				return;
			}
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
		if (!form.m_bMetadataOnly)
		{
			string error;
			if (!form.Create(resource.GetResource().ToBaseContainer(), worldResource, output, error))
			{
				Workbench.Dialog("Create Mission Data", "World saved. " + error);
				return;
			}
			if (!manager.GetMetaFile(absoluteOutput) && !manager.RegisterResourceFile(absoluteOutput, true))
			{
				Workbench.Dialog("Create Mission Data", "World and header saved. Header registration failed. Register the saved file in Resource Manager: \n" + output);
				return;
			}
		}
		if (!exporter.Save(world + ".metadata.json", world))
		{
			Workbench.Dialog("Create Mission Data", "World saved. Metadata export failed. Check write access to the mission folder and the Workbench console before retrying.");
			return;
		}
		string message = "Mission saved and metadata refreshed for " + exporter.GetEntityCount().ToString() + " entities.";
		if (!form.m_bMetadataOnly)
			message += "\nHeader: " + output;
		Workbench.Dialog("Mission Data Saved", message);
	}

	//------------------------------------------------------------------------------------------------
	protected bool EnsureDirectory(string path)
	{
		string absolute;
		if (Workbench.GetAbsolutePath(path, absolute))
			return true;
		return Workbench.GetAbsolutePath(path, absolute, false) && FileIO.MakeDirectory(absolute);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidPathPart(string name)
	{
		if (name.IsEmpty() || name.Length() > 120 || name.EndsWith(".") || name.EndsWith(" "))
			return false;
		string forbidden = "<>:\"/\\|?*";
		for (int i = 0; i < name.Length(); i++)
		{
			string ch = name.Substring(i, 1);
			if (ch.ToAscii() >= 0 && ch.ToAscii() < 32 || forbidden.Contains(ch))
				return false;
		}
		string deviceName = name;
		int dot = deviceName.IndexOf(".");
		if (dot >= 0)
			deviceName = deviceName.Substring(0, dot);
		deviceName.ToUpper();
		array<string> reserved = {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"};
		if (reserved.Contains(deviceName))
			return false;
		if (deviceName.Length() == 4 && (deviceName.StartsWith("COM") || deviceName.StartsWith("LPT")))
			return !"123456789".Contains(deviceName.Substring(3, 1));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected string HeaderFilename(string missionName)
	{
		missionName.Replace("_", "");
		missionName.Replace(" ", "");
		return missionName + ".conf";
	}

	//------------------------------------------------------------------------------------------------
	protected GC_MissionHeaderForm Prompt(string absoluteWorld)
	{
		GC_MissionHeaderForm form = new GC_MissionHeaderForm();
		form.m_sName = FilePath.StripExtension(FilePath.StripPath(absoluteWorld));
		form.m_sName.Replace("_", " ");
		string description = "World: " + FilePath.StripPath(absoluteWorld) + "\nHeader: Configs/Missions/<Author>/<Name>.conf";
		while (Workbench.ScriptDialog("Create Mission Data", description, form))
		{
			if (form.m_bMetadataOnly)
				return form;
			form.m_sAuthor.TrimInPlace();
			if (!ValidPathPart(form.m_sAuthor))
			{
				Workbench.Dialog("Create Mission Data", "Enter an author name that can be used as one folder name.");
				continue;
			}
			form.m_sName.TrimInPlace();
			if (form.m_sName.IsEmpty() || !ValidPathPart(FilePath.StripExtension(HeaderFilename(form.m_sName))) || !form.HasValidPlayerRange())
			{
				Workbench.Dialog("Create Mission Data", "Enter a mission name suitable for a filename. Minimum Players must be between 1 and Player Count (maximum 10000).");
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
	[Attribute("1", UIWidgets.EditBox, "Fewest players needed to run the mission.")]
	int m_iMinimumPlayers = 1;
	[Attribute("", UIWidgets.EditBox)]
	string m_sEra;
	[Attribute("Testing", UIWidgets.ComboBox, enums: GC_MissionCatalogChoices.MissionTypes())]
	string m_sMissionType = "Testing";
	[Attribute("0", UIWidgets.Flags, enums: ObjectiveFlags())]
	int m_iObjectives;
	[Attribute("0", UIWidgets.Flags, enums: CompositionFlags())]
	int m_iForceComposition;
	bool m_bMetadataOnly;

	//------------------------------------------------------------------------------------------------
	bool HasValidPlayerRange()
	{
		return m_iMinimumPlayers >= 1 && m_iMinimumPlayers <= m_iPlayerCount && m_iPlayerCount <= 10000;
	}

	//------------------------------------------------------------------------------------------------
	static ParamEnumArray ObjectiveFlags()
	{
		return Flags(GC_MissionCatalogChoices.Objectives());
	}

	//------------------------------------------------------------------------------------------------
	static ParamEnumArray CompositionFlags()
	{
		return Flags(GC_MissionCatalogChoices.ForceComposition());
	}

	//------------------------------------------------------------------------------------------------
	// Entry zero is the empty choice. Bit positions exist only while the form is open.
	protected static ParamEnumArray Flags(ParamEnumArray choices)
	{
		ParamEnumArray flags = new ParamEnumArray();
		for (int i = 1; i < choices.Count(); i++)
		{
			int bit = 1 << (i - 1);
			flags.Insert(new ParamEnum(choices[i].m_Key, bit.ToString(), choices[i].m_Desc));
		}
		return flags;
	}

	//------------------------------------------------------------------------------------------------
	protected array<string> SelectedLabels(int mask, ParamEnumArray choices)
	{
		array<string> labels = {};
		for (int i = 1; i < choices.Count(); i++)
		{
			if (mask & (1 << (i - 1)))
				labels.Insert(choices[i].m_Key);
		}
		return labels;
	}

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
		if (!header || m_sName.IsEmpty() || !HasValidPlayerRange() || world.IsEmpty())
		{
			error = "Enter a mission name and a valid player count.";
			return false;
		}
		Resource catalog = BaseContainerTools.CreateContainer("GC_CatalogMissionMetadata");
		if (!catalog || !catalog.IsValid())
		{
			error = "Could not prepare the mission categories.";
			return false;
		}
		BaseContainer fields = catalog.GetResource().ToBaseContainer();
		if (!fields.Set("m_sEra", m_sEra)
			|| !fields.Set("m_sMissionType", m_sMissionType)
			|| !fields.Set("m_aObjectives", SelectedLabels(m_iObjectives, GC_MissionCatalogChoices.Objectives()))
			|| !fields.Set("m_aPlayerAssets", SelectedLabels(m_iForceComposition, GC_MissionCatalogChoices.ForceComposition()))
			|| !fields.Set("m_iMinPlayers", m_iMinimumPlayers)
			|| !header.Set("World", world)
			|| !header.Set("m_sName", m_sName)
			|| !header.Set("m_sAuthor", m_sAuthor)
			|| !header.Set("m_sDescription", m_sDescription)
			|| !header.Set("m_iPlayerCount", m_iPlayerCount)
			|| !header.SetObject("m_GC_Catalog", fields))
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

#endif

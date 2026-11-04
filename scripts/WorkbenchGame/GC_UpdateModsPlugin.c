#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Update Mods", description: "Preview and optionally fast-forward clean GC addon checkouts.", wbModules: { "WorldEditor" }, category: "GC")]
class GC_UpdateModsPlugin : WorkbenchPlugin
{
	override void Run()
	{
		GC_ModWorkflow.Run(true, false, "");
	}
}

class GC_TemplateGitOptions
{
	[Attribute("0", UIWidgets.CheckBox, "Preview available updates for the four GC addons.")]
	bool m_bUpdateMods;

	[Attribute("0", UIWidgets.CheckBox, "Choose a name for a new local branch in the active GC Missions checkout.")]
	bool m_bCreateMissionBranch;

	[ButtonAttribute("Continue")]
	bool ContinueTemplate()
	{
		return true;
	}

	[ButtonAttribute("Cancel")]
	bool Cancel()
	{
		return false;
	}
}

class GC_ModSelection
{
	[Attribute("1", UIWidgets.CheckBox, "Lobby2")]
	bool m_bLobby2 = true;

	[Attribute("1", UIWidgets.CheckBox, "GC Core")]
	bool m_bGCCore = true;

	[Attribute("1", UIWidgets.CheckBox, "GC Units")]
	bool m_bGCUnits = true;

	[Attribute("1", UIWidgets.CheckBox, "GC Missions")]
	bool m_bGCMissions = true;

	string SelectedMods()
	{
		array<string> names = {};
		if (m_bLobby2)
			names.Insert("Lobby2");
		if (m_bGCCore)
			names.Insert("GlobalConflictsCore");
		if (m_bGCUnits)
			names.Insert("GlobalConflictsUnits");
		if (m_bGCMissions)
			names.Insert("GlobalConflictsMissions");
		return string.Join(",", names);
	}

	[ButtonAttribute("Continue")]
	bool Confirm()
	{
		return true;
	}

	[ButtonAttribute("Cancel")]
	bool Cancel()
	{
		return false;
	}
}

class GC_ModWorkflow
{
	static bool BeforeTemplate(string world)
	{
		GC_TemplateGitOptions options = new GC_TemplateGitOptions();
		if (!Workbench.ScriptDialog("Spawn Mission Template", "Both steps are optional. Leave them clear to place the template on your current branch.", options))
			return false;

		return Run(options.m_bUpdateMods, options.m_bCreateMissionBranch, world);
	}

	//------------------------------------------------------------------------------------------------
	static bool Run(bool updateMods, bool createBranch, string world)
	{
		if (!updateMods && !createBranch)
			return true;

		string selected;
		if (updateMods)
		{
			GC_ModSelection picker = new GC_ModSelection();
			if (!Workbench.ScriptDialog("Update Mods", "Choose the mods to check for updates. You can review available updates before applying them.", picker))
				return false;

			selected = picker.SelectedMods();
			if (selected.IsEmpty() && !createBranch)
				return true;
		}

		string helper;
		if (!Workbench.GetAbsolutePath("$Lobby2:tools/GC-ModLauncher.js", helper))
		{
			Workbench.Dialog("GC Git", "The Git helper is missing. Install the Lobby2 source checkout or continue with the optional steps off.");
			return false;
		}
		string command = "wscript.exe //nologo //E:JScript \"" + helper + "\"";
		array<string> mounts = {"Lobby2", "GlobalConflictsCore", "GlobalConflictsUnits", "GlobalConflictsMissions"};
		foreach (string mount : mounts)
		{
			string project;
			if (Workbench.GetAbsolutePath("$" + mount + ":addon.gproj", project))
				command += " -" + mount + " \"" + project + "\"";
		}

		if (!selected.IsEmpty())
			command += " -SelectedMods " + selected;
		if (createBranch)
		{
			string absoluteWorld;
			if (!Workbench.GetAbsolutePath(world, absoluteWorld))
			{
				Workbench.Dialog("GC Git", "Save the mission in GC Missions before creating its branch.");
				return false;
			}
			command += " -MissionWorld \"" + absoluteWorld + "\"";
		}

		int result = Workbench.RunCmd(command, true);
		if (result == 0)
			return true;
		if (result != 2)
			Workbench.Dialog("GC Git", "The Git workflow did not complete. Check that Windows PowerShell and Git for Windows are available.");
		return false;
	}
}
#endif

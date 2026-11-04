#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Update Mods", description: "Update the selected mods.", wbModules: { "WorldEditor" }, category: "GC")]
class GC_UpdateModsPlugin : WorkbenchPlugin
{
	override void Run()
	{
		GC_ModWorkflow.Run(true, false, "");
	}
}

class GC_TemplateGitOptions
{
	[Attribute("0", UIWidgets.CheckBox, "Update selected mods.")]
	bool m_bUpdateMods;

	[Attribute("0", UIWidgets.CheckBox, "Start a new mission branch.")]
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
		return string.Join(",", names, true);
	}

	[ButtonAttribute("Update")]
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
		if (!Workbench.ScriptDialog("Spawn Mission Template", "Choose any extras for this mission.", options))
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
			if (!Workbench.ScriptDialog("Update Mods", "Save your work, then choose which mods to update.", picker))
				return false;

			selected = picker.SelectedMods();
			if (selected.IsEmpty() && !createBranch)
			{
				Workbench.Dialog("Update Mods", "No mods selected.");
				return true;
			}
		}

		string command;
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

		return Launch(command);
	}

	//------------------------------------------------------------------------------------------------
	static bool Launch(string arguments)
	{
		string helper;
		if (!Workbench.GetAbsolutePath("$Lobby2:tools/GC-ModLauncher.js", helper))
		{
			Workbench.Dialog("GC Git", "Git tools are missing from Lobby2.");
			return false;
		}
		ProcessHandle process = Workbench.RunProcess("wscript.exe //nologo //E:JScript \"" + helper + "\"" + arguments);
		if (!process)
		{
			Workbench.Dialog("GC Git", "Git could not start. Open Workbench without -noThrow and allow the Git helper to run.");
			return false;
		}
		int result = Workbench.WaitProcess(process);
		Workbench.FinishProcess(process);
		PrintFormat("GC | Git | Launcher result %1", result);
		if (result == 73)
			return true;
		if (result != 74)
			Workbench.Dialog("GC Git", "Git could not finish. Check your Git installation.");
		return false;
	}
}
#endif

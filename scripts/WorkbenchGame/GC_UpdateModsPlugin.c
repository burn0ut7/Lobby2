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

		string helper;
		if (!Workbench.GetAbsolutePath("$Lobby2:tools/GC-ModWorkflow.ps1", helper))
		{
			Workbench.Dialog("GC Git", "The Git helper is missing. Install the Lobby2 source checkout or continue with the optional steps off.");
			return false;
		}
		string command = "powershell.exe -NoProfile -NonInteractive -STA -WindowStyle Hidden -ExecutionPolicy Bypass -File \"" + helper + "\"";
		array<string> mounts = {"Lobby2", "GlobalConflictsCore", "GlobalConflictsUnits", "GlobalConflictsMissions"};
		foreach (string mount : mounts)
		{
			string project;
			if (Workbench.GetAbsolutePath("$" + mount + ":addon.gproj", project))
				command += " -" + mount + " \"" + project + "\"";
		}

		if (updateMods)
			command += " -UpdateMods";
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

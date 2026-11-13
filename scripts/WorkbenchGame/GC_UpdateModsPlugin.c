#ifdef WORKBENCH
class GC_ModWorkflow
{
	// Template creation no longer interrupts authors with Git/branch dialogs.
	static bool BeforeTemplate(string world)
	{
		return true;
	}

	static string ProjectArguments()
	{
		string arguments;
		array<string> mounts = {"Lobby2", "GlobalConflictsCore", "GlobalConflictsUnits", "GlobalConflictsMissions"};
		foreach (string mount : mounts)
		{
			string project;
			if (Workbench.GetAbsolutePath("$" + mount + ":addon.gproj", project))
				arguments += " -" + mount + " \"" + project + "\"";
		}
		return arguments;
	}

}
#endif

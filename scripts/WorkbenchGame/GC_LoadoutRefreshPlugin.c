[WorkbenchPluginAttribute(name: "Loadout preview refresh", wbModules: {"WorldEditor"})]
class GC_LoadoutRefreshPlugin : WorldEditorPlugin
{
	protected bool m_bRefreshing;
	protected int m_iGeneration;

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStarted(string worldName, string gameMode, bool playFromCameraPos, vector cameraPosition, vector cameraAngles)
	{
		// A request from an interrupted previous session must never restart an unrelated session.
		if (!m_bRefreshing)
			FileIO.DeleteFile(GC_LoadoutRefresh.REQUEST_FILE);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnded()
	{
		m_iGeneration++;
		m_bRefreshing = false;
		FileHandle file = FileIO.OpenFile(GC_LoadoutRefresh.REQUEST_FILE, FileMode.READ);
		if (!file)
			return;
		string project, world, prefab, position, angles;
		file.ReadLine(project);
		file.ReadLine(world);
		file.ReadLine(prefab);
		file.ReadLine(position);
		file.ReadLine(angles);
		file.Close();
		FileIO.DeleteFile(GC_LoadoutRefresh.REQUEST_FILE);
		if (project != Workbench.GetCurrentGameProjectFile() || world.IsEmpty() || prefab.IsEmpty() || m_bRefreshing)
			return;
		m_bRefreshing = true;
		thread Restart(m_iGeneration, project, world, prefab, position.ToVector(), angles.ToVector());
	}

	//------------------------------------------------------------------------------------------------
	protected void Restart(int generation, string project, string world, ResourceName prefab, vector position, vector angles)
	{
		Sleep(1000);
		if (generation != m_iGeneration)
			return;
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		string currentWorld;
		if (editor && editor.GetApi())
			editor.GetApi().GetWorldPath(currentWorld);
		if (!editor || !editor.GetApi() || editor.GetApi().IsGameMode() || currentWorld != world || project != Workbench.GetCurrentGameProjectFile())
		{
			m_bRefreshing = false;
			return;
		}
		editor.SwitchToGameMode();
		for (int attempt; attempt < 100; attempt++)
		{
			Sleep(100);
			if (generation != m_iGeneration)
				return;
			if (!editor.GetApi())
				break;
			editor.GetApi().GetWorldPath(currentWorld);
			if (project != Workbench.GetCurrentGameProjectFile() || currentWorld != world)
				break;
			if (editor.GetApi().IsGameMode() && GetGame() && GetGame().GetPlayerController() && GetGame().GetPlayerController().GetControlledEntity())
			{
				bool resumed = GC_WorkbenchItemBrowser.ResumePrefab(prefab, position, angles);
				m_bRefreshing = false;
				if (!resumed)
					Workbench.Dialog("Loadout", "Could not reopen this prefab in the gear screen. The saved file is available in Workbench.");
				return;
			}
		}
		m_bRefreshing = false;
		Workbench.Dialog("Loadout", "The preview did not become ready. The saved prefab is available in Workbench.");
	}
}

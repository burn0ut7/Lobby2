#ifdef WORKBENCH
//! One-shot Workbench handoff. Only the resource identity and view position cross Play sessions.
class GC_LoadoutRefresh
{
	static const string REQUEST_FILE = "$profile:GC_LoadoutRefresh.txt";
	protected static ResourceName s_sPrefab;
	protected static IEntity s_Owner;

	//------------------------------------------------------------------------------------------------
	static bool Request(ResourceName prefab, IEntity owner)
	{
		if (!GC_WorkbenchItemBrowser.IsActive(owner) || prefab.IsEmpty() || s_Owner)
			return false;
		s_sPrefab = prefab;
		s_Owner = owner;
		GetGame().GetCallqueue().CallLater(StopPreview, 100);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void StopPreview()
	{
		IEntity owner = s_Owner;
		s_Owner = null;
		if (!GC_WorkbenchItemBrowser.IsActive(owner))
			return;
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		string world;
		if (!editor || !editor.GetApi())
			return;
		editor.GetApi().GetWorldPath(world);
		FileHandle file = FileIO.OpenFile(REQUEST_FILE, FileMode.WRITE);
		if (!file)
		{
			Workbench.Dialog("Loadout", "Could not reopen the gear screen. Your saved prefab is unchanged.");
			return;
		}
		file.WriteLine(Workbench.GetCurrentGameProjectFile());
		file.WriteLine(world);
		file.WriteLine(s_sPrefab);
		file.WriteLine(owner.GetOrigin().ToString(false));
		file.WriteLine(owner.GetAngles().ToString(false));
		file.Close();
		editor.SwitchToEditMode();
	}
}
#endif
